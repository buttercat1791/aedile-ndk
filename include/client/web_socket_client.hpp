#pragma once

#include <string>

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
     * @brief Closes the connection to the given server.
     */
    virtual void closeConnection(std::string uri) = 0;
};
} // namespace client
