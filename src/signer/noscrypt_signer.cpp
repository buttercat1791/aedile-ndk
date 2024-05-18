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
    NoscryptSigner(shared_ptr<plog::IAppender> appender)
    {
        // Set up the logger.
        plog::init(plog::debug, appender.get());

        // Set up the noscrypt library.
        this->context = make_shared<NCContext>();
        uint8_t randomEntropy[NC_CONTEXT_ENTROPY_SIZE];

        random_device device;
        mt19937 seed(device());
        uniform_int_distribution<int> distribution(1, NC_CONTEXT_ENTROPY_SIZE);
        generate_n(randomEntropy, NC_CONTEXT_ENTROPY_SIZE, [&]() { return distribution(seed); });

        NCInitContext(context.get(), randomEntropy);
    };

    ~NoscryptSigner()
    {
        NCDestroyContext(context.get());
    };

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

private:
    shared_ptr<NCContext> context;
};
} // namespace signer
} // namespace nostr
