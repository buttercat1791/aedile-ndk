#pragma once

#include "web_socket_client.hpp"

namespace nostr
{
namespace client
{
/**
 * @brief An implementation of the `IWebSocketClient` interface that uses the WebSocket++ library.
 */
class WebsocketppClient : public IWebSocketClient
{
public:
    void start() override;

    void stop() override;

    void openConnection(std::string uri) override;

    bool isConnected(std::string uri) override;

    std::tuple<std::string, bool> send(std::string message, std::string uri) override;

    std::tuple<std::string, bool> send(
        std::string message,
        std::string uri,
        std::function<void(const std::string&)> messageHandler) override;

    void receive(std::string uri, std::function<void(const std::string&)> messageHandler) override;

    void closeConnection(std::string uri) override;

private:
    typedef websocketpp::client<websocketpp::config::asio_client> websocketpp_client;
    typedef std::unordered_map<std::string, websocketpp::connection_hdl>::iterator connection_hdl_iterator;

    websocketpp_client _client;
    std::unordered_map<std::string, websocketpp::connection_hdl> _connectionHandles;
    std::mutex _propertyMutex;
};
} // namespace client
} // namespace nostr

