#include <plog/Init.h>
#include <plog/Log.h>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>

#include "nostr.hpp"
#include "client/web_socket_client.hpp"

using std::async;
using std::future;
using std::lock_guard;
using std::make_tuple;
using std::move;
using std::mutex;
using std::string;
using std::thread;
using std::tuple;
using std::vector;

namespace nostr
{
NostrService::NostrService(plog::IAppender* appender, client::IWebSocketClient* client)
    : NostrService(appender, client, {}) { };

NostrService::NostrService(plog::IAppender* appender, client::IWebSocketClient* client, RelayList relays)
    : _defaultRelays(relays), _client(client)
{ 
    plog::init(plog::debug, appender);
    client->start();
};

NostrService::~NostrService()
{
    this->_client->stop();
    delete this->_client;
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

tuple<RelayList, RelayList> NostrService::publishEvent(Event event)
{
    // TODO: Add validation function.

    RelayList successfulRelays;
    RelayList failedRelays;

    PLOG_INFO << "Attempting to publish event to Nostr relays.";

    vector<future<tuple<string, bool>>> publishFutures;
    for (const string& relay : this->_activeRelays)
    {
        future<tuple<string, bool>> publishFuture = async([this, &relay, &event]() {
            return this->_client->send(event.serialize(), relay);
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
} // namespace nostr
