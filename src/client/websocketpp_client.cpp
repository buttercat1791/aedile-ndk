#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>

#include "web_socket_client.hpp"

using std::error_code;
using std::lock_guard;
using std::make_tuple;
using std::mutex;
using std::string;
using std::tuple;
using std::unordered_map;

namespace client
{
/**
 * @brief An implementation of the `IWebSocketClient` interface that uses the WebSocket++ library.
 */
class WebsocketppClient : public IWebSocketClient
{
public:
    void start() override
    { 
        this->_client.init_asio();
        this->_client.start_perpetual();
    };

    void stop() override
    {
        this->_client.stop_perpetual();
        this->_client.stop();
    };

    void openConnection(string uri) override
    {
        error_code error;
        websocketpp_client::connection_ptr connection = this->_client.get_connection(uri, error);

        if (error.value() == -1)    
        {
            // PLOG_ERROR << "Error connecting to relay " << relay << ": " << error.message();
        }

        // Configure the connection here via the connection pointer.
        connection->set_fail_handler([this, uri](auto handle) {
            // PLOG_ERROR << "Error connecting to relay " << relay << ": Handshake failed.";
            lock_guard<mutex> lock(this->_propertyMutex);
            if (this->isConnected(uri))
            {
                this->_connectionHandles.erase(uri);
            }
        });

        lock_guard<mutex> lock(this->_propertyMutex);
        this->_connectionHandles[uri] = connection->get_handle();
        this->_client.connect(connection);
    };

    bool isConnected(string uri) override
    {
        lock_guard<mutex> lock(this->_propertyMutex);
        return this->_connectionHandles.find(uri) != this->_connectionHandles.end();
    };

    tuple<string, bool> send(string message, string uri) override
    {
        error_code error;

        // Make sure the connection isn't closed from under us.
        lock_guard<mutex> lock(this->_propertyMutex);
        this->_client.send(
            this->_connectionHandles[uri],
            message,
            websocketpp::frame::opcode::text,
            error);

        if (error.value() == -1)    
        {
            // PLOG_ERROR << "Error publishing event to relay " << relay << ": " << error.message();
            return make_tuple(uri, false);
        }

        return make_tuple(uri, true);
    };

    void closeConnection(string uri) override
    {
        lock_guard<mutex> lock(this->_propertyMutex);

        websocketpp::connection_hdl handle = this->_connectionHandles[uri];
        this->_client.close(
            handle,
            websocketpp::close::status::going_away,
            "_client requested close.");
        
        this->_connectionHandles.erase(uri);
    };

private:
    typedef websocketpp::client<websocketpp::config::asio_client> websocketpp_client;
    typedef unordered_map<string, websocketpp::connection_hdl>::iterator connection_hdl_iterator;

    websocketpp_client _client;
    unordered_map<string, websocketpp::connection_hdl> _connectionHandles;
    mutex _propertyMutex;
};
} // namespace client
