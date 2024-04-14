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
using std::make_shared;
using std::ostringstream;
using std::setw;
using std::setfill;
using std::shared_ptr;
using std::string;
using std::stringstream;
using std::time;

namespace nostr 
{
string Event::serialize()
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

    return j.dump();
};

Event Event::fromString(string jstr)
{
    json j = json::parse(jstr);
    Event event;

    try
    {
        event = Event::fromJson(j);
    }
    catch (const invalid_argument& e)
    {
        throw e;
    }

    return event;
};

Event Event::fromJson(json j)
{
    Event event;

    try {
        event.id = j.at("id");
        event.pubkey = j.at("pubkey");
        event.createdAt = j.at("created_at");
        event.kind = j.at("kind");
        event.tags = j.at("tags");
        event.content = j.at("content");
        event.sig = j.at("sig");
    } catch (const json::out_of_range& e) {
        ostringstream oss;
        oss << "Event::fromJson: Tried to access an out-of-range element: " << e.what();
        throw invalid_argument(oss.str());
    }

    return event;
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

    bool hasSignature = this->sig.length() > 0;
    if (!hasSignature)
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
