#include <algorithm>
#include <chrono>
#include <cstring>
#include <memory>
#include <random>
#include <sstream>
#include <tuple>

#include <nlohmann/json.hpp>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <uuid_v4.h>

#include "signer/noscrypt_signer.hpp"

using namespace nostr::data;
using namespace nostr::service;
using namespace nostr::signer;
using namespace std;

#pragma region Constructors and Destructors

NoscryptSigner::NoscryptSigner(
    shared_ptr<plog::IAppender> appender,
    shared_ptr<INostrServiceBase> nostrService)
{
    plog::init(plog::debug, appender.get());

    this->_reseedRandomNumberGenerator();
    this->_initNoscryptContext();
    this->_createLocalKeypair();

    this->_nostrService = nostrService;
};

NoscryptSigner::~NoscryptSigner()
{
    NCDestroyContext(this->_noscryptContext.get());
};

#pragma endregion

#pragma region Public Interface

void NoscryptSigner::receiveConnection(string connectionToken)
{
    if (connectionToken.empty())
    {
        PLOG_ERROR << "No connection token was provided - unable to connect to a remote signer.";
        return;
    }
    
    int queryStart = this->_parseRemotePublicKey(connectionToken);
    if (queryStart == -1)
    {
        return;
    }

    string remainingToken = connectionToken.substr(queryStart);
    int splitIndex = remainingToken.find('&');
    do
    {
        string param = remainingToken.substr(0, splitIndex);
        this->_handleConnectionTokenParam(param);

        remainingToken = remainingToken.substr(splitIndex + 1);
        splitIndex = remainingToken.find('&');
    } while (splitIndex != string::npos);

    // TODO: Handle any messaging with the remote signer.
};

string NoscryptSigner::initiateConnection(
    vector<string> relays,
    string name,
    string url,
    string description)
{
    // Return an empty string if the local keypair is invalid.
    if (this->_getLocalPrivateKey().empty() || this->_getLocalPublicKey().empty())
    {
        PLOG_ERROR << "A valid local keypair is required to connect to a remote signer.";
        return string();
    }

    // Return an empty string if no relays are provided.
    if (relays.empty())
    {
        PLOG_ERROR << "At least one relay must be provided to connect to a remote signer.";
        return string();
    }

    // Store the provided relay list to be used for sending and receving connection events.
    this->_relays = relays;

    // Generate the connection token.
    stringstream ss;
    ss << "nostrconnect://" << this->_localPublicKey;
    for (int i = 0; i < relays.size(); i++)
    {
        ss << (i == 0 ? "?" : "&");
        ss << "relay=" << relays[i];
    }
    ss << "&metadata={";
    ss << "\"name\":\"" << name << "\",";
    ss << "\"url\":\"" << url << "\",";
    ss << "\"description\":\"" << description << "\"";
    ss << "}";

    return ss.str();

    // TODO: Handle any messaging with the remote signer.
};

shared_ptr<promise<bool>> NoscryptSigner::sign(shared_ptr<Event> event)
{
    auto signingPromise = make_shared<promise<bool>>();

    auto signerAvailable = this->_pingSigner().get_future();
    if (signerAvailable.get() == false)
    {
        PLOG_ERROR << "Ping to the remote signer failed - the remote signer may be unavailable.";
        signingPromise->set_value(false);
        return signingPromise;
    }

    // Create the JSON-RPC-like message content.
    auto params = nlohmann::json::array();
    params.push_back(event->serialize());

    auto requestId = this->_generateSignerRequestId();

    // Create a filter set to find events from the remote signer.
    auto remoteSignerFilters = this->_buildSignerMessageFilters();

    // Generate the signing request event.
    nlohmann::json jrpc = {
        { "id", requestId },
        { "method", "sign_event" },
        { "params", params }
    };
    auto signingRequest = this->_wrapSignerMessage(jrpc);

    // Send the signing request.
    this->_nostrService->publishEvent(signingRequest);

    // Wait for the remote signer's response.
    this->_nostrService->queryRelays(
        remoteSignerFilters,
        [this, &event, &signingPromise](const string&, shared_ptr<Event> signerEvent)
        {
            // Assign the response event to the `event` parameter, accomplishing the intended
            // function result via side effect.
            string signerResponse = this->_unwrapSignerMessage(signerEvent);
            event = make_shared<Event>(Event::fromString(signerResponse));
            signingPromise->set_value(true);
        },
        [&signingPromise](const string&)
        {
            signingPromise->set_value(false);
        },
        [&signingPromise](const string&, const string&)
        {
            signingPromise->set_value(false);
        });
    
    return signingPromise;
};

#pragma endregion

#pragma region Private Accessors

inline string NoscryptSigner::_getLocalPrivateKey() const
{
    stringstream privkeyStream;
    for (int i = 0; i < sizeof(NCSecretKey); i++)
    {
        privkeyStream << hex << setw(2) << setfill('0') << static_cast<int>(this->_localPrivateKey->key[i]);
    }

    return privkeyStream.str();
};

inline void NoscryptSigner::_setLocalPrivateKey(const string value)
{
    auto seckeyBuf = reinterpret_cast<uint8_t*>(const_cast<char*>(value.data()));
    auto seckey = make_unique<NCSecretKey>();
    memcpy(seckey->key, seckeyBuf, sizeof(NCSecretKey));
    
    this->_localPrivateKey = move(seckey);
};

inline string NoscryptSigner::_getLocalPublicKey() const
{
    stringstream pubkeyStream;
    for (int i = 0; i < sizeof(NCPublicKey); i++)
    {
        pubkeyStream << hex << setw(2) << setfill('0') << static_cast<int>(this->_localPublicKey->key[i]);
    }

    return pubkeyStream.str();
};

inline void NoscryptSigner::_setLocalPublicKey(const string value)
{
    auto pubkeyBuf = reinterpret_cast<uint8_t*>(const_cast<char*>(value.data()));
    auto pubkey = make_unique<NCPublicKey>();
    memcpy(pubkey->key, pubkeyBuf, sizeof(NCPublicKey));
    
    this->_localPublicKey = move(pubkey);
};

inline string NoscryptSigner::_getRemotePublicKey() const
{
    stringstream pubkeyStream;
    for (int i = 0; i < sizeof(NCPublicKey); i++)
    {
        pubkeyStream << hex << setw(2) << setfill('0') << static_cast<int>(this->_remotePublicKey->key[i]);
    }

    return pubkeyStream.str();
};

inline void NoscryptSigner::_setRemotePublicKey(const string value)
{
    auto pubkeyBuf = reinterpret_cast<uint8_t*>(const_cast<char*>(value.data()));
    auto pubkey = make_unique<NCPublicKey>();
    memcpy(pubkey->key, pubkeyBuf, sizeof(NCPublicKey));
    
    this->_remotePublicKey = move(pubkey);
};

#pragma endregion

#pragma region Setup

void NoscryptSigner::_initNoscryptContext()
{
    shared_ptr<NCContext> context;
    auto contextStructSize = NCGetContextStructSize();
    auto randomEntropy = make_unique<uint8_t>(contextStructSize);

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(0, contextStructSize);
    generate_n(randomEntropy.get(), contextStructSize, [&]() { return dist(gen); });

    NCResult initResult = NCInitContext(context.get(), randomEntropy.get());
    this->_logNoscryptInitResult(initResult);

    this->_noscryptContext = move(context);
};

/**
 * @brief Generates a private/public key pair for local use.
 * @returns The generated keypair of the form `[privateKey, publicKey]`, or a pair of empty
 * strings if the function failed.
 * @remarks This keypair is intended for temporary use, and should not be saved or used outside
 * of this class.
 */
void NoscryptSigner::_createLocalKeypair()
{
    string privateKey;
    string publicKey;

    // To generate a private key, all we need is a random 32-bit buffer.
    auto secret = make_unique<NCSecretKey>();

    // Loop attempts to generate a secret key until a valid key is produced.
    // Limit the number of attempts to prevent resource exhaustion in the event of a failure.
    NCResult secretValidationResult;
    int loopCount = 0;
    do
    {
        int rc = RAND_bytes(secret->key, sizeof(NCSecretKey));
        if (rc != 1)
        {
            unsigned long err = ERR_get_error();
            PLOG_ERROR << "OpenSSL error " << err << " occurred while generating a secret key.";
        }

        secretValidationResult = NCValidateSecretKey(this->_noscryptContext.get(), secret.get());
    } while (secretValidationResult != NC_SUCCESS && ++loopCount < 64);

    this->_logNoscryptSecretValidationResult(secretValidationResult);
    this->_localPrivateKey = move(secret);

    // Use noscrypt to derive the public key from its private counterpart.
    auto pubkey = make_unique<NCPublicKey>();
    NCResult pubkeyGenerationResult = NCGetPublicKey(
        this->_noscryptContext.get(),
        secret.get(),
        pubkey.get());
    this->_logNoscryptPubkeyGenerationResult(pubkeyGenerationResult);
    this->_localPublicKey = move(pubkey);
};

int NoscryptSigner::_parseRemotePublicKey(string connectionToken)
{
    int queryStart = connectionToken.find('?');
    if (queryStart == string::npos)
    {
        PLOG_ERROR << "The connection token is invalid - no query string was found.";
        return -1;
    }

    const int pubkeyStart = 9;
    string prefix = connectionToken.substr(0, pubkeyStart);
    if (prefix != "bunker://")
    {
        PLOG_ERROR << "The connection token is invalid - the token must begin with 'bunker://'.";
        return -1;
    }

    string remotePubkey = connectionToken.substr(pubkeyStart, queryStart);
    this->_setRemotePublicKey(remotePubkey);

    return queryStart + 1;
};

void NoscryptSigner::_handleConnectionTokenParam(string param)
{
    // Parse the query param into a key-value pair.
    int splitIndex = param.find('=');
    if (splitIndex == string::npos)
    {
        PLOG_ERROR << "The connection token query param is invalid - it is not of the form 'key=value'.";
        return;
    }

    string key = param.substr(0, splitIndex);
    string value = param.substr(splitIndex + 1);

    // Handle the key-value pair.
    if (key == "relay")
    {
        this->_relays.push_back(value);
    }
    else if (key == "secret")
    {
        this->_bunkerSecret = value;
    }
};

#pragma endregion

#pragma region Signer Helpers

inline string NoscryptSigner::_generateSignerRequestId() const
{
    UUIDv4::UUIDGenerator<std::mt19937_64> uuidGenerator;
    UUIDv4::UUID uuid = uuidGenerator.getUUID();
    return uuid.str();
};

shared_ptr<Event> NoscryptSigner::_wrapSignerMessage(nlohmann::json jrpc)
{
    // Encrypt the message payload.
    string encryptedContent;
    switch (this->_nostrConnectEncryption)
    {
    case Encryption::NIP44:
        encryptedContent = this->_encryptNip44(jrpc.dump());
        if (!encryptedContent.empty())
        {
            break;
        }

    // Use NIP-04 encryption as a fallback.
    case Encryption::NIP04:
        encryptedContent = this->_encryptNip04(jrpc.dump());
        break;
    }

    // Wrap the event to be signed in a signing request event.
    auto wrapperEvent = make_shared<Event>();
    wrapperEvent->pubkey = this->_getLocalPublicKey();
    wrapperEvent->kind = this->_nostrConnectKind;
    wrapperEvent->tags.push_back({ "p", this->_getRemotePublicKey() });
    wrapperEvent->content = encryptedContent;

    // Generate a random seed for the signer.
    auto random32 = make_shared<uint8_t>(32);
    int code = RAND_bytes(random32.get(), 32);
    if (code <= 0)
    {
        PLOG_ERROR << "Failed to generate a random 32-byte seed buffer for the signer.";
        return nullptr;
    }

    // Sign the wrapper message with the local secret key.
    string serializedEvent = wrapperEvent->serialize();
    uint32_t dataSize = serializedEvent.length();
    auto signature = make_unique<uint8_t>(64);
    NCResult signatureResult = NCSignData(
        this->_noscryptContext.get(),
        this->_localPrivateKey.get(),
        random32.get(),
        reinterpret_cast<uint8_t*>(serializedEvent.data()),
        dataSize,
        signature.get());

    // TODO: Handle result codes.
    if (signatureResult != NC_SUCCESS)
    {
        return nullptr;
    }

    // Add the signature to the event.
    wrapperEvent->sig = string((char*)signature.get(), 64);

    return wrapperEvent;
};

string NoscryptSigner::_unwrapSignerMessage(shared_ptr<Event> event)
{
    // TODO: Verify the incoming event.

    // Extract and decrypt the event payload.
    string encryptedContent = event->content;
    string decryptedContent;

    // NIP-04 encrypted strings include `?iv=` near the end (source: hodlbod).
    if (encryptedContent.find("?iv=") != string::npos)
    {
        decryptedContent = this->_decryptNip04(encryptedContent);
    }
    else
    {
        decryptedContent = this->_decryptNip44(encryptedContent);
    }

    // Parse the decrypted string into a JSON object.
    return decryptedContent;
};

inline shared_ptr<Filters> NoscryptSigner::_buildSignerMessageFilters() const
{
    auto filters = make_shared<Filters>();
    filters->authors.push_back(this->_getRemotePublicKey());
    filters->kinds.push_back(this->_nostrConnectKind);
    filters->tags["p"] = { this->_getLocalPublicKey() };
    filters->since = time(nullptr);

    return filters;
};

promise<bool> NoscryptSigner::_pingSigner()
{
    promise<bool> pingPromise;

    // Generate a ping message and wrap it for the signer.
    nlohmann::json jrpc =
    {
        { "id", this->_generateSignerRequestId() },
        { "method", "ping" },
        { "params", nlohmann::json::array() }
    };
    auto messageEvent = this->_wrapSignerMessage(jrpc);

    // Generate a filter to receive the response.
    auto pingFilter = this->_buildSignerMessageFilters();

    this->_nostrService->publishEvent(messageEvent);

    // TODO: Handle the relay response.
    this->_nostrService->queryRelays(
        pingFilter,
        [this, &pingPromise](const string&, shared_ptr<Event> pongEvent)
        {
            // 
            string pongMessage = this->_unwrapSignerMessage(pongEvent);
            pingPromise.set_value(pongMessage == "pong");
        },
        [&pingPromise](const string&)
        {
            pingPromise.set_value(false);
        },
        [&pingPromise](const string&, const string&)
        {
            pingPromise.set_value(false);
        });
    
    return pingPromise;
};

#pragma endregion

#pragma region Cryptography

void NoscryptSigner::_reseedRandomNumberGenerator(uint32_t bufferSize)
{
    int rc = RAND_load_file("/dev/random", bufferSize);
    if (rc != bufferSize)
    {
        PLOG_WARNING << "Failed to reseed the RNG with /dev/random, falling back to /dev/urandom.";
        RAND_poll();
    }
};

string NoscryptSigner::_encryptNip04(std::string input)
{
    throw runtime_error("NIP-04 encryption is not yet implemented.");
};

string NoscryptSigner::_decryptNip04(string input)
{
    throw runtime_error("NIP-04 decryption is not yet implemented.");
};

string NoscryptSigner::_encryptNip44(string input)
{
    uint32_t nip44Version = 0x02;

    auto nonce = make_shared<uint8_t>(32);
    auto hmacKey = make_shared<uint8_t>(32);

    uint32_t bufferSize = input.length();
    auto output = make_shared<uint8_t>(bufferSize);

    // Generate a nonce to use for the encryption.
    int code = RAND_bytes(nonce.get(), 32);
    if (code <= 0)
    {
        PLOG_ERROR << "Failed to generate a nonce for NIP-44 encryption.";
        return string();
    }

    // Setup the encryption context.
    auto encryptionArgs = make_unique<NCEncryptionArgs>(NCEncryptionArgs
    {
        nonce.get(),
        hmacKey.get(),
        reinterpret_cast<uint8_t*>(input.data()),
        output.get(),
        bufferSize,
        nip44Version
    });

    // Perform the encryption.
    NCResult encryptionResult = NCEncrypt(
        this->_noscryptContext.get(),
        this->_localPrivateKey.get(),
        this->_remotePublicKey.get(),
        encryptionArgs.get());
    
    // TODO: Handle various codes.
    if (encryptionResult != NC_SUCCESS)
    {
        return string();
    }

    return string((char*)output.get(), bufferSize);
};

string NoscryptSigner::_decryptNip44(string input)
{
    uint32_t nip44Version = 0x02;

    auto nonce = make_shared<uint8_t>(32);
    auto hmacKey = make_shared<uint8_t>(32);

    uint32_t bufferSize = input.length();
    auto output = make_shared<uint8_t>(bufferSize);

    // Generate a nonce to use for the decryption.
    int code = RAND_bytes(nonce.get(), 32);
    if (code <= 0)
    {
        PLOG_ERROR << "Failed to generate a nonce for NIP-44 decryption.";
        return string();
    }

    // Set up the decryption context.
    auto decryptionArgs = make_unique<NCEncryptionArgs>(NCEncryptionArgs
    {
        nonce.get(),
        hmacKey.get(),
        reinterpret_cast<uint8_t*>(input.data()),
        output.get(),
        bufferSize,
        nip44Version
    });

    // Perform the decryption.
    NCResult decryptionResult = NCDecrypt(
        this->_noscryptContext.get(),
        this->_localPrivateKey.get(),
        this->_remotePublicKey.get(),
        decryptionArgs.get());

    // TODO: Handle various codes.
    if (decryptionResult != NC_SUCCESS)
    {
        return string();
    }

    return string((char*)output.get(), bufferSize);
};

#pragma endregion

#pragma region Logging

inline void NoscryptSigner::_logNoscryptInitResult(NCResult initResult) const
{
    switch (initResult) {
    case NC_SUCCESS:
        PLOG_INFO << "noscrypt - success";
        break;
    
    case E_NULL_PTR:
        PLOG_ERROR << "noscrypt - error: A null pointer was passed to the initializer.";
        break;

    case E_INVALID_ARG:
        PLOG_ERROR << "noscrypt - error: An invalid argument was passed to the initializer.";
        break;
    
    case E_INVALID_CONTEXT:
        PLOG_ERROR << "noscrypt - error: The NCContext struct is in an invalid state.";
        break;

    case E_ARGUMENT_OUT_OF_RANGE:
        PLOG_ERROR << "noscrypt - error: An initializer argument was outside the range of acceptable values.";
        break;

    case E_OPERATION_FAILED:
        PLOG_ERROR << "noscrypt - error";
        break;
    }
};

inline void NoscryptSigner::_logNoscryptSecretValidationResult(NCResult secretValidationResult) const
{
    if (secretValidationResult == NC_SUCCESS)
    {
        PLOG_INFO << "noscrypt_signer - success: Generated a valid secret key.";
    }
    else
    {
        PLOG_ERROR << "noscrypt_signer - error: Failed to generate a valid secret key.";
    }
};

inline void NoscryptSigner::_logNoscryptPubkeyGenerationResult(NCResult pubkeyGenerationResult) const
{
    switch (pubkeyGenerationResult) {
    case NC_SUCCESS:
        PLOG_INFO << "noscrypt - success: Generated a valid public key.";
        break;
    
    case E_NULL_PTR:
        PLOG_ERROR << "noscrypt - error: A null pointer was passed to the public key generation function.";
        break;

    case E_INVALID_ARG:
        PLOG_ERROR << "noscrypt - error: An invalid argument was passed to the public key generation function.";
        break;
    
    case E_INVALID_CONTEXT:
        PLOG_ERROR << "noscrypt - error: The NCContext struct is in an invalid state.";
        break;

    case E_ARGUMENT_OUT_OF_RANGE:
        PLOG_ERROR << "noscrypt - error: An argument was outside the range of acceptable values.";
        break;

    case E_OPERATION_FAILED:
        PLOG_ERROR << "noscrypt - error: Failed to generate the public key from the secret key.";
        break;
    }
};

#pragma endregion
