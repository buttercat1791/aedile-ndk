#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>

#include <plog/Init.h>
#include <plog/Log.h>

#include "data/data.hpp"
#include "client/web_socket_client.hpp"

namespace nostr
{
namespace service
{
class INostrServiceBase
{
public:
    virtual ~INostrServiceBase() = default;

    /**
     * @brief Opens connections to the default Nostr relays of the instance, as specified in
     * the constructor.
     * @return A list of the relay URLs to which connections were successfully opened.
     */
    virtual std::vector<std::string> openRelayConnections() = 0;

    /**
     * @brief Opens connections to the specified Nostr relays.
     * @returns A list of the relay URLs to which connections were successfully opened.
     */
    virtual std::vector<std::string> openRelayConnections(std::vector<std::string> relays) = 0;

    /**
     * @brief Closes all open relay connections.
     */
    virtual void closeRelayConnections() = 0;

    /**
     * @brief Closes any open connections to the specified Nostr relays.
     */
    virtual void closeRelayConnections(std::vector<std::string> relays) = 0;
    
    /**
     * @brief Publishes a Nostr event to all open relay connections.
     * @returns A tuple of `std::vector<std::string>` objects, of the form `<successes, failures>`, indicating
     * to which relays the event was published successfully, and to which relays the event failed
     * to publish.
     */
    virtual std::tuple<std::vector<std::string>, std::vector<std::string>> publishEvent(
        std::shared_ptr<data::Event> event) = 0;

    /**
     * @brief Queries all open relay connections for events matching the given set of filters, and
     * returns all stored matching events returned by the relays.
     * @param filters The filters to use for the query.
     * @returns A std::future that will eventually hold a vector of all events matching the filters
     * from all open relay connections.
     * @remark This method runs until the relays send an EOSE message, indicating they have no more
     * stored events matching the given filters.  When the EOSE message is received, the method
     * will close the subscription for each relay and return the received events.
     * @remark Use this method to fetch a batch of events from the relays.  A `limit` value must be
     * set on the filters in the range 1-64, inclusive.  If no valid limit is given, it will be
     * defaulted to 16.
     */
    virtual std::future<std::vector<std::shared_ptr<data::Event>>> queryRelays(
        std::shared_ptr<data::Filters> filters) = 0;

    /**
     * @brief Queries all open relay connections for events matching the given set of filters.
     * @param filters The filters to use for the query.
     * @param eventHandler A callable object that will be invoked each time the client receives
     * an event matching the filters.
     * @param eoseHandler A callable object that will be invoked when the relay sends an EOSE
     * message.
     * @param closeHandler A callable object that will be invoked when the relay sends a CLOSE
     * message.
     * @returns The ID of the subscription created for the query.
     * @remark By providing a response handler, the caller assumes responsibility for handling all
     * events returned from the relay for the given filters.  The service will not store the
     * events, and they will not be accessible via `getNewEvents`.
     */
    virtual std::string queryRelays(
        std::shared_ptr<data::Filters> filters,
        std::function<void(const std::string&, std::shared_ptr<data::Event>)> eventHandler,
        std::function<void(const std::string&)> eoseHandler,
        std::function<void(const std::string&, const std::string&)> closeHandler) = 0;
    
    /**
     * @brief Closes the subscription with the given ID on all open relay connections.
     * @returns A tuple of `std::vector<std::string>` objects, of the form `<successes, failures>`, indicating
     * to which relays the message was sent successfully, and which relays failed to receive the
     * message.
     */
    virtual std::tuple<std::vector<std::string>, std::vector<std::string>> closeSubscription(
        std::string subscriptionId) = 0;

    /**
     * @brief Closes the subscription with the given ID on the given relay.
     * @returns True if the relay received the CLOSE message, false otherwise.
     * @remark If the subscription does not exist on the given relay, or if the relay is not
     * connected, the method will do nothing and return false.
     */
    virtual bool closeSubscription(std::string subscriptionId, std::string relay) = 0;

    /**
     * @brief Closes all open subscriptions on all open relay connections.
     * @returns A list of any subscription IDs that failed to close.
     */
    virtual std::vector<std::string> closeSubscriptions() = 0;
};

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
} // namespace service
} // namespace nostr
