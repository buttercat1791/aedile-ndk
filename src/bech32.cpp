#include <nostr.hpp>

namespace nostr
{
class Bech32 : public IBech32
{
    public:
        void encode(char *hrp, uint8_t *data) override {}
        void encode(char *hrp, char *data) override {}
        void encode(std::string hrp, std::string data) override {}
        void encode(std::string hrp, std::vector<uint8_t> data) override {}

    private:
        void create_checksum() {}

};

}