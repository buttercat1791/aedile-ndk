#include "nostr.hpp"
#include "client/web_socket_client.hpp"

using namespace nlohmann;
using namespace std;

namespace nostr
{
NostrService::NostrService(
    shared_ptr<plog::IAppender> appender,
    shared_ptr<client::IWebSocketClient> client,
    shared_ptr<ISigner> signer)
: NostrService(appender, client, signer, {}) { };

NostrService::NostrService(
    shared_ptr<plog::IAppender> appender,
    shared_ptr<client::IWebSocketClient> client,
    shared_ptr<ISigner> signer,
    RelayList relays)
: _defaultRelays(relays), _client(client), _signer(signer)
{
    plog::init(plog::debug, appender.get());
    client->start();
};

NostrService::~NostrService()
{
    this->_client->stop();
};

RelayList NostrService::defaultRelays() const { return this->_defaultRelays; };

RelayList NostrService::activeRelays() const { return this->_activeRelays; };

unordered_map<string, vector<string>> NostrService::subscriptions() const { return this->_subscriptions; };

RelayList NostrService::openRelayConnections()
{
    return this->openRelayConnections(this->_defaultRelays);
};

RelayList NostrService::openRelayConnections(RelayList relays)
{
    PLOG_INFO << "Attempting to connect to Nostr relays.";
    RelayList unconnectedRelays = this->getUnconnectedRelays(relays);

    vector<thread> connectionThreads;
    for (string relay : unconnectedRelays)
    {
        thread connectionThread([this, relay]() {
            this->connect(relay);
        });
        connectionThreads.push_back(move(connectionThread));
    }

    for (thread& connectionThread : connectionThreads)
    {
        connectionThread.join();
    }

    size_t targetCount = relays.size();
    size_t activeCount = this->_activeRelays.size();
    PLOG_INFO << "Connected to " << activeCount << "/" << targetCount << " target relays.";

    // This property should only contain successful relays at this point.
    return this->_activeRelays;
};

void NostrService::closeRelayConnections()
{
    if (this->_activeRelays.size() == 0)
    {
        PLOG_INFO << "No active relay connections to close.";
        return;
    }

    this->closeRelayConnections(this->_activeRelays);
};

void NostrService::closeRelayConnections(RelayList relays)
{
    PLOG_INFO << "Disconnecting from Nostr relays.";
    RelayList connectedRelays = getConnectedRelays(relays);

    vector<thread> disconnectionThreads;
    for (string relay : connectedRelays)
    {
        thread disconnectionThread([this, relay]() {
            this->disconnect(relay);
        });
        disconnectionThreads.push_back(move(disconnectionThread));

        lock_guard<mutex> lock(this->_propertyMutex);
        this->_subscriptions.erase(relay);
    }

    for (thread& disconnectionThread : disconnectionThreads)
    {
        disconnectionThread.join();
    }
};

// TODO: Make this method return a promise.
tuple<RelayList, RelayList> NostrService::publishEvent(shared_ptr<Event> event)
{
    RelayList successfulRelays;
    RelayList failedRelays;

    PLOG_INFO << "Attempting to publish event to Nostr relays.";

    json message;
    try
    {
        this->_signer->sign(event);
        message = json::array({ "EVENT", event->serialize() });
    }
    catch (const std::invalid_argument& e)
    {
        PLOG_ERROR << "Failed to sign event: " << e.what();
        throw e;
    }
    catch (const json::exception& je)
    {
        PLOG_ERROR << "Failed to serialize event: " << je.what();
        throw je;
    }

    lock_guard<mutex> lock(this->_propertyMutex);
    RelayList targetRelays = this->_activeRelays;
    vector<future<tuple<string, bool>>> publishFutures;
    for (const string& relay : targetRelays)
    {
        promise<tuple<string, bool>> publishPromise;
        publishFutures.push_back(move(publishPromise.get_future()));

        auto [uri, success] = this->_client->send(
            message.dump(),
            relay,
            [this, &relay, &event, &publishPromise](string response)
            {
                this->onAcceptance(response, [this, &relay, &event, &publishPromise](bool isAccepted)
                {
                    if (isAccepted)
                    {
                        PLOG_INFO << "Relay " << relay << " accepted event: " << event->id;
                        publishPromise.set_value(make_tuple(relay, true));
                    }
                    else
                    {
                        PLOG_WARNING << "Relay " << relay << " rejected event: " << event->id;
                        publishPromise.set_value(make_tuple(relay, false));
                    }
                });
            });

        if (!success)
        {
            PLOG_WARNING << "Failed to send event to relay: " << relay;
            publishPromise.set_value(make_tuple(relay, false));
        }
    }

    for (auto& publishFuture : publishFutures)
    {
        auto [relay, isSuccess] = publishFuture.get();
        if (isSuccess)
        {
            successfulRelays.push_back(relay);
        }
        else
        {
            failedRelays.push_back(relay);
        }
    }

    size_t targetCount = targetRelays.size();
    size_t successfulCount = successfulRelays.size();
    PLOG_INFO << "Published event to " << successfulCount << "/" << targetCount << " target relays.";

    return make_tuple(successfulRelays, failedRelays);
};

// TODO: Make this method return a promise.
// TODO: Add a timeout to this method to prevent hanging while waiting for the relay.
vector<shared_ptr<Event>> NostrService::queryRelays(shared_ptr<Filters> filters)
{
    if (filters->limit > 64 || filters->limit < 1)
    {
        PLOG_WARNING << "Filters limit must be between 1 and 64, inclusive.  Setting limit to 16.";
        filters->limit = 16;
    }

    vector<shared_ptr<Event>> events;

    string subscriptionId = this->generateSubscriptionId();
    string request;

    try
    {
        request = filters->serialize(subscriptionId);
    }
    catch (const invalid_argument& e)
    {
        PLOG_ERROR << "Failed to serialize filters - invalid object: " << e.what();
        throw e;
    }
    catch (const json::exception& je)
    {
        PLOG_ERROR << "Failed to serialize filters - JSON exception: " << je.what();
        throw je;
    }

    vector<future<tuple<string, bool>>> requestFutures;

    // Send the same query to each relay.  As events trickle in from each relay, they will be added
    // to the events vector.  Multiple copies of an event may be received if the same event is
    // stored on multiple relays.  The function will block until all of the relays send an EOSE or
    // CLOSE message.
    for (const string relay : this->_activeRelays)
    {
        promise<tuple<string, bool>> eosePromise;
        requestFutures.push_back(move(eosePromise.get_future()));

        auto [uri, success] = this->_client->send(
            request,
            relay,
            [this, &relay, &events, &eosePromise](string payload)
            {
                this->onSubscriptionMessage(
                    payload,
                    [&events](const string&, shared_ptr<Event> event)
                    {
                        events.push_back(event);
                    },
                    [relay, &eosePromise](const string&)
                    {
                        eosePromise.set_value(make_tuple(relay, true));
                    },
                    [relay, &eosePromise](const string&, const string&)
                    {
                        eosePromise.set_value(make_tuple(relay, false));
                    });
            });

        if (success)
        {
            PLOG_INFO << "Sent query to relay: " << relay;
            lock_guard<mutex> lock(this->_propertyMutex);
            this->_subscriptions[relay].push_back(subscriptionId);
        }
        else
        {
            PLOG_WARNING << "Failed to send query to relay: " << relay;
            eosePromise.set_value(make_tuple(uri, false));
        }
    }

    // Close open subscriptions and disconnect from relays after events are received.
    for (auto& publishFuture : requestFutures)
    {
        auto [relay, isEose] = publishFuture.get();
        if (isEose)
        {
            PLOG_INFO << "Received EOSE message from relay: " << relay;
        }
        else
        {
            PLOG_WARNING << "Received CLOSE message from relay: " << relay;
            this->closeRelayConnections({ relay });
        }
    }
    this->closeSubscription(subscriptionId);
    this->closeRelayConnections(this->_activeRelays);

    // TODO: De-duplicate events in the vector before returning.

    return events;
};

string NostrService::queryRelays(
    shared_ptr<Filters> filters,
    function<void(const string&, shared_ptr<Event>)> eventHandler,
    function<void(const string&)> eoseHandler,
    function<void(const string&, const string&)> closeHandler)
{
    RelayList successfulRelays;
    RelayList failedRelays;

    string subscriptionId = this->generateSubscriptionId();
    string request = filters->serialize(subscriptionId);
    vector<future<tuple<string, bool>>> requestFutures;
    for (const string relay : this->_activeRelays)
    {
        unique_lock<mutex> lock(this->_propertyMutex);
        this->_subscriptions[relay].push_back(subscriptionId);
        lock.unlock();

        future<tuple<string, bool>> requestFuture = async(
            [this, &relay, &request, &eventHandler, &eoseHandler, &closeHandler]()
            {
                return this->_client->send(
                    request,
                    relay,
                    [this, &eventHandler, &eoseHandler, &closeHandler](string payload)
                    {
                        this->onSubscriptionMessage(payload, eventHandler, eoseHandler, closeHandler);
                    });
            });
        requestFutures.push_back(move(requestFuture));
    }

    for (auto& publishFuture : requestFutures)
    {
        auto [relay, isSuccess] = publishFuture.get();
        if (isSuccess)
        {
            successfulRelays.push_back(relay);
        }
        else
        {
            failedRelays.push_back(relay);
        }
    }

    size_t targetCount = this->_activeRelays.size();
    size_t successfulCount = successfulRelays.size();
    PLOG_INFO << "Sent query to " << successfulCount << "/" << targetCount << " open relay connections.";

    return subscriptionId;
};

tuple<RelayList, RelayList> NostrService::closeSubscription(string subscriptionId)
{
    RelayList successfulRelays;
    RelayList failedRelays;
    vector<future<tuple<string, bool>>> closeFutures;
    
    for (const string relay : this->_activeRelays)
    {
        if (!this->hasSubscription(relay, subscriptionId))
        {
            continue;
        }

        string request = this->generateCloseRequest(subscriptionId);
        future<tuple<string, bool>> closeFuture = async([this, relay, request]()
        {
            PLOG_INFO << "Sending " << request << " to relay " << relay;
            return this->_client->send(request, relay);
        });
        closeFutures.push_back(move(closeFuture));
    }

    for (auto& closeFuture : closeFutures)
    {
        auto [relay, isSuccess] = closeFuture.get();
        if (isSuccess)
        {
            successfulRelays.push_back(relay);

            lock_guard<mutex> lock(this->_propertyMutex);
            auto it = find(
                this->_subscriptions[relay].begin(),
                this->_subscriptions[relay].end(),
                subscriptionId);
            this->_subscriptions[relay].erase(it);
        }
        else
        {
            failedRelays.push_back(relay);
        }
    }

    size_t targetCount = this->_activeRelays.size();
    size_t successfulCount = successfulRelays.size();
    PLOG_INFO << "Sent close request to " << successfulCount << "/" << targetCount << " open relay connections.";

    return make_tuple(successfulRelays, failedRelays);
};

tuple<RelayList, RelayList> NostrService::closeSubscriptions()
{
    return this->closeSubscriptions(this->_activeRelays);
};

tuple<RelayList, RelayList> NostrService::closeSubscriptions(RelayList relays)
{
    RelayList successfulRelays;
    RelayList failedRelays;

    vector<future<tuple<RelayList, RelayList>>> closeFutures;
    for (const string relay : relays)
    {
        future<tuple<RelayList, RelayList>> closeFuture = async([this, &relay]()
        {
            RelayList successfulRelays;
            RelayList failedRelays;

            unique_lock<mutex> lock(this->_propertyMutex);
            auto subscriptionIds = this->_subscriptions[relay];
            lock.unlock();

            for (const string& subscriptionId : subscriptionIds)
            {
                auto [successes, failures] = this->closeSubscription(subscriptionId);
                successfulRelays.insert(successfulRelays.end(), successes.begin(), successes.end());
                failedRelays.insert(failedRelays.end(), failures.begin(), failures.end());
            }

            return make_tuple(successfulRelays, failedRelays);
        });
        closeFutures.push_back(move(closeFuture));
    }

    for (auto& closeFuture : closeFutures)
    {
        auto [successes, failures] = closeFuture.get();
        successfulRelays.insert(successfulRelays.end(), successes.begin(), successes.end());
        failedRelays.insert(failedRelays.end(), failures.begin(), failures.end());
    }

    size_t targetCount = relays.size();
    size_t successfulCount = successfulRelays.size();
    PLOG_INFO << "Sent close requests to " << successfulCount << "/" << targetCount << " open relay connections.";

    return make_tuple(successfulRelays, failedRelays);
};

RelayList NostrService::getConnectedRelays(RelayList relays)
{
    PLOG_VERBOSE << "Identifying connected relays.";
    RelayList connectedRelays;
    for (string relay : relays)
    {
        bool isActive = find(this->_activeRelays.begin(), this->_activeRelays.end(), relay)
            != this->_activeRelays.end();
        bool isConnected = this->_client->isConnected(relay);
        PLOG_VERBOSE << "Relay " << relay << " is active: " << isActive << ", is connected: " << isConnected;

        if (isActive && isConnected)
        {
            connectedRelays.push_back(relay);
        }
        else if (isActive && !isConnected)
        {
            this->eraseActiveRelay(relay);
        }
        else if (!isActive && isConnected)
        {
            this->_activeRelays.push_back(relay);
            connectedRelays.push_back(relay);
        }
    }
    return connectedRelays;
};

RelayList NostrService::getUnconnectedRelays(RelayList relays)
{
    PLOG_VERBOSE << "Identifying unconnected relays.";
    RelayList unconnectedRelays;
    for (string relay : relays)
    {
        bool isActive = find(this->_activeRelays.begin(), this->_activeRelays.end(), relay)
            != this->_activeRelays.end();
        bool isConnected = this->_client->isConnected(relay);
        PLOG_VERBOSE << "Relay " << relay << " is active: " << isActive << ", is connected: " << isConnected;

        if (!isActive && !isConnected)
        {
            PLOG_VERBOSE << "Relay " << relay << " is not active and not connected.";
            unconnectedRelays.push_back(relay);
        }
        else if (isActive && !isConnected)
        {
            PLOG_VERBOSE << "Relay " << relay << " is active but not connected.  Removing from active relays list.";
            this->eraseActiveRelay(relay);
            unconnectedRelays.push_back(relay);
        }
        else if (!isActive && isConnected)
        {
            PLOG_VERBOSE << "Relay " << relay << " is connected but not active.  Adding to active relays list.";
            this->_activeRelays.push_back(relay);
        }
    }
    return unconnectedRelays;
};

bool NostrService::isConnected(string relay)
{
    auto it = find(this->_activeRelays.begin(), this->_activeRelays.end(), relay);
    if (it != this->_activeRelays.end()) // If the relay is in this->_activeRelays
    {
        return true;
    }
    return false;
};

void NostrService::eraseActiveRelay(string relay)
{
    auto it = find(this->_activeRelays.begin(), this->_activeRelays.end(), relay);
    if (it != this->_activeRelays.end()) // If the relay is in this->_activeRelays
    {
        this->_activeRelays.erase(it);
    }
};

void NostrService::connect(string relay)
{
    PLOG_VERBOSE << "Connecting to relay " << relay;
    this->_client->openConnection(relay);

    lock_guard<mutex> lock(this->_propertyMutex);
    bool isConnected = this->_client->isConnected(relay);

    if (isConnected)
    {
        PLOG_VERBOSE << "Connected to relay " << relay << ": " << isConnected;
        this->_activeRelays.push_back(relay);
    }
    else
    {
        PLOG_ERROR << "Failed to connect to relay " << relay;
    }
};

void NostrService::disconnect(string relay)
{
    this->_client->closeConnection(relay);

    lock_guard<mutex> lock(this->_propertyMutex);
    this->eraseActiveRelay(relay);
};

string NostrService::generateSubscriptionId()
{
    UUIDv4::UUIDGenerator<std::mt19937_64> uuidGenerator;
    UUIDv4::UUID uuid = uuidGenerator.getUUID();
    return uuid.str();
};

string NostrService::generateCloseRequest(string subscriptionId)
{
    json jarr = json::array({ "CLOSE", subscriptionId });
    return jarr.dump();
};

bool NostrService::hasSubscription(string relay, string subscriptionId)
{
    lock_guard<mutex> lock(this->_propertyMutex);
    auto it = find(this->_subscriptions[relay].begin(), this->_subscriptions[relay].end(), subscriptionId);
    if (it != this->_subscriptions[relay].end()) // If the subscription is in this->_subscriptions[relay]
    {
        return true;
    }
    return false;
};

void NostrService::onSubscriptionMessage(
    string message,
    function<void(const string&, shared_ptr<Event>)> eventHandler,
    function<void(const string&)> eoseHandler,
    function<void(const string&, const string&)> closeHandler)
{
    try
    {
        json jMessage = json::parse(message);
        string messageType = jMessage.at(0);
        if (messageType == "EVENT")
        {
            string subscriptionId = jMessage.at(1);
            Event event = Event::fromString(jMessage.at(2));
            eventHandler(subscriptionId, make_shared<Event>(event));
        }
        else if (messageType == "EOSE")
        {
            string subscriptionId = jMessage.at(1);
            eoseHandler(subscriptionId);
        }
        else if (messageType == "CLOSE")
        {
            string subscriptionId = jMessage.at(1);
            string reason = jMessage.at(2);
            closeHandler(subscriptionId, reason);
        }
    }
    catch (const json::out_of_range& joor)
    {
        PLOG_ERROR << "JSON out-of-range exception: " << joor.what();
        throw joor;
    }
    catch (const json::exception& je)
    {
        PLOG_ERROR << "JSON handling exception: " << je.what();
        throw je;
    }
    catch (const invalid_argument& ia)
    {
        PLOG_ERROR << "Invalid argument exception: " << ia.what();
        throw ia;
    }
};

void NostrService::onAcceptance(string message, function<void(const bool)> acceptanceHandler)
{
    try
    {
        json jMessage = json::parse(message);
        string messageType = jMessage[0];
        if (messageType == "OK")
        {
            bool isAccepted = jMessage[2];
            acceptanceHandler(isAccepted);
        }
    }
    catch (const json::exception& je)
    {
        PLOG_ERROR << "JSON handling exception: " << je.what();
        throw je;
    }
};
} // namespace nostr
