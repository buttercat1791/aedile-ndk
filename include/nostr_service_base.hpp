#pragma once

#include "nostr.hpp"

namespace nostr
{
class NostrServiceBase : public INostrServiceBase
{
public:
    NostrServiceBase(
        std::shared_ptr<plog::IAppender> appender,
        std::shared_ptr<client::IWebSocketClient> client);

    NostrServiceBase(
        std::shared_ptr<plog::IAppender> appender,
        std::shared_ptr<client::IWebSocketClient> client,
        std::vector<std::string> relays);

    ~NostrServiceBase() override;

    std::vector<std::string> defaultRelays() const;

    std::vector<std::string> activeRelays() const;

    std::unordered_map<std::string, std::vector<std::string>> subscriptions() const;

    std::vector<std::string> openRelayConnections() override;

    std::vector<std::string> openRelayConnections(std::vector<std::string> relays) override;

    void closeRelayConnections() override;

    void closeRelayConnections(std::vector<std::string> relays) override;

    // TODO: Make this method return a promise.
    std::tuple<std::vector<std::string>, std::vector<std::string>> publishEvent(
        std::shared_ptr<data::Event> event) override;

    // TODO: Make this method return a promise.
    // TODO: Add a timeout to this method to prevent hanging while waiting for the relay.
    std::vector<std::shared_ptr<data::Event>> queryRelays(
        std::shared_ptr<data::Filters> filters) override;

    std::string queryRelays(
        std::shared_ptr<data::Filters> filters,
        std::function<void(const std::string&, std::shared_ptr<data::Event>)> eventHandler,
        std::function<void(const std::string&)> eoseHandler,
        std::function<void(const std::string&, const std::string&)> closeHandler) override;

    std::tuple<std::vector<std::string>, std::vector<std::string>> closeSubscription(
        std::string subscriptionId) override;

    bool closeSubscription(std::string subscriptionId, std::string relay) override;

    std::vector<std::string> closeSubscriptions() override;

private:
    ///< The maximum number of events the service will store for each subscription.
    const int MAX_EVENTS_PER_SUBSCRIPTION = 128;

    ///< The WebSocket client used to communicate with relays.
    std::shared_ptr<client::IWebSocketClient> _client;

    ///< A mutex to protect the instance properties.
    std::mutex _propertyMutex;

    ///< The default set of Nostr relays to which the service will attempt to connect.
    std::vector<std::string> _defaultRelays;

    ///< The set of Nostr relays to which the service is currently connected.
    std::vector<std::string> _activeRelays; 
    
    ///< A map from subscription IDs to the relays on which each subscription is open.
    std::unordered_map<std::string, std::vector<std::string>> _subscriptions;

    std::vector<std::string> _getConnectedRelays(std::vector<std::string> relays);

    std::vector<std::string> _getUnconnectedRelays(std::vector<std::string> relays);

    bool _isConnected(std::string relay);

    void _eraseActiveRelay(std::string relay);

    void _connect(std::string relay);

    void _disconnect(std::string relay);

    std::string _generateSubscriptionId();

    std::string _generateCloseRequest(std::string subscriptionId);

    bool _hasSubscription(std::string subscriptionId);

    bool _hasSubscription(std::string subscriptionId, std::string relay);

    void _onSubscriptionMessage(
        std::string message,
        std::function<void(const std::string&, std::shared_ptr<data::Event>)> eventHandler,
        std::function<void(const std::string&)> eoseHandler,
        std::function<void(const std::string&, const std::string&)> closeHandler);

    void _onAcceptance(std::string message, std::function<void(const bool)> acceptanceHandler);
};
} // namespace nostr
