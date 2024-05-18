#include <noscrypt.h>

#include "signer.hpp"

using namespace std;

namespace nostr
{
namespace signer
{
class NoscryptSigner : public INostrConnectSigner
{
public:
    void receiveConnection(string connectionToken) override
    {
        // Receive the connection token here.
    };

    void initiateConnection(
        string relay,
        string name,
        string url,
        string description) override
    {
        // Initiate the connection here.
    };

    void sign(shared_ptr<data::Event> event) override
    {
        // Sign the event here.
    };
};
} // namespace signer
} // namespace nostr
