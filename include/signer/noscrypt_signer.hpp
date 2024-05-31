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

    std::string initiateConnection(
        std::vector<std::string> relays,
        std::string name,
        std::string url,
        std::string description) override;

    void sign(std::shared_ptr<data::Event> event) override;

private:
    std::shared_ptr<NCContext> _noscryptContext;

    std::string _localPrivateKey;
    std::string _localPublicKey;

    std::string _remotePublicKey;
    std::string _bunkerSecret;

    ///< A list of relays that will be used to connect to the remote signer.
    std::vector<std::string> _relays;
    
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

    /**
     * @brief Parses the remote signer npub from a connection token provided by the signer.
     * @param connectionToken A connection token beginning with `bunker://`.
     * @returns The index of the first character of the connection token's query string, or -1 if
     * no valid public key could be parsed.
     * @remark This function updates the `_remotePublicKey` string in its class instance by side
     * effect.
     */
    int _parseRemotePublicKey(std::string connectionToken);

    /**
     * @brief Parses a single query param from a connection token provided by a remote signer.
     * @param param A single query param from the connection token of the form `key=value`.
     * @remark This function updates the `_relays` vector and the `_bunkerSecret` string in its
     * class instance by side effect.
     */
    void _handleConnectionTokenParam(std::string param);

    #pragma region Logging

    void _logNoscryptInitResult(NCResult initResult);

    void _logNoscryptSecretValidationResult(NCResult secretValidationResult);

    void _logNoscryptPubkeyGenerationResult(NCResult pubkeyGenerationResult);

    #pragma endregion
};
} // namespace signer
} // namespace nostr
