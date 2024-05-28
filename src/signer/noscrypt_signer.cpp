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

void nostr::signer::NoscryptSigner::initiateConnection(
    string relay,
    string name,
    string url,
    string description)
{
    // Initiate the connection here.
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

    NCResult result = NCInitContext(context.get(), randomEntropy.get());
    this->_logNoscryptInitResult(result);

    if (result != NC_SUCCESS)
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
    NCResult result = NCValidateSecretKey(this->_noscryptContext.get(), secretKey.get());
    this->_logNoscryptSecretKeyResult(result);

    if (result != NC_SUCCESS)
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
    NCResult result = NCGetPublicKey(
        this->_noscryptContext.get(),
        secretKey.get(),
        pubkey.get());
    this->_logNoscryptPublicKeyResult(result);

    if (result != NC_SUCCESS)
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

void nostr::signer::NoscryptSigner::_logNoscryptInitResult(NCResult result)
{
    switch (result) {
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

void nostr::signer::NoscryptSigner::_logNoscryptSecretKeyResult(NCResult result)
{
    switch (result) {
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

void nostr::signer::NoscryptSigner::_logNoscryptPublicKeyResult(NCResult result)
{
    switch (result) {
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
