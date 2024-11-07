#pragma once

#include <memory>

#include "signer/signer.hpp"

#include "nostr_service_base.hpp"

namespace nostr
{
namespace service
{
class NostrService
{
// TODO: Use inheritance and/or factories to support alternate logger, signer, and client configs.
public:
    ~NostrService();

    /**
     * Returns a singleton instance of `NostrService`, initializing the instance if it has not
     * been initialized already.
     */
    static NostrService instance();

private:
    /**
     * The static NostrService instance used as a singleton.
     */
    static std::shared_ptr<NostrService> _instance;

    std::shared_ptr<plog::IAppender> _appender;
    std::shared_ptr<client::IWebSocketClient> _client;
    std::shared_ptr<INostrServiceBase> _base;
    std::shared_ptr<signer::ISigner> _signer;
};
} // namespace service
} // namespace nostr
