#pragma once

#include <algorithm>
#include <functional>
#include <mutex>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>
#include <plog/Init.h>
#include <plog/Log.h>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <uuid_v4.h>

#include "client/web_socket_client.hpp"
#include "data/data.hpp"
#include "signer/signer.hpp"

namespace nostr
{
// TODO: Create custom exception types for the nostr namespace.

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
     * @returns A vector of all events matching the filters from all open relay connections.
     * @remark This method runs until the relays send an EOSE message, indicating they have no more
     * stored events matching the given filters.  When the EOSE message is received, the method
     * will close the subscription for each relay and return the received events.
     * @remark Use this method to fetch a batch of events from the relays.  A `limit` value must be
     * set on the filters in the range 1-64, inclusive.  If no valid limit is given, it will be
     * defaulted to 16.
     */
    virtual std::vector<std::shared_ptr<data::Event>> queryRelays(
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
} // namespace nostr
