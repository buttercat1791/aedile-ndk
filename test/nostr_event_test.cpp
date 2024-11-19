#include <gtest/gtest.h>

#include "data/data.hpp"

using namespace nostr::data;
using namespace std;
using namespace ::testing;

shared_ptr<Event> testEvent()
{
    auto event = make_shared<Event>();

    event->pubkey = "13tn5ccv2guflxgffq4aj0hw5x39pz70zcdrfd6vym887gry38zys28dask";
    event->createdAt = 1627846261;
    event->kind = 1;
    event->tags = {
        { "e", "5c83da77af1dec6d7289834998ad7aafbd9e2191396d75ec3cc27f5a77226f36", "wss://gitcitadel.nostr1.com" },
        { "p", "f7234bd4c1394dda46d09f35bd384dd30cc552ad5541990f98844fb06676e9ca" },
        { "a", "30023:f7234bd4c1394dda46d09f35bd384dd30cc552ad5541990f98844fb06676e9ca:abcd", "wss://gitcitadel.nostr1.com" }
    };
    event->content = "Hello, World!";

    return event;
}

TEST(NostrEventTest, Equivalent_Events_Have_Same_ID)
{
    // Create two events with the same values
    auto event1 = testEvent();
    auto event2 = testEvent();

    // Serialize both events
    string serializedEvent1 = event1->serialize();
    string serializedEvent2 = event2->serialize();

    auto event1WithId = Event::fromString(serializedEvent1);
    auto event2WithId = Event::fromString(serializedEvent2);

    // Hash both serialized events using sha256
    string id1 = event1WithId.id;
    string id2 = event2WithId.id;

    // Verify that both hashes are equal
    ASSERT_EQ(id1, id2);
}

