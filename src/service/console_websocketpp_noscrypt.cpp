#include <plog/Appenders/ConsoleAppender.h>
#include <plog/Formatters/TxtFormatter.h>

#include "client/websocketpp_client.hpp"
#include "service/nostr_service.hpp"
#include "signer/noscrypt_signer.hpp"

#define NSERVICE_CONSOLE_WEBSOCKETPP_NOSCRYPT_DEF NostrService<ConsoleAppender<TxtFormatter>, WebsocketppClient, NoscryptSigner>

using namespace nostr::client;
using namespace nostr::service;
using namespace nostr::signer;
using namespace plog;
using namespace std;

template<>
shared_ptr<NSERVICE_CONSOLE_WEBSOCKETPP_NOSCRYPT_DEF> NSERVICE_CONSOLE_WEBSOCKETPP_NOSCRYPT_DEF::instance()
{
    lock_guard<mutex> lock(NSERVICE_CONSOLE_WEBSOCKETPP_NOSCRYPT_DEF::_mutex);
    if (NSERVICE_CONSOLE_WEBSOCKETPP_NOSCRYPT_DEF::_instance == nullptr)
    {
        auto appender = make_shared<ConsoleAppender<TxtFormatter>>();
        auto client = make_shared<WebsocketppClient>();
        auto base = make_shared<NostrServiceBase>(appender, client);
        auto signer = make_shared<NoscryptSigner>(appender, base);

        NSERVICE_CONSOLE_WEBSOCKETPP_NOSCRYPT_DEF::_instance = shared_ptr<NSERVICE_CONSOLE_WEBSOCKETPP_NOSCRYPT_DEF>(
            new NSERVICE_CONSOLE_WEBSOCKETPP_NOSCRYPT_DEF(appender, client, signer));
    }

    // When there is a ready instance, return it.
    return NSERVICE_CONSOLE_WEBSOCKETPP_NOSCRYPT_DEF::_instance;
};
