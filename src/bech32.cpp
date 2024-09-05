#include <nostr.hpp>

namespace nostr
{
class Bech32 : public IBech32
{
    public:
        void encode(char *hrp, uint8_t *data) override
        {
            uint8_t *squashed;
            size_t squashed_length = sizeof(data) * (int)(8 / 5) + 1;
            convert_bits(data, squashed, 8, 5);
        }
        void encode(char *hrp, char *data) override {}
        void encode(std::string hrp, std::string data) override {}
        void encode(std::string hrp, std::vector<uint8_t> data) override {}

    private:
        void create_checksum() {}


    private:
        char BECH32_ALPHABET[32] = {
            'q', 'p', 'z', 'r', 'y', '9', 'x',
            '8', 'g', 'f', '2', 't', 'v', 'd',
            'w', '0', 's', '3', 'j', 'n', '5',
            '4', 'k', 'h', 'c', 'e', '6', 'm',
            'u','a','7','l'
        };

};
void convert_bits(uint8_t *data, uint8_t *squashed, uint32_t frombits, uint32_t tobits) {

}
}