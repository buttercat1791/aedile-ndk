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
        auto contextStructSize = NCGetContextStructSize();
        unique_ptr<uint8_t[]> randomEntropy(new uint8_t[contextStructSize]);

        random_device device;
        mt19937 seed(device());
        uniform_int_distribution<int> distribution(1, NC_CONTEXT_ENTROPY_SIZE);
        generate_n(randomEntropy.get(), NC_CONTEXT_ENTROPY_SIZE, [&]() { return distribution(seed); });

        NCResult result = NCInitContext(context.get(), randomEntropy.get());
        this->handleNoscryptInitResult(result);
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

    void handleNoscryptInitResult(NCResult result)
    {
        switch (result) {
        case NC_SUCCESS:
            PLOG_INFO << "Successfully initialized noscrypt.";
            break;
        
        case E_NULL_PTR:
            PLOG_ERROR << "Failed to initialize noscrypt: A null pointer was passed to the initializer.";
            break;

        case E_INVALID_ARG:
            PLOG_ERROR << "Failed to initialize noscrypt: An invalid argument was passed to the initializer.";
            break;
        
        case E_INVALID_CONTEXT:
            PLOG_ERROR << "Failed to initialize noscrypt: The NCContext struct is in an invalid state.";
            break;

        case E_ARGUMENT_OUT_OF_RANGE:
            PLOG_ERROR << "Failed to initialize noscrypt: An initializer argument was outside the range of acceptable values.";
            break;

        case E_OPERATION_FAILED:
            PLOG_ERROR << "Failed to initialize noscrypt.";
            break;
        }
    };
};
} // namespace signer
} // namespace nostr
