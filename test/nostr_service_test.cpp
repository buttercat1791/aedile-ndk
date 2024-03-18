#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <plog/Appenders/ConsoleAppender.h>
#include <plog/Formatters/TxtFormatter.h>
#include <websocketpp/client.hpp>

#include <client/web_socket_client.hpp>
#include <nostr.hpp>

using std::function;
using std::lock_guard;
using std::make_shared;
using std::mutex;
using std::shared_ptr;
using std::string;
using std::tuple;
using std::unordered_map;
using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

namespace nostr_test
{
class MockWebSocketClient : public client::IWebSocketClient {
public:
    MOCK_METHOD(void, start, (), (override));
    MOCK_METHOD(void, stop, (), (override));
    MOCK_METHOD(void, openConnection, (string uri), (override));
    MOCK_METHOD(bool, isConnected, (string uri), (override));
    MOCK_METHOD((tuple<string, bool>), send, (string message, string uri), (override));
    MOCK_METHOD(void, receive, (string uri, function<void(const string&)> messageHandler), (override));
    MOCK_METHOD(void, closeConnection, (string uri), (override));
};

class NostrServiceTest : public testing::Test
{
public:
    inline static const nostr::RelayList defaultTestRelays =
    {
        "wss://relay.damus.io",
        "wss://nostr.thesamecat.io"
    };

protected:
    shared_ptr<plog::ConsoleAppender<plog::TxtFormatter>> testAppender;
    shared_ptr<MockWebSocketClient> testClient;

    void SetUp() override
    {
        testAppender = make_shared<plog::ConsoleAppender<plog::TxtFormatter>>();
        testClient = make_shared<MockWebSocketClient>();
    };
};

TEST_F(NostrServiceTest, Constructor_StartsClient)
{
    EXPECT_CALL(*testClient, start()).Times(1);

    auto nostrService = new nostr::NostrService(testAppender.get(), testClient.get());
};

TEST_F(NostrServiceTest, Constructor_InitializesService_WithNoDefaultRelays)
{
    auto nostrService = new nostr::NostrService(testAppender.get(), testClient.get());
    auto defaultRelays = nostrService->defaultRelays();
    auto activeRelays = nostrService->activeRelays();

    ASSERT_EQ(defaultRelays.size(), 0);
    ASSERT_EQ(activeRelays.size(), 0);
};

TEST_F(NostrServiceTest, Constructor_InitializesService_WithProvidedDefaultRelays)
{
    auto nostrService = new nostr::NostrService(testAppender.get(), testClient.get(), defaultTestRelays);
    auto defaultRelays = nostrService->defaultRelays();
    auto activeRelays = nostrService->activeRelays();

    ASSERT_EQ(defaultRelays.size(), defaultTestRelays.size());
    for (auto relay : defaultRelays)
    {
        ASSERT_NE(find(defaultTestRelays.begin(), defaultTestRelays.end(), relay), defaultTestRelays.end());
    }
    ASSERT_EQ(activeRelays.size(), 0);
};

TEST_F(NostrServiceTest, Destructor_StopsClient)
{
    EXPECT_CALL(*testClient, start()).Times(1);

    auto nostrService = new nostr::NostrService(testAppender.get(), testClient.get());
};

TEST_F(NostrServiceTest, OpenRelayConnections_OpensConnections_ToDefaultRelays)
{
    mutex connectionStatusMutex;
    auto connectionStatus = make_shared<unordered_map<string, bool>>();
    connectionStatus->insert({ defaultTestRelays[0], false });
    connectionStatus->insert({ defaultTestRelays[1], false });

    EXPECT_CALL(*testClient, openConnection(defaultTestRelays[0])).Times(1);
    EXPECT_CALL(*testClient, openConnection(defaultTestRelays[1])).Times(1);

    EXPECT_CALL(*testClient, isConnected(_))
        .WillRepeatedly(Invoke([connectionStatus, &connectionStatusMutex](string uri)
        {
            lock_guard<mutex> lock(connectionStatusMutex);
            bool status = connectionStatus->at(uri);
            if (status == false)
            {
                connectionStatus->at(uri) = true;
            }
            return status;
        }));
    
    auto nostrService = new nostr::NostrService(testAppender.get(), testClient.get(), defaultTestRelays);
    nostrService->openRelayConnections();

    auto activeRelays = nostrService->activeRelays();
    ASSERT_EQ(activeRelays.size(), defaultTestRelays.size());
    for (auto relay : activeRelays)
    {
        ASSERT_NE(find(defaultTestRelays.begin(), defaultTestRelays.end(), relay), defaultTestRelays.end());
    }
};

TEST_F(NostrServiceTest, OpenRelayConnections_OpensConnections_ToProvidedRelays)
{
    nostr::RelayList testRelays = { "wss://nos.lol" };

    mutex connectionStatusMutex;
    auto connectionStatus = make_shared<unordered_map<string, bool>>();
    connectionStatus -> insert({ testRelays[0], false });

    EXPECT_CALL(*testClient, openConnection(testRelays[0])).Times(1);
    EXPECT_CALL(*testClient, openConnection(defaultTestRelays[0])).Times(0);
    EXPECT_CALL(*testClient, openConnection(defaultTestRelays[1])).Times(0);

    EXPECT_CALL(*testClient, isConnected(_))
        .WillRepeatedly(Invoke([connectionStatus, &connectionStatusMutex](string uri)
        {
            lock_guard<mutex> lock(connectionStatusMutex);
            bool status = connectionStatus->at(uri);
            if (status == false)
            {
                connectionStatus->at(uri) = true;
            }
            return status;
        }));

    auto nostrService = new nostr::NostrService(testAppender.get(), testClient.get(), defaultTestRelays);
    nostrService->openRelayConnections(testRelays);

    auto activeRelays = nostrService->activeRelays();
    ASSERT_EQ(activeRelays.size(), testRelays.size());
    for (auto relay : activeRelays)
    {
        ASSERT_NE(find(testRelays.begin(), testRelays.end(), relay), testRelays.end());
    }
};

TEST_F(NostrServiceTest, OpenRelayConnections_AddsOpenConnections_ToActiveRelays)
{
    nostr::RelayList testRelays = { "wss://nos.lol" };

    mutex connectionStatusMutex;
    auto connectionStatus = make_shared<unordered_map<string, bool>>();
    connectionStatus->insert({ defaultTestRelays[0], false });
    connectionStatus->insert({ defaultTestRelays[1], false });
    connectionStatus->insert({ testRelays[0], false });

    EXPECT_CALL(*testClient, openConnection(defaultTestRelays[0])).Times(1);
    EXPECT_CALL(*testClient, openConnection(defaultTestRelays[1])).Times(1);
    EXPECT_CALL(*testClient, openConnection(testRelays[0])).Times(1);

    EXPECT_CALL(*testClient, isConnected(_))
        .WillRepeatedly(Invoke([connectionStatus, &connectionStatusMutex](string uri)
        {
            lock_guard<mutex> lock(connectionStatusMutex);
            bool status = connectionStatus->at(uri);
            if (status == false)
            {
                connectionStatus->at(uri) = true;
            }
            return status;
        }));

    auto nostrService = new nostr::NostrService(testAppender.get(), testClient.get(), defaultTestRelays);
    nostrService->openRelayConnections();

    auto activeRelays = nostrService->activeRelays();
    ASSERT_EQ(activeRelays.size(), defaultTestRelays.size());
    for (auto relay : activeRelays)
    {
        ASSERT_NE(find(defaultTestRelays.begin(), defaultTestRelays.end(), relay), defaultTestRelays.end());
    }

    nostrService->openRelayConnections(testRelays);

    activeRelays = nostrService->activeRelays();
    ASSERT_EQ(activeRelays.size(), defaultTestRelays.size() + testRelays.size());
    for (auto relay : activeRelays)
    {
        bool isDefaultRelay = find(defaultTestRelays.begin(), defaultTestRelays.end(), relay)
            != defaultTestRelays.end();
        bool isTestRelay = find(testRelays.begin(), testRelays.end(), relay)
            != testRelays.end();
        ASSERT_TRUE(isDefaultRelay || isTestRelay);
    }
};

TEST_F(NostrServiceTest, CloseRelayConnections_ClosesConnections_ToActiveRelays)
{
    mutex connectionStatusMutex;
    auto connectionStatus = make_shared<unordered_map<string, bool>>();
    connectionStatus->insert({ defaultTestRelays[0], false });
    connectionStatus->insert({ defaultTestRelays[1], false });

    EXPECT_CALL(*testClient, isConnected(_))
        .WillRepeatedly(Invoke([connectionStatus, &connectionStatusMutex](string uri)
        {
            lock_guard<mutex> lock(connectionStatusMutex);
            bool status = connectionStatus->at(uri);
            if (status == false)
            {
                connectionStatus->at(uri) = true;
            }
            return status;
        }));

    auto nostrService = new nostr::NostrService(testAppender.get(), testClient.get(), defaultTestRelays);
    nostrService->openRelayConnections();

    EXPECT_CALL(*testClient, closeConnection(defaultTestRelays[0])).Times(1);
    EXPECT_CALL(*testClient, closeConnection(defaultTestRelays[1])).Times(1);

    nostrService->closeRelayConnections();

    auto activeRelays = nostrService->activeRelays();
    ASSERT_EQ(activeRelays.size(), 0);
};

TEST_F(NostrServiceTest, CloseRelayConnections_RemovesClosedConnections_FromActiveRelays)
{
    nostr::RelayList testRelays = { "wss://nos.lol" };
    nostr::RelayList allTestRelays = { defaultTestRelays[0], defaultTestRelays[1], testRelays[0] };

    mutex connectionStatusMutex;
    auto connectionStatus = make_shared<unordered_map<string, bool>>();
    connectionStatus->insert({ defaultTestRelays[0], false });
    connectionStatus->insert({ defaultTestRelays[1], false });
    connectionStatus->insert({ testRelays[0], false });

    EXPECT_CALL(*testClient, isConnected(_))
        .WillRepeatedly(Invoke([connectionStatus, &connectionStatusMutex](string uri)
        {
            lock_guard<mutex> lock(connectionStatusMutex);
            bool status = connectionStatus->at(uri);
            if (status == false)
            {
                connectionStatus->at(uri) = true;
            }
            return status;
        }));

    auto nostrService = new nostr::NostrService(testAppender.get(), testClient.get(), allTestRelays);
    nostrService->openRelayConnections();

    EXPECT_CALL(*testClient, closeConnection(testRelays[0])).Times(1);

    nostrService->closeRelayConnections(testRelays);

    auto activeRelays = nostrService->activeRelays();
    ASSERT_EQ(activeRelays.size(), defaultTestRelays.size());
    for (auto relay : activeRelays)
    {
        bool isDefaultRelay = find(defaultTestRelays.begin(), defaultTestRelays.end(), relay)
            != defaultTestRelays.end();
        bool isTestRelay = find(testRelays.begin(), testRelays.end(), relay)
            != testRelays.end();
        ASSERT_TRUE((isDefaultRelay || isTestRelay) && !(isDefaultRelay && isTestRelay)); // XOR
    }
};

TEST_F(NostrServiceTest, PublishEvent_CorrectlyIndicates_AllSuccesses)
{
    mutex connectionStatusMutex;
    auto connectionStatus = make_shared<unordered_map<string, bool>>();
    connectionStatus->insert({ defaultTestRelays[0], false });
    connectionStatus->insert({ defaultTestRelays[1], false });

    EXPECT_CALL(*testClient, isConnected(_))
        .WillRepeatedly(Invoke([connectionStatus, &connectionStatusMutex](string uri)
        {
            lock_guard<mutex> lock(connectionStatusMutex);
            bool status = connectionStatus->at(uri);
            if (status == false)
            {
                connectionStatus->at(uri) = true;
            }
            return status;
        }));

    auto nostrService = new nostr::NostrService(testAppender.get(), testClient.get(), defaultTestRelays);
    nostrService->openRelayConnections();

    EXPECT_CALL(*testClient, send(_, _))
        .Times(2)
        .WillRepeatedly(Invoke([](string message, string uri)
        {
            return make_tuple(uri, true);
        }));
    
    auto [successes, failures] = nostrService->publishEvent(nostr::Event());

    ASSERT_EQ(successes.size(), defaultTestRelays.size());
    for (auto relay : successes)
    {
        ASSERT_NE(find(defaultTestRelays.begin(), defaultTestRelays.end(), relay), defaultTestRelays.end());
    }

    ASSERT_EQ(failures.size(), 0);
};

TEST_F(NostrServiceTest, PublishEvent_CorrectlyIndicates_AllFailures)
{
    mutex connectionStatusMutex;
    auto connectionStatus = make_shared<unordered_map<string, bool>>();
    connectionStatus->insert({ defaultTestRelays[0], false });
    connectionStatus->insert({ defaultTestRelays[1], false });

    EXPECT_CALL(*testClient, isConnected(_))
        .WillRepeatedly(Invoke([connectionStatus, &connectionStatusMutex](string uri)
        {
            lock_guard<mutex> lock(connectionStatusMutex);
            bool status = connectionStatus->at(uri);
            if (status == false)
            {
                connectionStatus->at(uri) = true;
            }
            return status;
        }));

    auto nostrService = new nostr::NostrService(testAppender.get(), testClient.get(), defaultTestRelays);
    nostrService->openRelayConnections();

    EXPECT_CALL(*testClient, send(_, _))
        .Times(2)
        .WillRepeatedly(Invoke([](string message, string uri)
        {
            return make_tuple(uri, false);
        }));
    
    auto [successes, failures] = nostrService->publishEvent(nostr::Event());

    ASSERT_EQ(successes.size(), 0);

    ASSERT_EQ(failures.size(), defaultTestRelays.size());
    for (auto relay : failures)
    {
        ASSERT_NE(find(defaultTestRelays.begin(), defaultTestRelays.end(), relay), defaultTestRelays.end());
    }
};

TEST_F(NostrServiceTest, PublishEvent_CorrectlyIndicates_MixedSuccessesAndFailures)
{
    mutex connectionStatusMutex;
    auto connectionStatus = make_shared<unordered_map<string, bool>>();
    connectionStatus->insert({ defaultTestRelays[0], false });
    connectionStatus->insert({ defaultTestRelays[1], false });

    EXPECT_CALL(*testClient, isConnected(_))
        .WillRepeatedly(Invoke([connectionStatus, &connectionStatusMutex](string uri)
        {
            lock_guard<mutex> lock(connectionStatusMutex);
            bool status = connectionStatus->at(uri);
            if (status == false)
            {
                connectionStatus->at(uri) = true;
            }
            return status;
        }));

    auto nostrService = new nostr::NostrService(testAppender.get(), testClient.get(), defaultTestRelays);
    nostrService->openRelayConnections();

    EXPECT_CALL(*testClient, send(_, defaultTestRelays[0]))
        .Times(1)
        .WillRepeatedly(Invoke([](string message, string uri)
        {
            return make_tuple(uri, true);
        }));
    EXPECT_CALL(*testClient, send(_, defaultTestRelays[1]))
        .Times(1)
        .WillRepeatedly(Invoke([](string message, string uri)
        {
            return make_tuple(uri, false);
        }));
    
    auto [successes, failures] = nostrService->publishEvent(nostr::Event());

    ASSERT_EQ(successes.size(), 1);
    ASSERT_EQ(successes[0], defaultTestRelays[0]);

    ASSERT_EQ(failures.size(), 1);
    ASSERT_EQ(failures[0], defaultTestRelays[1]);
};
} // namespace nostr_test
