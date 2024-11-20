#include <stdexcept>

#include "data/data.hpp"

using namespace nlohmann;
using namespace nostr::data;
using namespace std;

string Filters::serialize(string& subscriptionId)
{
    try
    {
        this->validate();
    }
    catch (const invalid_argument& e)
    {
        throw e;
    }

    json j = *this;
    json jarr = json::array({ "REQ", subscriptionId, j });

    return jarr.dump();
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

void adl_serializer<Filters>::to_json(json& j, const Filters& filters)
{
    j = {
        { "ids", filters.ids },
        { "authors", filters.authors },
        { "kinds", filters.kinds },
        { "since", filters.since },
        { "until", filters.until },
        { "limit", filters.limit }
    };

    for (auto& tag : filters.tags)
    {
        string name = tag.first[0] == '#'
            ? tag.first
            : '#' + tag.first;

        json values = json::array();
        for (auto& value : tag.second)
        {
            values.push_back(value);
        }

        j[name] = values;
    }
}
