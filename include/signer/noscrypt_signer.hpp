#pragma once

#include <plog/Init.h>
#include <plog/Log.h>
#include <noscrypt.h>

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

    std::shared_ptr<std::promise<bool>> sign(std::shared_ptr<data::Event> event) override;

private:
    const int _nostrConnectKind = 24133; // Kind 24133 is reserved for NIP-46 events.

    Encryption _nostrConnectEncryption;

    std::shared_ptr<NCContext> _noscryptContext;
    std::shared_ptr<nostr::service::INostrServiceBase> _nostrService;

    std::shared_ptr<NCPublicKey> _remotePubkey; // TODO: Set this when available.
    std::shared_ptr<NCSecretKey> _localSecret;

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

    /**
     * @brief Generates a unique ID for a signer request.
     * @returns A GUID string.
     */
    std::string _generateSignerRequestId();

    /**
     * @brief Builds a wrapper event for JRPC-like signer messages.
     * @param jrpc The JRPC-like payload that will comprise the event content, as specified by
     * NIP-46.
     * @returns A shared pointer to the wrapper event.
     */
    std::shared_ptr<nostr::data::Event> _wrapSignerMessage(nlohmann::json jrpc);

    /**
     * @brief Unwraps the JRPC-like payload from a signer message, typically one received from the
     * remote signer in response to a request.
     * @param event An event containing a NIP-46 message payload.
     * @returns The unwrapped payload.  The returned object will be empty if no valid payload could
     * be extracted from the given event.
     */
    std::string _unwrapSignerMessage(std::shared_ptr<nostr::data::Event> event);

    /**
     * @brief Pings the remote signer to confirm that it is online and available.
     * @returns `true` if the signer is available, `false` otherwise.
     */
    bool _pingSigner();

    #pragma region Cryptography

    /**
     * @brief Reseeds OpenSSL's pseudo-random number generator, using `/dev/random` as the seed, if
     * possible.
    */
    void _reseedRandomNumberGenerator(uint32_t bufferSize = 32);

    /**
     * @brief Encrypts a string according to the standard specified in NIP-04.
     * @param input The string to be encrypted.
     * @return The resulting encrypted string, or an empty string if the input could not be
     * encrypted.
     */
    std::string _encryptNip04(const std::string input);

    /**
     * @brief Decrypts a NIP-04 encrypted string.
     * @param input The string to be decrypted.
     * @return The decrypted string, or an empty string if the input could not be decrypted.
     */
    std::string _decryptNip04(const std::string input);

    /**
     * @brief Encrypts a string according to the standard specified in NIP-44.
     * @param input The string to be encrypted.
     * @return The resulting encrypted string, or an empty string if the input could not be
     * encrypted.
     */
    std::string _encryptNip44(const std::string input); // TODO: Return or set HMAC?

    /**
     * @brief Decrypts a NIP-44 encrypted string.
     * @param input The string to be decrypted.
     * @return The decrypted string, or an empty string if the input could not be decrypted.
     */
    std::string _decryptNip44(const std::string input);

    #pragma endregion

    #pragma region Logging

    void _logNoscryptInitResult(NCResult initResult);

    void _logNoscryptSecretValidationResult(NCResult secretValidationResult);

    void _logNoscryptPubkeyGenerationResult(NCResult pubkeyGenerationResult);

    #pragma endregion
};
} // namespace signer
} // namespace nostr
