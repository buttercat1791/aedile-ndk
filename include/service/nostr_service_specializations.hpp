#pragma once

#include <plog/Appenders/ConsoleAppender.h>
#include <plog/Formatters/TxtFormatter.h>

#include "client/websocketpp_client.hpp"
#include "signer/noscrypt_signer.hpp"

#include "nostr_service.hpp"

using CONSOLE_WEBSOCKETPP_NOSCRYPT = NostrService<
    plog::ConsoleAppender<plog::TxtFormatter>,
    client::WebsocketppClient,
    signer::NoscryptSigner
>;

namespace nostr
{
namespace service
{
/**
 * Specialization of the NostrService class template that appends logs to the console, handles
 * WebSocket connections using the Websocket++ library, and signs events using noscrypt.
 */
extern template class NostrService<plog::ConsoleAppender<plog::TxtFormatter>, client::WebsocketppClient, signer::NoscryptSigner>;

template<>
shared_ptr<CONSOLE_WEBSOCKETPP_NOSCRYPT> CONSOLE_WEBSOCKETPP_NOSCRYPT::instance()
{
    lock_guard<mutex> lock(CONSOLE_WEBSOCKETPP_NOSCRYPT::_mutex);
    if (CONSOLE_WEBSOCKETPP_NOSCRYPT::_instance == nullptr)
    {
        auto appender = make_shared<plog::ConsoleAppender<plog::TxtFormatter>>();
        auto client = make_shared<client::WebsocketppClient>();
        auto base = make_shared<NostrServiceBase>(appender, client);
        auto signer = make_shared<signer::NoscryptSigner>(appender, base);

        CONSOLE_WEBSOCKETPP_NOSCRYPT::_instance = shared_ptr<CONSOLE_WEBSOCKETPP_NOSCRYPT>(
            new CONSOLE_WEBSOCKETPP_NOSCRYPT(appender, client, signer));
    }

    // When there is a ready instance, return it.
    return CONSOLE_WEBSOCKETPP_NOSCRYPT::_instance;
};
} // namespace service
} // namespace nostr
