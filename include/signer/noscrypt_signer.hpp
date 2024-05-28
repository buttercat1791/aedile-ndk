#pragma once

#include <plog/Init.h>
#include <plog/Log.h>

extern "C"
{
#include <noscrypt.h>
}

#include "service/nostr_service_base.hpp"
#include "signer/signer.hpp"

namespace nostr
{
namespace signer
{
class NoscryptSigner : public INostrConnectSigner
{
public:
    NoscryptSigner(
        std::shared_ptr<plog::IAppender> appender,
        std::shared_ptr<nostr::service::INostrServiceBase> nostrService);

    ~NoscryptSigner();

    void receiveConnection(std::string connectionToken) override;

    void initiateConnection(
        std::string relay,
        std::string name,
        std::string url,
        std::string description) override;

    void sign(std::shared_ptr<data::Event> event) override;

private:
    std::shared_ptr<NCContext> _noscryptContext;

    std::string _localPrivateKey;
    std::string _localPublicKey;
    
    /**
     * @brief Initializes the noscrypt library context into the class's `context` property.
     * @returns `true` if successful, `false` otherwise.
     */
    std::shared_ptr<NCContext> _initNoscryptContext();

    /**
     * @brief Generates a private/public key pair for local use.
     * @returns The generated keypair of the form `[privateKey, publicKey]`, or a pair of empty
     * strings if the function failed.
     * @remarks This keypair is intended for temporary use, and should not be saved or used outside
     * of this class.
     */
    std::tuple<std::string, std::string> _createLocalKeypair();

    #pragma region Logging

    void _logNoscryptInitResult(NCResult result);

    void _logNoscryptSecretKeyResult(NCResult result);

    void _logNoscryptPublicKeyResult(NCResult result);

    #pragma endregion
};
} // namespace signer
} // namespace nostr
