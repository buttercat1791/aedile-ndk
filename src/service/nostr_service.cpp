#include <plog/Appenders/ConsoleAppender.h>
#include <plog/Formatters/TxtFormatter.h>

#include "client/websocketpp_client.hpp"
#include "service/nostr_service.hpp"
#include "signer/noscrypt_signer.hpp"

using namespace nostr::client;
using namespace nostr::service;
using namespace nostr::signer;
using namespace std;

NostrService::~NostrService()
{
    // Delete items in the singleton instance in reverse order relative to how they were
    // constructed.
    _instance->_signer.reset();
    _instance->_base.reset();
    _instance->_client.reset();
    _instance->_appender.reset();
};

NostrService NostrService::instance()
{
    // If the instance doesn't exist, initialize it.
    if (NostrService::_instance != nullptr)
    {
        _instance = make_shared<NostrService>();

        // TODO: Support different logging configurations.
        _instance->_appender = make_shared<plog::ConsoleAppender<plog::TxtFormatter>>();
        _instance->_client = make_shared<WebsocketppClient>();
        
        _instance->_base = make_shared<NostrServiceBase>(
            _instance->_appender,
            _instance->_client
        );

        _instance->_signer = make_shared<NoscryptSigner>(
            _instance->_appender,
            _instance->_base
        );
    }

    // When there is a ready instance, return it.
    return *NostrService::_instance;
};
