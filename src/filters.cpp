#include <ctime>
#include <sstream>
#include <string>
#include <nlohmann/json.hpp>

#include "nostr.hpp"

using nlohmann::json;
using std::invalid_argument;
using std::stringstream;
using std::string;
using std::time;

namespace nostr
{
string Filters::serialize()
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
        {"ids", this->ids},
        {"authors", this->authors},
        {"kinds", this->kinds},
        {"since", this->since},
        {"until", this->until},
        {"limit", this->limit}
    };
    
    for (auto& tag : this->tags)
    {
        stringstream ss;
        ss << "#" << tag.first;
        string tagname = ss.str();

        j[tagname] = tag.second;
    }

    return j.dump();
};

void Filters::validate()
{
    bool hasLimit = this->limit > 0;
    if (!hasLimit)
    {
        throw invalid_argument("Filters::validate: The limit must be greater than 0.");
    }

    bool hasUntil = this->until > 0;
    if (!hasUntil)
    {
        this->until = time(nullptr);
    }

    bool hasIds = this->ids.size() > 0;
    bool hasAuthors = this->authors.size() > 0;
    bool hasKinds = this->kinds.size() > 0;
    bool hasTags = this->tags.size() > 0;

    bool hasFilter = hasIds || hasAuthors || hasKinds || hasTags;

    if (!hasFilter)
    {
        throw invalid_argument("Filters::validate: At least one filter must be set.");
    }
};
} // namespace nostr
