#include <plog/Init.h>
#include <plog/Log.h>
#include <noscryptutil.h>

#include <openssl/rand.h>

#include "noscrypt_cipher.hpp"

using namespace nostr::signer;
using namespace std;

static void _printNoscryptError(NCResult result, const std::string funcName, int lineNum)
{
    uint8_t argPosition;

    switch (NCParseErrorCode(result, &argPosition))
    {
    case E_NULL_PTR:
        PLOG_ERROR << "noscrypt - error: A null pointer was passed in " << funcName << "(" << argPosition << ") at line " << lineNum;
        break;

    case E_INVALID_ARG:
        PLOG_ERROR << "noscrypt - error: An invalid argument was passed in " << funcName << "(" << argPosition << ") at line " << lineNum;
        break;

    case E_INVALID_CONTEXT:
        PLOG_ERROR << "noscrypt - error: An invalid context was passed in " << funcName << "(" << argPosition << ") on line " << lineNum;
        break;

    case E_ARGUMENT_OUT_OF_RANGE:
        PLOG_ERROR << "noscrypt - error: An argument was out of range in " << funcName << "(" << argPosition << ") at line " << lineNum;
        break;

    case E_OPERATION_FAILED:
        PLOG_ERROR << "noscrypt - error: An operation failed in " << funcName << "(" << argPosition << ") at line " << lineNum;
        break;

    default:
        PLOG_ERROR << "noscrypt - error: An unknown error " << result << " occurred in " << funcName << "(" << argPosition << ") at line " << lineNum;
        break;
    }

}

#define LOG_NC_ERROR(result) _printNoscryptError(result, __func__, __LINE__)

NoscryptCipher::NoscryptCipher(uint32_t version, uint32_t mode) :
 _cipher(version, mode)
{
    /*
    * We can know what iv size we need for the cipher now and allocate
    * a buffer just to save some allocations and code during the
    * encryption phase. This buffer is only needed during an encryption
    * operation.
    */

    if ((mode & NC_UTIL_CIPHER_MODE) == NC_UTIL_CIPHER_MODE_ENCRYPT)
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
        LOG_NC_ERROR(result);
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
    if ((this->_cipher.flags() & NC_UTIL_CIPHER_MODE) == NC_UTIL_CIPHER_MODE_ENCRYPT)
    {
        int code = RAND_bytes(
            this->_ivBuffer.data(),
            this->_ivBuffer.size()  //Size set in constructor
        );

        if (code <= 0)
        {
            PLOG_ERROR << "Failed to generate a nonce or IV for encryption.";
            return string();
        }
    }

    //Performs the operation (either encryption or decryption) 
    result = this->_cipher.update(libContext, localKey, remoteKey);
    if (result != NC_SUCCESS)
    {
        LOG_NC_ERROR(result);
        return string();
    }

    /*
    * Time to read the ciper output by getting the size of the output, then creating
    * a string to store it to
    */

    NCResult outputSize = this->_cipher.outputSize();
    if (outputSize <= 0)
    {
        LOG_NC_ERROR(outputSize);
        return string();
    }

    //Alloc vector for reading input data (maybe only alloc once)
    vector<uint8_t> output(outputSize);

    result = this->_cipher.readOutput(output);
    if (result != outputSize)
    {
        LOG_NC_ERROR(result);
        return string();
    }

	return string(output.begin(), output.end());
}
