#include <ctime>
#include <openssl/evp.h>
#include <iomanip>
#include <sstream>
#include <string>
#include <nlohmann/json.hpp>
#include <sha.h>

#include "nostr.hpp"

using nlohmann::json;
using std::hex;
using std::invalid_argument;
using std::setw;
using std::setfill;
using std::shared_ptr;
using std::string;
using std::stringstream;
using std::time;

namespace nostr 
{
string Event::serialize(shared_ptr<ISigner> signer)
{
    try
    {
        this->validate();
    }
    catch (const invalid_argument& e)
    {
        throw e;
    }

    json j = {
        {"pubkey", this->pubkey},
        {"created_at", this->createdAt},
        {"kind", this->kind},
        {"tags", this->tags},
        {"content", this->content},
        {"sig", this->sig}
    };

    j["id"] = this->generateId(j.dump());
    j["sig"] = signer->generateSignature(shared_ptr<Event>(this));

    json jarr = json::array({ "EVENT", j });

    return jarr.dump();
};

void Event::deserialize(string jsonString)
{
    json j = json::parse(jsonString);
    this->id = j["id"];
    this->pubkey = j["pubkey"];
    this->createdAt = j["created_at"];
    this->kind = j["kind"];
    this->tags = j["tags"];
    this->content = j["content"];
    this->sig = j["sig"];
};

void Event::validate()
{
    bool hasPubkey = this->pubkey.length() > 0;
    if (!hasPubkey)
    {
        throw std::invalid_argument("Event::validate: The pubkey of the event author is required.");
    }

    bool hasCreatedAt = this->createdAt > 0;
    if (!hasCreatedAt)
    {
        this->createdAt = time(nullptr);
    }

    bool hasKind = this->kind >= 0 && this->kind < 40000;
    if (!hasKind)
    {
        throw std::invalid_argument("Event::validate: A valid event kind is required.");
    }

    bool hasSig = this->sig.length() > 0;
    if (!hasSig)
    {
        throw std::invalid_argument("Event::validate: The event must be signed.");
    }
};

string Event::generateId(string serializedData) const
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    EVP_Digest(serializedData.c_str(), serializedData.length(), hash, NULL, EVP_sha256(), NULL);

    stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    }

    return ss.str();
};
} // namespace nostr
