#pragma once

#include <memory>

#include <noscrypt.h>
#include <noscryptutil.h>

namespace nostr
{
namespace cryptography
{
class NoscryptCipherContext
{
private:
    NCUtilCipherContext* _cipher;

public:

    NoscryptCipherContext(uint32_t version, uint32_t mode)
    {
    /*
    * Create a new cipher context with the specified
    * version and mode that will live for the duration of the
    * instance.
    *
    * The user is expected to use the noscryptutil mode for
    * setting encryption/decryption modes.
    *
    * The cipher will zero out the memory when it is freed.
    *
    * For decryption, by default the mac is verified before
    * decryption occurs.
    *
    * NOTE: The ciper is set to reusable mode, so encrypt/decrypt
    * can be called multiple times although it's not recommended,
    * its just the more predictable way for users to handle it.
    */

    _cipher = NCUtilCipherAlloc(
        version,
        mode | NC_UTIL_CIPHER_ZERO_ON_FREE | NC_UTIL_CIPHER_REUSEABLE
    );

    //TODO, may fail to allocate memory.
    }

    ~NoscryptCipherContext()
    {
        //Free the cipher context (will also zero any data/pointers)
        NCUtilCipherFree(_cipher);
    }

    NCResult update(
        const std::shared_ptr<const NCContext> libContext,
        const std::shared_ptr<const NCSecretKey> localKey,
        const std::shared_ptr<const NCPublicKey> remoteKey
    ) const
    {
        return NCUtilCipherUpdate(_cipher, libContext.get(), localKey.get(), remoteKey.get());
    }

    NCResult setIV(std::vector<uint8_t>& iv) const
    {
        return NCUtilCipherSetProperty(_cipher, NC_ENC_SET_IV, iv.data(), (uint32_t)iv.size());
    }

    size_t ivSize() const
    {
        NCResult size = NCUtilCipherGetIvSize(_cipher);

        if (size <= 0)
        {
            //TODO Implement error handling
            return 0;
        }

        return size;
    }

    NCResult outputSize() const
    {
        return NCUtilCipherGetOutputSize(_cipher);
    }

    uint32_t flags() const
    {
        NCResult result = NCUtilCipherGetFlags(_cipher);

        if (result <= 0)
        {
            //TODO Implement error handling
            return 0;
        }

        return (uint32_t)result;
    }

    NCResult readOutput(std::vector<uint8_t>& output) const
    {
        return NCUtilCipherReadOutput(_cipher, output.data(), (uint32_t)output.size());
    }

    NCResult setInput(const std::vector<uint8_t>& input) const
    {
        /*
        * Assign and validate input string. Init can be only called multiple times
        * without side effects when the reusable flag is set. (currently set)
        */

        return NCUtilCipherInit(_cipher, input.data(), input.size());
    }
};

class NoscryptCipher
{

private:
    const NoscryptCipherContext _cipher;
    /*
     * Stores the initialziation vector (aka nonce for nip44) for the cipher.
     * Noscrypt needs a memory buffer to store the iv, as it only holds pointers.
     * 
     * This buffer must always point to valid memory after the cipher is created.
     */
    std::vector<uint8_t> _ivBuffer;

public:
    NoscryptCipher(uint32_t version, uint32_t mode);

    /*
     * @brief Performs the cipher operation on the input data. Depending on the mode
     * the cipher was initialized as, this will either encrypt or decrypt the data.
     * @param libContext The noscrypt library context.
     * @param localKey The local secret key used to encrypt/decrypt the data.
     * @param remoteKey The remote public key used to encrypt/decrypt the data.
     * @param input The data to encrypt/decrypt.
     * @returns The opposite of the input data.
     * @remark This cipher function follows the nostr nips format and will use do it's
     * best to
     */
    std::string update(
        const std::shared_ptr<const NCContext> libContext,
        const std::shared_ptr<const NCSecretKey> localKey,
        const std::shared_ptr<const NCPublicKey> remoteKey,
        const std::string& input
    );

    /**
     * @brief Computes the length of a base64 encoded string.
     * @param n The length of the string to be encoded.
     * @return The length of the resulting base64 encoded string.
     */
    inline static size_t base64EncodedSize(const size_t n)
    {
        return (((n + 2) / 3) << 2) + 1;
    };

    /**
     * @brief Computes the length of a string decoded from base64.
     * @param n The length of the base64 encoded string.
     * @return The length of the decoded string.
     */
    inline static size_t base64DecodedSize(const size_t n)
    {
        return (n * 3) >> 2;
    };

    static std::string naiveEncodeBase64(const std::string& str);

    static std::string naiveDecodeBase64(const std::string& str);
};
} // namespace cryptography
} // namespace nostr
