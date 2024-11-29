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
        std::shared_ptr<nostr::service::INostrServiceBase> nostrService
    );

    ~NoscryptSigner();

    void receiveConnection(std::string connectionToken) override;

    std::string initiateConnection(
        std::vector<std::string> relays,
        std::string name,
        std::string url,
        std::string description
    ) override;

    std::shared_ptr<std::promise<bool>> sign(std::shared_ptr<data::Event> event) override;

private:
    static constexpr int _nostrConnectKind = 24133; // Kind 24133 is reserved for NIP-46 events.

    Encryption _nostrConnectEncryption;

    std::shared_ptr<NCContext> _noscryptContext;
    std::shared_ptr<nostr::service::INostrServiceBase> _nostrService;

    ///< Local nsec for communicating with the remote signer.
    std::shared_ptr<NCSecretKey> _localPrivateKey;

    ///< Local npub for communicating with the remote signer.
    std::shared_ptr<NCPublicKey> _localPublicKey;

    ///< The npub on whose behalf the remote signer is acting.
    std::shared_ptr<NCPublicKey> _remotePublicKey;

    ///< An optional secret value provided by the remote signer.
    std::string _bunkerSecret;

    ///< A list of relays that will be used to connect to the remote signer.
    std::vector<std::string> _relays;

    #pragma region Private Accessors

    inline std::string _getLocalPrivateKey() const;

    inline void _setLocalPrivateKey(const std::string value);

    inline std::string _getLocalPublicKey() const;

    inline void _setLocalPublicKey(const std::string value);

    inline std::string _getRemotePublicKey() const;

    inline void _setRemotePublicKey(const std::string value);

    #pragma endregion

    #pragma region Setup
    
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

    #pragma endregion

    #pragma region Signer Helpers

    /**
     * @brief Generates a unique ID for a signer request.
     * @returns A GUID string.
     */
    inline std::string _generateSignerRequestId() const;

    /**
     * @brief Builds and signs a wrapper event for JRPC-like signer messages.
     * @param jrpc The JRPC-like payload that will comprise the event content, as specified by
     * NIP-46.
     * @returns A shared pointer to the signed wrapper event.
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
     * @brief Constructs a filter set that queries for messages sent from the signer to the client.
     * @returns A shared pointer to the constructed filter set.
     */
    inline std::shared_ptr<nostr::data::Filters> _buildSignerMessageFilters() const;

    /**
     * @brief Pings the remote signer to confirm that it is online and available.
     * @returns A promise that will be set to `true` if the signer is available, `false` otherwise.
     */
    std::promise<bool> _pingSigner();

    #pragma endregion

    #pragma region Cryptography

    /**
     * @brief Encrypts a string according to the standard specified in NIP-04.
     * @param input The string to be encrypted.
     * @return The resulting encrypted string, or an empty string if the input could not be
     * encrypted.
     */
    std::string _encryptNip04(std::string input);

    /**
     * @brief Decrypts a NIP-04 encrypted string.
     * @param input The string to be decrypted.
     * @return The decrypted string, or an empty string if the input could not be decrypted.
     */
    std::string _decryptNip04(std::string input);

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
};
} // namespace signer
} // namespace nostr
