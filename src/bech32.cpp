#include <bech32.hpp>

namespace nostr {
    void Bech32::hello_world() {
        std::cout << "Hello encode" << std::endl;
    }
    void Bech32::encode(uint8_t *hrp, uint8_t *data) {}
    void Bech32::encode(std::string hrp, std::string data) {}

    void Bech32::decode(char *encoded) {}
    void Bech32::decode(std::string encoded) {}
}