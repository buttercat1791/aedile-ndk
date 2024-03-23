#include <algorithm>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <nlohmann/json.hpp>
#include <plog/Init.h>
#include <plog/Log.h>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>

#include "nostr.hpp"
#include "client/web_socket_client.hpp"

using boost::uuids::random_generator;
using boost::uuids::to_string;
using boost::uuids::uuid;
using nlohmann::json;
using std::async;
using std::find_if;
using std::function;
using std::future;
using std::lock_guard;
using std::make_tuple;
using std::move;
using std::mutex;
using std::out_of_range;
using std::shared_ptr;
using std::string;
using std::thread;
using std::tuple;
using std::unique_ptr;
using std::vector;

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
    }

    for (thread& disconnectionThread : disconnectionThreads)
    {
        disconnectionThread.join();
    }
};

tuple<RelayList, RelayList> NostrService::publishEvent(shared_ptr<Event> event)
{
    RelayList successfulRelays;
    RelayList failedRelays;

    PLOG_INFO << "Attempting to publish event to Nostr relays.";

    string serializedEvent;
    try
    {
        this->_signer->sign(event);
        serializedEvent = event->serialize();
    }
    catch (const std::invalid_argument& error)
    {
        PLOG_ERROR << "Failed to sign event: " << error.what();
        throw error;
    }

    lock_guard<mutex> lock(this->_propertyMutex);
    vector<future<tuple<string, bool>>> publishFutures;
    for (const string& relay : this->_activeRelays)
    {
        PLOG_INFO << "Entering lambda.";
        future<tuple<string, bool>> publishFuture = async([this, relay, serializedEvent]() {
            return this->_client->send(serializedEvent, relay);
        });
        publishFutures.push_back(move(publishFuture));
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

    size_t targetCount = this->_activeRelays.size();
    size_t successfulCount = successfulRelays.size();
    PLOG_INFO << "Published event to " << successfulCount << "/" << targetCount << " target relays.";

    return make_tuple(successfulRelays, failedRelays);
};

string NostrService::queryRelays(Filters filters)
{
    return this->queryRelays(filters, [this](string subscriptionId, Event event) {
        lock_guard<mutex> lock(this->_propertyMutex);
        this->_lastRead[subscriptionId] = event.id;
        this->onEvent(subscriptionId, event);
    });
};

string NostrService::queryRelays(Filters filters, function<void(const string&, Event)> responseHandler)
{
    RelayList successfulRelays;
    RelayList failedRelays;

    string subscriptionId = this->generateSubscriptionId();
    vector<future<tuple<string, bool>>> requestFutures;
    for (const string relay : this->_activeRelays)
    {
        lock_guard<mutex> lock(this->_propertyMutex);
        this->_subscriptions[relay].push_back(subscriptionId);
        string request = filters.serialize(subscriptionId);

        future<tuple<string, bool>> requestFuture = async([this, &relay, &request]() {
            return this->_client->send(request, relay);
        });
        requestFutures.push_back(move(requestFuture));

        this->_client->receive(relay, [this, responseHandler](string payload) {
            this->onMessage(payload, responseHandler);
        });
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

vector<Event> NostrService::getNewEvents()
{
    vector<Event> newEvents;

    for (auto& [subscriptionId, events] : this->_events)
    {
        vector<Event> subscriptionEvents = this->getNewEvents(subscriptionId);
        newEvents.insert(newEvents.end(), subscriptionEvents.begin(), subscriptionEvents.end());
    }

    return newEvents;
};

vector<Event> NostrService::getNewEvents(string subscriptionId)
{
    if (this->_events.find(subscriptionId) == this->_events.end())
    {
        PLOG_ERROR << "No events found for subscription: " << subscriptionId;
        throw out_of_range("No events found for subscription: " + subscriptionId);
    }

    if (this->_lastRead.find(subscriptionId) == this->_lastRead.end())
    {
        PLOG_ERROR << "No last read event ID found for subscription: " << subscriptionId;
        throw out_of_range("No last read event ID found for subscription: " + subscriptionId);
    }

    lock_guard<mutex> lock(this->_propertyMutex);
    vector<Event> newEvents;
    vector<Event> receivedEvents = this->_events[subscriptionId];
    vector<Event>::iterator eventIt = find_if(
        receivedEvents.begin(),
        receivedEvents.end(), 
        [this,subscriptionId](Event event) {
            return event.id == this->_lastRead[subscriptionId];
        }) + 1;

    while (eventIt != receivedEvents.end())
    {
        newEvents.push_back(move(*eventIt));
        eventIt++;
    }

    return newEvents;
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
        future<tuple<string, bool>> closeFuture = async([this, &relay, &request]() {
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
        future<tuple<RelayList, RelayList>> closeFuture = async([this, &relay]() {
            RelayList successfulRelays;
            RelayList failedRelays;

            for (const string& subscriptionId : this->_subscriptions[relay])
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
    uuid uuid = random_generator()();
    return to_string(uuid);
};

string NostrService::generateCloseRequest(string subscriptionId)
{
    json jarr = json::array({ "CLOSE", subscriptionId });
    return jarr.dump();
};

bool NostrService::hasSubscription(string relay, string subscriptionId)
{
    auto it = find(this->_subscriptions[relay].begin(), this->_subscriptions[relay].end(), subscriptionId);
    if (it != this->_subscriptions[relay].end()) // If the subscription is in this->_subscriptions[relay]
    {
        return true;
    }
    return false;
};

void NostrService::onMessage(string message, function<void(const string&, Event)> eventHandler)
{
    json jarr = json::array();
    jarr = json::parse(message);

    string messageType = jarr[0];

    if (messageType == "EVENT")
    {
        string subscriptionId = jarr[1];
        string serializedEvent = jarr[2].dump();
        Event event;
        event.deserialize(message);
        eventHandler(subscriptionId, event);
    }

    // Support other message types here, if necessary.
};

void NostrService::onEvent(string subscriptionId, Event event)
{
    lock_guard<mutex> lock(this->_propertyMutex);
    this->_events[subscriptionId].push_back(event);
    PLOG_INFO << "Received event for subscription: " << subscriptionId;

    // To protect memory, only keep a limited number of events per subscription.
    while (this->_events[subscriptionId].size() > NostrService::MAX_EVENTS_PER_SUBSCRIPTION)
    {
        auto events = this->_events[subscriptionId];
        auto eventIt = find_if(
            events.begin(),
            events.end(),
            [this, subscriptionId](Event event) {
                return event.id == this->_lastRead[subscriptionId];
            });

        if (eventIt == events.begin())
        {
            eventIt++;
        }

        this->_lastRead[subscriptionId] = eventIt->id;
        _events[subscriptionId].erase(_events[subscriptionId].begin());
    }
};
} // namespace nostr
