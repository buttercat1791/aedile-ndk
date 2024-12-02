#pragma once

#include "signer/signer.hpp"

#include "nostr_service_base.hpp"

#define NSERVICE_DECL template<class TAppender, class TClient, class TSigner>
#define NSERVICE_DECL2 template<class TAppender2, class TClient2, class TSigner2>
#define NSERVICE_DEF NostrService<TAppender, TClient, TSigner>
#define NSERVICE_DEF2 NostrService<TAppender2, TClient2, TSigner2>

namespace nostr
{
namespace service
{
/**
 * A template for a singleton class that implements Nostr features.
 */
NSERVICE_DECL
class NostrService
{
public:
    NostrService(NostrService &other) = delete;

    ~NostrService();

    void operator=(const NostrService &) = delete;

    /**
     * Returns a singleton instance of `NostrService`, initializing the instance if it has not
     * been initialized already.
     */
    static std::shared_ptr<NSERVICE_DEF> instance();

protected:
    NostrService(
        std::shared_ptr<TAppender> appender,
        std::shared_ptr<TClient> client,
        std::shared_ptr<TSigner> signer);

private:
    std::shared_ptr<TAppender> _appender;
    std::shared_ptr<TClient> _client;
    std::shared_ptr<INostrServiceBase> _base;
    std::shared_ptr<TSigner> _signer;

    static std::shared_ptr<NSERVICE_DEF> _instance;
    static std::mutex _mutex;
};

/**
 * The singleton instance.
 */
NSERVICE_DECL
std::shared_ptr<NSERVICE_DEF> NSERVICE_DEF::_instance = nullptr;

/**
 * Enables thread-safe access to the singleton instance.
 */
NSERVICE_DECL
std::mutex NSERVICE_DEF::_mutex;
} // namespace service
} // namespace nostr
