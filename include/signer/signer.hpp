#pragma once

#include <future>
#include <memory>
#include <string>
#include <vector>

#include "data/data.hpp"

namespace nostr
{
namespace signer
{
enum class Encryption
{
    NIP04,
    NIP44
};

/**
 * @brief An interface for Nostr event signing that implements NIP-46.
 */
class ISigner
{
public:
    virtual ~ISigner() = default;

    /**
     * @brief Signs the given Nostr event.
     * @param event The event to sign.
     * @returns A promise that will be fulfilled when the event has been signed.  It will be
     * fulfilled with `true` if the signing succeeded, and `false` if it failed.
     * @remark The event's `sig` field will be updated in-place with the signature.
     */
    virtual std::shared_ptr<std::promise<bool>> sign(std::shared_ptr<nostr::data::Event> event) = 0;
};

class INostrConnectSigner : public ISigner
{
public:
    virtual ~INostrConnectSigner() = default;

    /**
     * @brief Establishes a connection to a remote signer using a connection token generated by the
     * signer.
     * @param connectionToken A connection token string beginning with `bunker://`, as defined by
     * NIP-46.
     * @remark A typical use case for this method would be for the user to paste a signer-generated
     * connection token into a client application, which would then call this method to establish a
     * connection to the remote signer.
     */
    virtual void receiveConnection(std::string connectionToken) = 0;

    /**
     * @brief Generates a connection token that a remote signer may use to establish a connection
     * to the client.
     * @param relays A list of one or more relays the client will use to communicate with the
     * remote signer.
     * @returns A connection token string beginning with `nostrconnect://`, as specified by NIP-46,
     * that may be provided to a remote signer to establish a connection to the client.  Returns an
     * empty string if the connection token generation fails.
     */
    virtual std::string initiateConnection(
        std::vector<std::string> relays,
        std::string name,
        std::string url,
        std::string description
    ) = 0;
};
} // namespace signer
} // namespace nostr
