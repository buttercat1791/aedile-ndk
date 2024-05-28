#include <algorithm>
#include <random>
#include <sstream>
#include <tuple>

#include "signer/noscrypt_signer.hpp"

using namespace std;

nostr::signer::NoscryptSigner::NoscryptSigner(
    shared_ptr<plog::IAppender> appender,
    shared_ptr<nostr::service::INostrServiceBase> nostrService)
{
    plog::init(plog::debug, appender.get());

    this->_noscryptContext = this->_initNoscryptContext();
    if (this->_noscryptContext == nullptr)
    {
        return;
    }

    const auto [privateKey, publicKey] = this->_createLocalKeypair();
    this->_localPrivateKey = privateKey;
    this->_localPublicKey = publicKey;
};

nostr::signer::NoscryptSigner::~NoscryptSigner()
{
    NCDestroyContext(this->_noscryptContext.get());
};

void nostr::signer::NoscryptSigner::receiveConnection(string connectionToken)
{
    // Receive the connection token here.
};

string nostr::signer::NoscryptSigner::initiateConnection(
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
};

void nostr::signer::NoscryptSigner::sign(shared_ptr<data::Event> event)
{
    // Sign the event here.
};

/**
 * @brief Initializes the noscrypt library context into the class's `context` property.
 * @returns `true` if successful, `false` otherwise.
 */
shared_ptr<NCContext> nostr::signer::NoscryptSigner::_initNoscryptContext()
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
tuple<string, string> nostr::signer::NoscryptSigner::_createLocalKeypair()
{
    string privateKey;
    string publicKey;

    // To generate a private key, all we need is a random 32-bit buffer.
    unique_ptr<NCSecretKey> secretKey(new NCSecretKey);

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(0, NC_SEC_KEY_SIZE);
    generate_n(secretKey.get()->key, NC_SEC_KEY_SIZE, [&]() { return dist(gen); });

    // Check the validity of the secret key.
    NCResult secretValidationResult = NCValidateSecretKey(this->_noscryptContext.get(), secretKey.get());
    this->_logNoscryptSecretValidationResult(secretValidationResult);

    if (secretValidationResult != NC_SUCCESS)
    {
        // Return empty strings if the secret key generation fails.
        return make_tuple(string(), string());
    }

    // Convert the buffer into a hex string for a more human-friendly representation.
    stringstream secretKeyStream;
    for (int i = 0; i < NC_SEC_KEY_SIZE; i++)
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
    for (int i = 0; i < NC_SEC_KEY_SIZE; i++)
    {
        pubkeyStream << hex << setw(2) << setfill('0') << static_cast<int>(pubkey->key[i]);
    }
    publicKey = pubkeyStream.str();

    return make_tuple(privateKey, publicKey);
};

#pragma region Logging

void nostr::signer::NoscryptSigner::_logNoscryptInitResult(NCResult initResult)
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

void nostr::signer::NoscryptSigner::_logNoscryptSecretValidationResult(NCResult secretValidationResult)
{
    switch (secretValidationResult) {
    case NC_SUCCESS:
        PLOG_INFO << "noscrypt - success: Generated a valid secret key.";
        break;
    
    case E_NULL_PTR:
        PLOG_ERROR << "noscrypt - error: A null pointer was passed to the secret key validation function.";
        break;

    case E_INVALID_ARG:
        PLOG_ERROR << "noscrypt - error: An invalid argument was passed to the secret key validation function.";
        break;
    
    case E_INVALID_CONTEXT:
        PLOG_ERROR << "noscrypt - error: The NCContext struct is in an invalid state.";
        break;

    case E_ARGUMENT_OUT_OF_RANGE:
        PLOG_ERROR << "noscrypt - error: An argument was outside the range of acceptable values.";
        break;

    case E_OPERATION_FAILED:
        PLOG_ERROR << "noscrypt - error: Failed to validate the generated secret key.";
        break;
    }
};

void nostr::signer::NoscryptSigner::_logNoscryptPubkeyGenerationResult(NCResult pubkeyGenerationResult)
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
