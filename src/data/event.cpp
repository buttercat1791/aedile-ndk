#include <sstream>
#include <stdexcept>

#include "data/data.hpp"

using namespace nlohmann;
using namespace nostr::data;
using namespace std;

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

    // Generate the event ID from the serialized data.
    this->generateId();

    json j = *this;
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
    Event event = j.get<Event>();
    return event;
};

void Event::validate()
{
    bool hasPubkey = this->pubkey.length() > 0;
    if (!hasPubkey)
    {
        throw invalid_argument("Event::validate: The pubkey of the event author is required.");
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
};

void Event::generateId()
{
    // Create a JSON array of values used to generate the event ID.
    json arr = { 0, this->pubkey, this->createdAt, this->kind, this->tags, this->content };
    string serializedData = arr.dump();

    unsigned char hash[SHA256_DIGEST_LENGTH];
    EVP_Digest(serializedData.c_str(), serializedData.length(), hash, NULL, EVP_sha256(), NULL);

    stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    }

    this->id = ss.str();
};

bool Event::operator==(const Event& other) const
{
    if (this->id.empty())
    {
        throw invalid_argument("Event::operator==: Cannot check equality, the left-side argument is undefined.");
    }
    if (other.id.empty())
    {
        throw invalid_argument("Event::operator==: Cannot check equality, the right-side argument is undefined.");
    }

    return this->id == other.id;
};

void adl_serializer<Event>::to_json(json& j, const Event& event)
{
    // Serialize the event to a JSON object.
    j = {
        { "id", event.id },
        { "pubkey", event.pubkey },
        { "created_at", event.createdAt },
        { "kind", event.kind },
        { "tags", event.tags },
        { "content", event.content },
        { "sig", event.sig },
    };
}

void adl_serializer<Event>::from_json(const json& j, Event& event)
{
    // TODO: Set up custom exception types for improved exception handling.
    try
    {
        event.id = j.at("id");
        event.pubkey = j.at("pubkey");
        event.createdAt = j.at("created_at");
        event.kind = j.at("kind");
        event.tags = j.at("tags");
        event.content = j.at("content");
        event.sig = j.at("sig");

        // TODO: Validate the event against its signature.
    }
    catch (const json::type_error& te)
    {
        throw te;
    }
    catch (const json::out_of_range& oor)
    {
        throw oor;
    }
}
