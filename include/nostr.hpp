#pragma once

#include <mutex>
#include <string>
#include <tuple>
#include <vector>

#include <nlohmann/json.hpp>
#include <plog/Log.h>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>

#include "client/web_socket_client.hpp"

namespace nostr
{
typedef std::vector<std::string> RelayList;

// TODO: Add null checking to seralization and deserialization methods.
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
    std::string created_at; ///< Unix timestamp of the event creation.
    int kind; ///< Event kind.
    std::vector<std::vector<std::string>> tags; ///< Arbitrary event metadata.
    std::string content; ///< Event content.
    std::string sig; ///< Event signature created with the private key of the event creator.

    nlohmann::json serialize() const;
    void deserialize(std::string jsonString);
};

class NostrService
{
public:
    NostrService(plog::IAppender* appender, client::IWebSocketClient* client);
    NostrService(plog::IAppender* appender, client::IWebSocketClient* client, RelayList relays);
    ~NostrService();

    RelayList defaultRelays() const;

    RelayList activeRelays() const;

    /**
     * @brief Opens connections to the default Nostr relays of the instance, as specified in
     * the constructor.
     * @return A list of the relay URLs to which connections were successfully opened.
     */
    RelayList openRelayConnections();

    /**
     * @brief Opens connections to the specified Nostr relays.
     * @returns A list of the relay URLs to which connections were successfully opened.
     */
    RelayList openRelayConnections(RelayList relays);

    /**
     * @brief Closes all open relay connections.
     */
    void closeRelayConnections();

    /**
     * @brief Closes any open connections to the specified Nostr relays.
     */
    void closeRelayConnections(RelayList relays);
    
    /**
     * @brief Publishes a Nostr event to all open relay connections.
     * @returns A tuple of `RelayList` objects, of the form `<successes, failures>`, indicating
     * to which relays the event was published successfully, and to which relays the event failed
     * to publish.
    */
    std::tuple<RelayList, RelayList> publishEvent(Event event);

    // TODO: Add methods for reading events from relays.

private:
    std::mutex _propertyMutex;
    RelayList _defaultRelays;
    RelayList _activeRelays;
    client::IWebSocketClient* _client;

    /**
     * @brief Determines which of the given relays are currently connected.
     * @returns A list of the URIs of currently-open relay connections from the given list.
     */
    RelayList getConnectedRelays(RelayList relays);

    /**
     * @brief Determines which of the given relays are not currently connected.
     * @returns A list of the URIs of currently-unconnected relays from the given list.
     */
    RelayList getUnconnectedRelays(RelayList relays);

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
};
} // namespace nostr
