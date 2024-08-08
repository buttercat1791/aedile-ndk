
#include <memory>

#include <noscrypt.h>
#include <noscryptutil.h>

namespace nostr
{
    namespace signer
    {
        class NoscryptCipher
        {
        public:
            NoscryptCipher(uint32_t version, uint32_t mode);

            ~NoscryptCipher();

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
                const std::string input
            );

            static std::string naiveEncodeBase64(const std::string& str)
            {
                //TODO Implement base64 encoding
                return str;
            }

            static std::string naiveDecodeBase64(const std::string& str)
            {
                //TODO Implement base64 decoding
                return str;
            }

        private:

            NoscryptCipherContext _cipher;
            std::vector<uint8_t> _ivBuffer;
        };

        class NoscryptCipherContext
        {
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
			}

			~NoscryptCipherContext()
			{
                //Free the cipher context (will also zero any data/pointers)
                NCUtilCipherFree(_cipher);
			}

			NCResult update(
				const shared_ptr<const NCContext>& libContext,
				const shared_ptr<const NCSecretKey>& localKey,
				const shared_ptr<const NCPublicKey>& remoteKey
            )
			{
                return NCUtilCipherUpdate(this->_cipher, libContext.get(), localKey.get(), remoteKey.get());
			}

			NCResult setIV(const std::vector<uint8_t>& iv)
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

            NCResult readOutput(const std::vector<uint8_t>& output) const
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

        private:
			NCUtilCipherContext* _cipher;
        };
    }
}
