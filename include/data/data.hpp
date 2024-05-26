#pragma once

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <openssl/sha.h>

namespace nostr
{
namespace data
{
/**
 * @brief A Nostr event.
 * @remark All data transmitted over the Nostr protocol is encoded in JSON blobs.  This struct
 * is common to every Nostr event kind.  The significance of each event is determined by the
 * `tags` and `content` fields.
*/
struct Event
{
    std::string id; ///< SHA-256 hash of the event data.
    std::string pubkey; ///< Public key of the event creator.
    std::time_t createdAt; ///< Unix timestamp of the event creation.
    int kind; ///< Event kind.
    std::vector<std::vector<std::string>> tags; ///< Arbitrary event metadata.
    std::string content; ///< Event content.
    std::string sig; ///< Event signature created with the private key of the event creator.

    /**
     * @brief Serializes the event to a JSON object.
     * @returns A stringified JSON object representing the event.
     * @throws `std::invalid_argument` if the event object is invalid.
     */
    std::string serialize();

    /**
     * @brief Deserializes the event from a JSON string.
     * @param jsonString A stringified JSON object representing the event.
     * @returns An event instance created from the JSON string.
     */
    static Event fromString(std::string jsonString);

    /**
     * @brief Deserializes the event from a JSON object.
     * @param j A JSON object representing the event.
     * @returns An event instance created from the JSON object.
     */
    static Event fromJson(nlohmann::json j);

    /**
     * @brief Compares two events for equality.
     * @remark Two events are considered equal if they have the same ID, since the ID is uniquely
     * generated from the event data.  If the `id` field is empty for either event, the comparison
     * function will throw an exception.
     */
    bool operator==(const Event& other) const;

private:
    /**
     * @brief Validates the event.
     * @throws `std::invalid_argument` if the event object is invalid.
     * @remark The `createdAt` field defaults to the present if it is not already set.
     */
    void validate();

    /**
     * @brief Generates an ID for the event.
     * @param serializedData The serialized JSON string of all of the event data except the ID and
     * the signature.
     * @return A valid Nostr event ID.
     * @remark The ID is a 32-bytes lowercase hex-encoded sha256 of the serialized event data.
     */
    std::string generateId(std::string serializedData) const;
};

/**
 * @brief A set of filters for querying Nostr relays.
 * @remark The `limit` field should always be included to keep the response size reasonable.  The
 * `since` field is not required, and the `until` field will default to the present.  At least one
 * of the other fields must be set for a valid filter.
 */
struct Filters
{
    std::vector<std::string> ids; ///< Event IDs.
    std::vector<std::string> authors; ///< Event author npubs.
    std::vector<int> kinds; ///< Kind numbers.
    std::unordered_map<std::string, std::vector<std::string>> tags; ///< Tag names mapped to lists of tag values.
    std::time_t since; ///< Unix timestamp.  Matching events must be newer than this.
    std::time_t until; ///< Unix timestamp.  Matching events must be older than this.
    int limit; ///< The maximum number of events the relay should return on the initial query.

    /**
     * @brief Serializes the filters to a JSON object.
     * @param subscriptionId A string up to 64 chars in length that is unique per relay connection.
     * @returns A stringified JSON object representing the filters.
     * @throws `std::invalid_argument` if the filter object is invalid.
     * @remarks The Nostr client is responsible for managing subscription IDs.  Responses from the
     * relay will be organized by subscription ID.
     */
    std::string serialize(std::string& subscriptionId);

private:
    /**
     * @brief Validates the filters.
     * @throws `std::invalid_argument` if the filter object is invalid.
     * @remark The `until` field defaults to the present if it is not already set.
     */
    void validate();
};
} // namespace data
} // namespace nostr
