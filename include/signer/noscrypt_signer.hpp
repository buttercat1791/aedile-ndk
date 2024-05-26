#pragma once

extern "C"
{
#include <noscrypt.h>
}

#include "signer.hpp"

namespace nostr
{
namespace signer
{
class NoscryptSigner : public INostrConnectSigner
{
public:
    NoscryptSigner(std::shared_ptr<plog::IAppender> appender);

    ~NoscryptSigner();

    void receiveConnection(std::string connectionToken) override;

    void initiateConnection(
        std::string relay,
        std::string name,
        std::string url,
        std::string description) override;

    void sign(std::shared_ptr<data::Event> event) override;

private:
    std::shared_ptr<NCContext> noscryptContext;

    std::string localPrivateKey;
    std::string localPublicKey;
    
    /**
     * @brief Initializes the noscrypt library context into the class's `context` property.
     * @returns `true` if successful, `false` otherwise.
     */
    std::shared_ptr<NCContext> _initNoscryptContext();

    void _logNoscryptResult(NCResult result);

    /**
     * @brief Generates a private/public key pair for local use.
     * @returns The generated keypair of the form `[privateKey, publicKey]`, or a pair of empty
     * strings if the function failed.
     * @remarks This keypair is intended for temporary use, and should not be saved or used outside
     * of this class.
     */
    std::tuple<std::string, std::string> _createLocalKeypair();
};
} // namespace signer
} // namespace nostr
