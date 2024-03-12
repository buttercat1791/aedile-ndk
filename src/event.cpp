#include <ctime>
#include <string>
#include <nlohmann/json.hpp>

#include "nostr.hpp"

using nlohmann::json;
using std::invalid_argument;
using std::string;
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
        {"id", this->id},
        {"pubkey", this->pubkey},
        {"created_at", this->createdAt},
        {"kind", this->kind},
        {"tags", this->tags},
        {"content", this->content},
        {"sig", this->sig}
    };
    return j.dump();
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
    bool hasId = this->id.length() > 0;
    if (!hasId)
    {
        throw std::invalid_argument("Event::validate: The event id is required.");
    }

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
} // namespace nostr
