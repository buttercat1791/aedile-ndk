#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#include "nostr.hpp"

using nlohmann::json;
using std::string;

namespace nostr 
{
json Event::serialize() const
{
    json j = {
        {"id", this->id},
        {"pubkey", this->pubkey},
        {"created_at", this->created_at},
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
    this->created_at = j["created_at"];
    this->kind = j["kind"];
    this->tags = j["tags"];
    this->content = j["content"];
    this->sig = j["sig"];
};
} // namespace nostr
