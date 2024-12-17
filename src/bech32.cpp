#include <bech32.hpp>

namespace nostr {
    void Bech32::encode(const char *hrp, const char *data) {
        uint8_t *squashed;
        Bech32::convert_bits((uint8_t*)data, squashed, 8, 5);
    }
    void Bech32::encode(std::string hrp, std::string data) {}

    void Bech32::decode(const char *encoded) {}
    void Bech32::decode(std::string encoded) {}

    void Bech32::convert_bits(uint8_t *data, uint8_t *squashed, uint32_t frombits, uint32_t tobits) {
        uint32_t i = 0, j = 0;
        uint32_t squashed_size = ceil((frombits * (sizeof(data) -1)) / tobits);
        squashed = (uint8_t*)malloc(squashed_size);
        uint32_t bits = 0;
        uint32_t acc = 0;
        uint32_t maxv = (1 << tobits) - 1;
        uint32_t max_acc = (1 << (frombits + tobits - 1)) - 1;

        while (data[i] != '\0' && i < sizeof(data) - 1) {
            acc = acc << frombits | data[i];
            bits += frombits;
            while (bits >= tobits) {
                squashed[j] = acc >> (frombits - tobits);
                bits -= tobits;
                j++;
            }
            i++;
        }
        std::cout <<std::endl;
    }
}