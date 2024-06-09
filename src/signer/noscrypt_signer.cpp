#include <algorithm>
#include <chrono>
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

NoscryptSigner::NoscryptSigner(
    shared_ptr<plog::IAppender> appender,
    shared_ptr<INostrServiceBase> nostrService)
{
    plog::init(plog::debug, appender.get());

    this->_reseedRandomNumberGenerator();

    this->_noscryptContext = this->_initNoscryptContext();
    if (this->_noscryptContext == nullptr)
    {
        return;
    }

    const auto [privateKey, publicKey] = this->_createLocalKeypair();
    this->_localPrivateKey = privateKey;
    this->_localPublicKey = publicKey;

    this->_nostrService = nostrService;
};

NoscryptSigner::~NoscryptSigner()
{
    NCDestroyContext(this->_noscryptContext.get());
};

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
    if (this->_localPrivateKey.empty() || this->_localPublicKey.empty())
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
    const int nostrConnectKind = 24133; // Kind 24133 is reserved for NIP-46 events.

    auto signingPromise = make_shared<promise<bool>>();

    // Create the JSON-RPC-like message content.
    auto params = nlohmann::json::array();
    params.push_back(event->serialize());

    auto requestId = this->_generateSignerRequestId();

    nlohmann::json jrpc = {
        { "id", requestId },
        { "method", "sign_event" },
        { "params", params }
    };

    // TODO: Encrypt the message content with NIP-04 (or NIP-44).
    NCEncryptionArgs encryptionArgs;

    string encryptedContent;

    // Wrap the event to be signed in a signing request event.
    auto signingRequest = make_shared<Event>();
    signingRequest->pubkey = this->_localPublicKey;
    signingRequest->kind = nostrConnectKind;
    signingRequest->tags.push_back({ "p", this->_remotePublicKey });
    signingRequest->content = encryptedContent;
    // TODO: Sign the wrapper event with the local private key.

    // Create a filter set to find events from the remote signer.
    auto remoteSignerFilters = make_shared<Filters>();
    remoteSignerFilters->kinds.push_back(nostrConnectKind);
    remoteSignerFilters->since = time(nullptr); // Filter for new signer events.
    remoteSignerFilters->tags["p"] = { this->_localPublicKey }; // Signer events tag the local npub.
    remoteSignerFilters->limit = 1; // We only need the immediate response to the signing request.

    // TODO: Ping the remote signer before sending the request.

    // Send the signing request.
    this->_nostrService->publishEvent(signingRequest);

    // Wait for the remote signer's response.
    this->_nostrService->queryRelays(
        remoteSignerFilters,
        [this, &signingPromise](const string&, shared_ptr<Event> event)
        {
            // TODO: Handle the response from the remote signer.
            
            // Eventually:
            signingPromise->set_value(true);
        },
        nullptr,
        nullptr);
    
    return signingPromise;
};

/**
 * @brief Initializes the noscrypt library context into the class's `context` property.
 * @returns `true` if successful, `false` otherwise.
 */
shared_ptr<NCContext> NoscryptSigner::_initNoscryptContext()
{
    shared_ptr<NCContext> context;
    auto contextStructSize = NCGetContextStructSize();
    unique_ptr<uint8_t> randomEntropy(new uint8_t[contextStructSize]);

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(0, contextStructSize);
    generate_n(randomEntropy.get(), contextStructSize, [&]() { return dist(gen); });

    NCResult initResult = NCInitContext(context.get(), randomEntropy.get());
    this->_logNoscryptInitResult(initResult);

    if (initResult != NC_SUCCESS)
    {
        return nullptr;
    }

    return context;
};

/**
 * @brief Generates a private/public key pair for local use.
 * @returns The generated keypair of the form `[privateKey, publicKey]`, or a pair of empty
 * strings if the function failed.
 * @remarks This keypair is intended for temporary use, and should not be saved or used outside
 * of this class.
 */
tuple<string, string> NoscryptSigner::_createLocalKeypair()
{
    string privateKey;
    string publicKey;

    // To generate a private key, all we need is a random 32-bit buffer.
    unique_ptr<NCSecretKey> secretKey(new NCSecretKey);

    // Loop attempts to generate a secret key until a valid key is produced.
    // Limit the number of attempts to prevent resource exhaustion in the event of a failure.
    NCResult secretValidationResult;
    int loopCount = 0;
    do
    {
        int rc = RAND_bytes(secretKey->key, sizeof(NCSecretKey));
        if (rc != 1)
        {
            unsigned long err = ERR_get_error();
            PLOG_ERROR << "OpenSSL error " << err << " occurred while generating a secret key.";
            return make_tuple(string(), string());
        }

        secretValidationResult = NCValidateSecretKey(this->_noscryptContext.get(), secretKey.get());
    } while (secretValidationResult != NC_SUCCESS && ++loopCount < 64);

    this->_logNoscryptSecretValidationResult(secretValidationResult);
    if (secretValidationResult != NC_SUCCESS)
    {
        // Return empty strings if the secret key generation fails.
        return make_tuple(string(), string());
    }

    this->_localSecret = move(secretKey);

    // Convert the buffer into a hex string for a more human-friendly representation.
    stringstream secretKeyStream;
    for (int i = 0; i < sizeof(NCSecretKey); i++)
    {
        secretKeyStream << hex << setw(2) << setfill('0') << static_cast<int>(secretKey->key[i]);
    }
    privateKey = secretKeyStream.str();

    // Use noscrypt to derive the public key from its private counterpart.
    unique_ptr<NCPublicKey> pubkey(new NCPublicKey);
    NCResult pubkeyGenerationResult = NCGetPublicKey(
        this->_noscryptContext.get(),
        secretKey.get(),
        pubkey.get());
    this->_logNoscryptPubkeyGenerationResult(pubkeyGenerationResult);

    if (pubkeyGenerationResult != NC_SUCCESS)
    {
        // Return empty strings if the pubkey generation fails.
        return make_tuple(string(), string());
    }

    // Convert the now-populated pubkey buffer into a hex string for the pubkey representation
    // used by Nostr events.
    stringstream pubkeyStream;
    for (int i = 0; i < sizeof(NCPublicKey); i++)
    {
        pubkeyStream << hex << setw(2) << setfill('0') << static_cast<int>(pubkey->key[i]);
    }
    publicKey = pubkeyStream.str();

    return make_tuple(privateKey, publicKey);
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
    this->_remotePublicKey = remotePubkey;

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

string NoscryptSigner::_generateSignerRequestId()
{
    UUIDv4::UUIDGenerator<std::mt19937_64> uuidGenerator;
    UUIDv4::UUID uuid = uuidGenerator.getUUID();
    return uuid.str();
};

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

string NoscryptSigner::_encryptNip04()
{
    throw runtime_error("NIP-04 encryption is not yet implemented.");
};

string NoscryptSigner::_encryptNip44(const string input)
{
    uint32_t nip44Version = 0x02;

    shared_ptr<uint8_t> nonce(new uint8_t[32]);
    shared_ptr<uint8_t> hmacKey(new uint8_t[32]);

    uint32_t bufferSize = input.length();
    shared_ptr<uint8_t> output(new uint8_t[bufferSize]);

    // Generate a nonce to use for the encryption.
    int code = RAND_bytes(nonce.get(), 32);
    if (code <= 0)
    {
        PLOG_ERROR << "Failed to generate a nonce for NIP-44 encryption.";
        return string();
    }

    // Setup the encryption context.
    NCEncryptionArgs encryptionArgs =
    {
        nonce.get(),
        hmacKey.get(),
        (uint8_t*)input.c_str(),
        output.get(),
        bufferSize,
        nip44Version
    };

    // Perform the encryption.
    NCResult encryptionResult = NCEncrypt(
        this->_noscryptContext.get(),
        this->_localSecret.get(),
        this->_remotePubkey.get(),
        &encryptionArgs);
    
    // TODO: Handle various codes.
    if (encryptionResult != NC_SUCCESS)
    {
        return string();
    }

    return string((char*)output.get(), bufferSize);
};

#pragma endregion

#pragma region Logging

void NoscryptSigner::_logNoscryptInitResult(NCResult initResult)
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

void NoscryptSigner::_logNoscryptSecretValidationResult(NCResult secretValidationResult)
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

void NoscryptSigner::_logNoscryptPubkeyGenerationResult(NCResult pubkeyGenerationResult)
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
