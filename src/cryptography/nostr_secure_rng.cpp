#include <plog/Init.h>
#include <plog/Log.h>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>

#include "cryptography/nostr_secure_rng.hpp"

using namespace std;
using namespace nostr::cryptography;

void NostrSecureRng::fill(void* buffer, size_t length)
{
	if (RAND_bytes((uint8_t*)buffer, length) != 1)
	{
		//TODO throw runtime exception
		PLOG_ERROR << "Failed to generate random bytes";
	}
}

void NostrSecureRng::reseed(uint32_t bufferSize)
{
	int rc = RAND_load_file("/dev/random", bufferSize);

	if (rc != bufferSize)
	{
		PLOG_WARNING << "Failed to reseed the RNG with /dev/random, falling back to /dev/urandom.";
		RAND_poll();
	}
}

void NostrSecureRng::zero(void* buffer, size_t length)
{
	OPENSSL_cleanse(buffer, length);
}

inline void NostrSecureRng::zero(vector<uint8_t>& buffer)
{
	zero(buffer.data(), buffer.size());
}