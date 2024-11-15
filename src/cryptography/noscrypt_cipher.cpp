#include <plog/Init.h>
#include <plog/Log.h>

#include <openssl/evp.h>

#include "cryptography/nostr_secure_rng.hpp"
#include "cryptography/noscrypt_cipher.hpp"
#include "../internal/noscrypt_logger.hpp"

using namespace std;
using namespace nostr::cryptography;


NoscryptCipher::NoscryptCipher(NoscryptCipherVersion version, NoscryptCipherMode mode) :
 _cipher(version, mode)
{
    /*
    * We can know what iv size we need for the cipher now and allocate
    * a buffer just to save some allocations and code during the
    * encryption phase. This buffer is only needed during an encryption
    * operation.
    */

    if (mode == NoscryptCipherMode::CIPHER_MODE_ENCRYPT)
    {
        //Resize the vector to the size of the current cipher
        this->_ivBuffer.resize(this->_cipher.ivSize());

        //Safe to assign the iv to the context now and it will maintain a pointer to the buffer
        this->_cipher.setIV(this->_ivBuffer);
    }
}

std::string NoscryptCipher::update(
    const std::shared_ptr<const NCContext> libContext,
    const std::shared_ptr<const NCSecretKey> localKey,
    const std::shared_ptr<const NCPublicKey> remoteKey,
    const std::string& input
)
{
    NCResult result;

    //Argument exception if the input is empty
    if (input.empty())
    {
        return string();
    }

    //Safely convert the string to a vector of bytes (allocates and copies, so maybe speed up later)
    const vector<uint8_t> inputBuffer(input.begin(), input.end());

    result = this->_cipher.setInput(inputBuffer);
    if (result != NC_SUCCESS)
    {
        NC_LOG_ERROR(result);
        return string();
    }

    /*
    * If were in encryption mode a random nonce (iv) must be generated. The size was determined
    * when the cipher was created and already assigned to the context, so we just need to assign
    * the random data.
    *
    * Keep in mind, this will automatically work for nip44 and nip04, either the
    * AES iv or the ChaCha nonce.
    */
    if (this->_cipher.mode() == NoscryptCipherMode::CIPHER_MODE_ENCRYPT)
    {
		//A secure random initialization vector is needed for encryption operations
		NostrSecureRng::fill(this->_ivBuffer);
    }

    //Performs the operation (either encryption or decryption) 
    result = this->_cipher.update(libContext, localKey, remoteKey);
    if (result != NC_SUCCESS)
    {
        NC_LOG_ERROR(result);
        return string();
    }

    /*
    * Time to read the ciper output by getting the size of the output, then creating
    * a string to store it to
    */

    NCResult outputSize = this->_cipher.outputSize();
    if (outputSize <= 0)
    {
        NC_LOG_ERROR(outputSize);
        return string();
    }

    //Alloc vector for reading input data (maybe only alloc once)
    vector<uint8_t> output(outputSize);

    result = this->_cipher.readOutput(output);
    if (result != outputSize)
    {
        NC_LOG_ERROR(result);
        return string();
    }

    return string(output.begin(), output.end());
}

string NoscryptCipher::naiveEncodeBase64(const std::string& str)
{
    // Compute base64 size and allocate a string buffer of that size.
    const size_t encodedSize = NoscryptCipher::base64EncodedSize(str.size());
    
    auto encodedData = make_unique<uint8_t>(encodedSize);

    // Encode the input string to base64.
    EVP_EncodeBlock(
        encodedData.get(),
        reinterpret_cast<const uint8_t*>(str.c_str()),
        str.size()
    );

    // Construct the encoded string from the buffer.
    return string(
        reinterpret_cast<char*>(encodedData.get()),
        encodedSize
    );
}

string NoscryptCipher::naiveDecodeBase64(const string& str)
{
    // Compute the size of the decoded string and allocate a buffer of that size.
    const size_t decodedSize = NoscryptCipher::base64DecodedSize(str.size());
  
    auto decodedData = make_unique<uint8_t>(decodedSize);

    // Decode the input string from base64.
    EVP_DecodeBlock(
        decodedData.get(),
        reinterpret_cast<const uint8_t*>(str.c_str()),
        str.size()
    );

    // Construct the decoded string from the buffer.
    return string(
        reinterpret_cast<char*>(decodedData.get()),
        decodedSize
    );
};
