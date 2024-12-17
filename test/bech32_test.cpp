#include <nostr.hpp>
#include <bech32.hpp>
#include <gtest/gtest.h>

namespace nostr_test {

class Bech32Test: public testing::Test {
    protected:
    Bech32Test() {}
    ~Bech32Test() override {}
    void SetUp() override {}
};

TEST_F(Bech32Test, TestCompilation) {
    auto encoder = nostr::Bech32();
    encoder.encode("npub", "stufff");
}
}


