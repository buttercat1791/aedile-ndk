#pragma once

#include <algorithm>
#include <functional>
#include <mutex>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <plog/Init.h>
#include <plog/Log.h>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <uuid_v4.h>

#include "client/web_socket_client.hpp"

namespace nostr
{
class ISigner;
class NostrService;

/**
 * @brief A Nostr event.
 * @remark All data transmitted over the Nostr protocol is encoded in JSON blobs.  This struct
 * is common to every Nostr event kind.  The significance of each event is determined by the
 * `tags` and `content` fields.
*/
struct Event
{
    std::string id; ///< SHA-256 hash of the event data.
    std::string pubkey; ///< Public key of the event creator.
    std::time_t createdAt; ///< Unix timestamp of the event creation.
    int kind; ///< Event kind.
    std::vector<std::vector<std::string>> tags; ///< Arbitrary event metadata.
    std::string content; ///< Event content.
    std::string sig; ///< Event signature created with the private key of the event creator.

    /**
     * @brief Serializes the event to a JSON object.
     * @returns A stringified JSON object representing the event.
     * @throws `std::invalid_argument` if the event object is invalid.
     */
    std::string serialize();

    /**
     * @brief Deserializes the event from a JSON string.
     * @param jsonString A stringified JSON object representing the event.
     * @returns An event instance created from the JSON string.
     */
    static Event fromString(std::string jsonString);

    /**
     * @brief Deserializes the event from a JSON object.
     * @param j A JSON object representing the event.
     * @returns An event instance created from the JSON object.
     */
    static Event fromJson(nlohmann::json j);

    /**
     * @brief Compares two events for equality.
     * @remark Two events are considered equal if they have the same ID, since the ID is uniquely
     * generated from the event data.  If the `id` field is empty for either event, the comparison
     * function will throw an exception.
     */
    bool operator==(const Event& other) const;

private:
    /**
     * @brief Validates the event.
     * @throws `std::invalid_argument` if the event object is invalid.
     * @remark The `createdAt` field defaults to the present if it is not already set.
     */
    void validate();

    /**
     * @brief Generates an ID for the event.
     * @param serializedData The serialized JSON string of all of the event data except the ID and
     * the signature.
     * @return A valid Nostr event ID.
     * @remark The ID is a 32-bytes lowercase hex-encoded sha256 of the serialized event data.
     */
    std::string generateId(std::string serializedData) const;
};

/**
 * @brief A set of filters for querying Nostr relays.
 * @remark The `limit` field should always be included to keep the response size reasonable.  The
 * `since` field is not required, and the `until` field will default to the present.  At least one
 * of the other fields must be set for a valid filter.
 */
struct Filters
{
    std::vector<std::string> ids; ///< Event IDs.
    std::vector<std::string> authors; ///< Event author npubs.
    std::vector<int> kinds; ///< Kind numbers.
    std::unordered_map<std::string, std::vector<std::string>> tags; ///< Tag names mapped to lists of tag values.
    std::time_t since; ///< Unix timestamp.  Matching events must be newer than this.
    std::time_t until; ///< Unix timestamp.  Matching events must be older than this.
    int limit; ///< The maximum number of events the relay should return on the initial query.

    /**
     * @brief Serializes the filters to a JSON object.
     * @param subscriptionId A string up to 64 chars in length that is unique per relay connection.
     * @returns A stringified JSON object representing the filters.
     * @throws `std::invalid_argument` if the filter object is invalid.
     * @remarks The Nostr client is responsible for managing subscription IDs.  Responses from the
     * relay will be organized by subscription ID.
     */
    std::string serialize(std::string& subscriptionId);

private:
    /**
     * @brief Validates the filters.
     * @throws `std::invalid_argument` if the filter object is invalid.
     * @remark The `until` field defaults to the present if it is not already set.
     */
    void validate();
};

class NostrService
{
public:
    NostrService(
        std::shared_ptr<plog::IAppender> appender,
        std::shared_ptr<client::IWebSocketClient> client,
        std::shared_ptr<ISigner> signer);
    NostrService(
        std::shared_ptr<plog::IAppender> appender,
        std::shared_ptr<client::IWebSocketClient> client,
        std::shared_ptr<ISigner> signer,
        std::vector<std::string> relays);
    ~NostrService();

    std::vector<std::string> defaultRelays() const;

    std::vector<std::string> activeRelays() const;

    std::unordered_map<std::string, std::vector<std::string>> subscriptions() const;

    /**
     * @brief Opens connections to the default Nostr relays of the instance, as specified in
     * the constructor.
     * @return A list of the relay URLs to which connections were successfully opened.
     */
    std::vector<std::string> openRelayConnections();

    /**
     * @brief Opens connections to the specified Nostr relays.
     * @returns A list of the relay URLs to which connections were successfully opened.
     */
    std::vector<std::string> openRelayConnections(std::vector<std::string> relays);

    /**
     * @brief Closes all open relay connections.
     */
    void closeRelayConnections();

    /**
     * @brief Closes any open connections to the specified Nostr relays.
     */
    void closeRelayConnections(std::vector<std::string> relays);

    /**
     * @brief Publishes a Nostr event to all open relay connections.
     * @returns A tuple of `std::vector<std::string>` objects, of the form `<successes, failures>`, indicating
     * to which relays the event was published successfully, and to which relays the event failed
     * to publish.
     */
    std::tuple<std::vector<std::string>, std::vector<std::string>> publishEvent(std::shared_ptr<Event> event);

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
    std::vector<std::shared_ptr<Event>> queryRelays(std::shared_ptr<Filters> filters);

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
    std::string queryRelays(
        std::shared_ptr<Filters> filters,
        std::function<void(const std::string&, std::shared_ptr<Event>)> eventHandler,
        std::function<void(const std::string&)> eoseHandler,
        std::function<void(const std::string&, const std::string&)> closeHandler);

    /**
     * @brief Closes the subscription with the given ID on all open relay connections.
     * @returns A tuple of `std::vector<std::string>` objects, of the form `<successes, failures>`, indicating
     * to which relays the message was sent successfully, and which relays failed to receive the
     * message.
     */
    std::tuple<std::vector<std::string>, std::vector<std::string>> closeSubscription(std::string subscriptionId);

    /**
     * @brief Closes the subscription with the given ID on the given relay.
     * @returns True if the relay received the CLOSE message, false otherwise.
     * @remark If the subscription does not exist on the given relay, or if the relay is not
     * connected, the method will do nothing and return false.
     */
    bool closeSubscription(std::string subscriptionId, std::string relay);

    /**
     * @brief Closes all open subscriptions on all open relay connections.
     * @returns A list of any subscription IDs that failed to close.
     */
    std::vector<std::string> closeSubscriptions();

private:
    ///< The maximum number of events the service will store for each subscription.
    const int MAX_EVENTS_PER_SUBSCRIPTION = 128;

    ///< The WebSocket client used to communicate with relays.
    std::shared_ptr<client::IWebSocketClient> _client;
    ///< The signer used to sign Nostr events.
    std::shared_ptr<ISigner> _signer;

    ///< A mutex to protect the instance properties.
    std::mutex _propertyMutex;
    ///< The default set of Nostr relays to which the service will attempt to connect.
    std::vector<std::string> _defaultRelays;
    ///< The set of Nostr relays to which the service is currently connected.
    std::vector<std::string> _activeRelays;
    ///< A map from subscription IDs to the relays on which each subscription is open.
    std::unordered_map<std::string, std::vector<std::string>> _subscriptions;

    /**
     * @brief Determines which of the given relays are currently connected.
     * @returns A list of the URIs of currently-open relay connections from the given list.
     */
    std::vector<std::string> getConnectedRelays(std::vector<std::string> relays);

    /**
     * @brief Determines which of the given relays are not currently connected.
     * @returns A list of the URIs of currently-unconnected relays from the given list.
     */
    std::vector<std::string> getUnconnectedRelays(std::vector<std::string> relays);

    /**
     * @brief Determines whether the given relay is currently connected.
     * @returns True if the relay is connected, false otherwise.
     */
    bool isConnected(std::string relay);

    /**
     * @brief Removes the given relay from the instance's list of active relays.
     */
    void eraseActiveRelay(std::string relay);

    /**
     * @brief Opens a connection from the client to the given relay.
     */
    void connect(std::string relay);

    /**
     * @brief Closes the connection from the client to the given relay.
     */
    void disconnect(std::string relay);

    /**
     * @brief Generates a unique subscription ID that may be used to identify event requests.
     * @returns A stringified UUID.
     */
    std::string generateSubscriptionId();

    /**
     * @brief Generates a message requesting a relay to close the subscription with the given ID.
     * @returns A stringified JSON object representing the close request.
     */
    std::string generateCloseRequest(std::string subscriptionId);

    /**
     * @brief Indicates whether the the service has an open subscription with the given ID.
     * @returns True if the service has the subscription, false otherwise.
     */
    bool hasSubscription(std::string subscriptionId);

    /**
     * @brief Indicates whether the service has an open subscription with the given ID on the given
     * relay.
     * @returns True if the subscription exists on the relay, false otherwise.
     */
    bool hasSubscription(std::string subscriptionId, std::string relay);

    /**
     * @brief Parses EVENT messages received from the relay and invokes the given event handler.
     * @param message The raw message received from the relay.
     * @param eventHandler A callable object that will be invoked with the subscription ID and the
     * payload of the event.
     * @param eoseHandler A callable object that will be invoked with the subscription ID when the
     * relay sends an EOSE message, indicating it has reached the end of stored events for the
     * given query.
     * @param closeHandler A callable object that will be invoked with the subscription ID and the
     * message sent by the relay if the subscription is ended by the relay.
     */
    void onSubscriptionMessage(
        std::string message,
        std::function<void(const std::string&, std::shared_ptr<Event>)> eventHandler,
        std::function<void(const std::string&)> eoseHandler,
        std::function<void(const std::string&, const std::string&)> closeHandler);

    /**
     * @brief Parses OK messages received from the relay and invokes the given acceptance handler.
     * @remark The OK message type is sent to indicate whether the relay has accepted an event sent
     * by the client.  Note that this is distinct from whether the message was successfully sent to
     * the relay over the WebSocket connection.
     */
    void onAcceptance(std::string message, std::function<void(const bool)> acceptanceHandler);
};

class ISigner
{
public:
    /**
     * @brief Signs the given Nostr event.
     * @param event The event to sign.
     * @remark The event's `sig` field will be updated in-place with the signature.
    */
    virtual void sign(std::shared_ptr<Event> event) = 0;
};
} // namespace nostr
