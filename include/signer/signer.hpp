#pragma once

#include "data/data.hpp"

namespace nostr
{
namespace signer
{
/**
 * @brief An interface for Nostr event signing that implements NIP-46.
 */
class ISigner
{
public:
    /**
     * @brief Signs the given Nostr event.
     * @param event The event to sign.
     * @remark The event's `sig` field will be updated in-place with the signature.
     */
    virtual void sign(std::shared_ptr<nostr::data::Event> event) = 0;
};

class INostrConnectSigner : public ISigner
{
    virtual void receiveConnection(std::string connectionToken) = 0;

    virtual void initiateConnection(
        std::string relay,
        std::string name,
        std::string url,
        std::string description) = 0;
};
} // namespace signer
} // namespace nostr
