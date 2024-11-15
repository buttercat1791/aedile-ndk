#pragma once

#include <memory>

namespace nostr
{
namespace cryptography
{

class NostrSecureRng
{
private:

public:

    /**
     * @brief Fills the given buffer with secure random bytes.
     * @param buffer The buffer to fill with random bytes.
     * @param length The number of bytes to fill.
     */
    static void fill(void* buffer, size_t length);

    /*
     * @brief Fills the given vector with secure random bytes.
     * @param buffer The vector to fill with random bytes.
     */
    static inline void fill(std::vector<uint8_t>& buffer)
    {
        fill(buffer.data(), buffer.size());
    }

    /*
     * @brief Reseeds the RNG with random bytes from /dev/random.
     * @param bufferSize The number of bytes to read from /dev/random.
     * @remark Falls back to /dev/urandom if /dev/random is not available.
     */
    static void reseed(uint32_t bufferSize = 32);

    /*
     * @brief Securley zeroes out the given buffer.
     * @param buffer A pointer to the buffer to zero out.
     * @param length The number of bytes to zero out.
     */
    static void zero(void* buffer, size_t length);

    /*
     * @brief Securley zeroes out the given vector.
     * @param buffer The vector to zero out.
     */
    static inline void zero(std::vector<uint8_t>& buffer);
};

} // namespace cryptography
} // namespace nostr
