#pragma once

#include <functional>
#include <string>

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>

namespace nostr
{
namespace client
{
/**
 * @brief An interface for a WebSocket client singleton.
 */
class IWebSocketClient
{
public:
    /**
     * @brief Starts the client.
     * @remark This method must be called before any other client methods.
     */
    virtual void start() = 0;

    /**
     * @brief Stops the client.
     * @remark This method should be called when the client is no longer needed, before it is
     * destroyed.
     */
    virtual void stop() = 0;

    /**
     * @brief Opens a connection to the given server.
     */
    virtual void openConnection(std::string uri) = 0;

    /**
     * @brief Indicates whether the client is connected to the given server.
     * @returns True if the client is connected, false otherwise.
     */
    virtual bool isConnected(std::string uri) = 0;

    /**
     * @brief Sends the given message to the given server.
     * @returns A tuple indicating the server URI and whether the message was successfully
     * sent.
     */
    virtual std::tuple<std::string, bool> send(std::string message, std::string uri) = 0;

    /**
     * @brief Sends the given message to the given server and sets up a message handler for
     * messages received from the server.
     * @returns A tuple indicating the server URI and whether the message was successfully
     * sent.
     * @remark Use this method to send a message and set up a message handler for responses in the
     * same call.
     */
    virtual std::tuple<std::string, bool> send(
        std::string message,
        std::string uri,
        std::function<void(const std::string&)> messageHandler) = 0;

    /**
     * @brief Sets up a message handler for the given server.
     * @param uri The URI of the server to which the message handler should be attached.
     * @param messageHandler A callable object that will be invoked with the payload the client
     * receives from the server.
     */
    virtual void receive(std::string uri, std::function<void(const std::string&)> messageHandler) = 0;

    /**
     * @brief Closes the connection to the given server.
     */
    virtual void closeConnection(std::string uri) = 0;
};
} // namespace client
} // namespace nostr
