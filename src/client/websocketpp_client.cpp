#include <mutex>

#include "client/websocketpp_client.hpp"

using namespace std;

void nostr::client::WebsocketppClient::start()
{ 
    this->_client.init_asio();
    this->_client.start_perpetual();
};

void nostr::client::WebsocketppClient::stop()
{
    this->_client.stop_perpetual();
    this->_client.stop();
};

void nostr::client::WebsocketppClient::openConnection(string uri)
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

bool nostr::client::WebsocketppClient::isConnected(string uri)
{
    lock_guard<mutex> lock(this->_propertyMutex);
    return this->_connectionHandles.find(uri) != this->_connectionHandles.end();
};

tuple<string, bool> nostr::client::WebsocketppClient::send(string message, string uri)
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
        return make_tuple(uri, false);
    }

    return make_tuple(uri, true);
};

tuple<string, bool> nostr::client::WebsocketppClient::send(
    string message,
    string uri,
    function<void(const string&)> messageHandler)
{
    auto successes = this->send(message, uri);
    this->receive(uri, messageHandler);
    return successes;
};

void nostr::client::WebsocketppClient::receive(
    string uri,
    function<void(const string&)> messageHandler)
{
    lock_guard<mutex> lock(this->_propertyMutex);
    auto connectionHandle = this->_connectionHandles[uri];
    auto connection = this->_client.get_con_from_hdl(connectionHandle);

    connection->set_message_handler([messageHandler](
        websocketpp::connection_hdl connectionHandle,
        websocketpp_client::message_ptr message)
    {
        messageHandler(message->get_payload());
    });
};

void nostr::client::WebsocketppClient::closeConnection(string uri)
{
    lock_guard<mutex> lock(this->_propertyMutex);

    websocketpp::connection_hdl handle = this->_connectionHandles[uri];
    this->_client.close(
        handle,
        websocketpp::close::status::going_away,
        "_client requested close.");
    
    this->_connectionHandles.erase(uri);
};
