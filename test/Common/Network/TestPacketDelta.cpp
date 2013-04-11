/*
    Game-in-a-box. Simple First Person Shooter Network Game.
    Copyright (C) 2012 Richard Maxwell <jodi.the.tigger@gmail.com>

    This file is part of Game-in-a-box

    Game-in-a-box is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <Common/Network/PacketSimple.h>
#include <Common/Network/PacketChallenge.h>
#include <Common/Network/PacketChallengeResponse.h>
#include <Common/Network/PacketDelta.h>
#include <gmock/gmock.h>

using namespace std;

// Class definition!
class TestPacketDelta : public ::testing::Test
{
public:
    TestPacketDelta()
        : delta8BytePayloadServer(
              WrappingCounter<uint16_t>(1),
              WrappingCounter<uint16_t>(2),
              3,
              nullptr,
              {1,2,3,4,5,6,7,8})
    {
    }

    void TestEmpty(const PacketDelta& toTest)
    {
        EXPECT_FALSE(toTest.IsValid());

        EXPECT_FALSE(toTest.IsFragmented());
        EXPECT_FALSE(toTest.IsLastFragment());
        EXPECT_EQ(0, toTest.FragmentId());
        EXPECT_EQ(0, toTest.Size());

        EXPECT_FALSE(toTest.HasClientId());
        EXPECT_EQ(0, toTest.GetSequence());
        EXPECT_EQ(0, toTest.GetSequenceAck());
        EXPECT_EQ(0, toTest.GetSequenceBase());
        EXPECT_EQ(0, toTest.GetPayload().size());
    }

    PacketDelta delta8BytePayloadServer;
};

TEST_F(TestPacketDelta, Empty)
{
    PacketDelta empty;

    TestEmpty(empty);
    EXPECT_EQ(0, empty.TakeBuffer().size());
}

TEST_F(TestPacketDelta, TakeBuffer)
{
    uint16_t clientId(4);

    PacketDelta toTest(
                WrappingCounter<uint16_t>(1),
                WrappingCounter<uint16_t>(2),
                3,
                &clientId,
                std::vector<uint8_t>());

    EXPECT_TRUE(toTest.IsValid());
    EXPECT_EQ(7, toTest.TakeBuffer().size());
    TestEmpty(toTest);
    EXPECT_EQ(0, toTest.TakeBuffer().size());
}

TEST_F(TestPacketDelta, NoDataClient)
{
    uint16_t clientId(4);

    PacketDelta toTest(
                WrappingCounter<uint16_t>(1),
                WrappingCounter<uint16_t>(2),
                3,
                &clientId,
                std::vector<uint8_t>());

    EXPECT_TRUE(toTest.IsValid());

    EXPECT_FALSE(toTest.IsFragmented());
    EXPECT_FALSE(toTest.IsLastFragment());
    EXPECT_EQ(0, toTest.FragmentId());
    EXPECT_NE(0, toTest.Size());

    EXPECT_TRUE(toTest.HasClientId());
    EXPECT_EQ(1, toTest.GetSequence());
    EXPECT_EQ(2, toTest.GetSequenceAck());
    EXPECT_EQ(0xFFFF, toTest.GetSequenceBase());
    EXPECT_EQ(4, toTest.ClientId());
    EXPECT_EQ(7, toTest.TakeBuffer().size());
}


TEST_F(TestPacketDelta, NoDataServer)
{
    PacketDelta toTest(2,4,6,nullptr,{});

    EXPECT_TRUE(toTest.IsValid());

    EXPECT_FALSE(toTest.IsFragmented());
    EXPECT_FALSE(toTest.IsLastFragment());
    EXPECT_EQ(0, toTest.FragmentId());
    EXPECT_NE(0, toTest.Size());

    EXPECT_FALSE(toTest.HasClientId());
    EXPECT_EQ(2, toTest.GetSequence());
    EXPECT_EQ(4, toTest.GetSequenceAck());
    EXPECT_EQ(0xFFFE, toTest.GetSequenceBase());
    EXPECT_EQ(0, toTest.ClientId());
    EXPECT_EQ(5, toTest.TakeBuffer().size());
}

TEST_F(TestPacketDelta, SimpleServer)
{
    PacketDelta& toTest = delta8BytePayloadServer;
    std::vector<uint8_t> payload(toTest.GetPayload());

    EXPECT_TRUE(toTest.IsValid());

    EXPECT_FALSE(toTest.IsFragmented());
    EXPECT_FALSE(toTest.IsLastFragment());
    EXPECT_EQ(0, toTest.FragmentId());
    EXPECT_NE(0, toTest.Size());

    EXPECT_FALSE(toTest.HasClientId());
    EXPECT_EQ(1, toTest.GetSequence());
    EXPECT_EQ(2, toTest.GetSequenceAck());
    EXPECT_EQ(0xFFFF, toTest.GetSequenceBase());
    EXPECT_EQ(0, toTest.ClientId());
    EXPECT_EQ(13, toTest.TakeBuffer().size());
    EXPECT_EQ(std::vector<uint8_t>({1,2,3,4,5,6,7,8}), payload);
}

TEST_F(TestPacketDelta, SimpleClient)
{
    uint16_t clientId(4);

    PacketDelta toTest(
                WrappingCounter<uint16_t>(1),
                WrappingCounter<uint16_t>(2),
                3,
                &clientId,
                {1,2,3,4});

    std::vector<uint8_t> payload(toTest.GetPayload());

    EXPECT_TRUE(toTest.IsValid());

    EXPECT_FALSE(toTest.IsFragmented());
    EXPECT_FALSE(toTest.IsLastFragment());
    EXPECT_EQ(0, toTest.FragmentId());
    EXPECT_NE(0, toTest.Size());

    EXPECT_TRUE(toTest.HasClientId());
    EXPECT_EQ(1, toTest.GetSequence());
    EXPECT_EQ(2, toTest.GetSequenceAck());
    EXPECT_EQ(0xFFFF, toTest.GetSequenceBase());
    EXPECT_EQ(4, toTest.ClientId());
    EXPECT_EQ(11, toTest.TakeBuffer().size());
    EXPECT_EQ(std::vector<uint8_t>({1,2,3,4}), payload);
}

TEST_F(TestPacketDelta, EncodeDecodeServer)
{
    PacketDelta& source = delta8BytePayloadServer;
    PacketDelta toTest(source.TakeBuffer());

    std::vector<uint8_t> payload(toTest.GetPayload());

    EXPECT_FALSE(source.IsValid());

    EXPECT_FALSE(toTest.IsFragmented());
    EXPECT_FALSE(toTest.IsLastFragment());
    EXPECT_EQ(0, toTest.FragmentId());
    EXPECT_NE(0, toTest.Size());

    EXPECT_TRUE(toTest.IsValid());
    EXPECT_FALSE(toTest.HasClientId());
    EXPECT_EQ(1, toTest.GetSequence());
    EXPECT_EQ(2, toTest.GetSequenceAck());
    EXPECT_EQ(0xFFFF, toTest.GetSequenceBase());
    EXPECT_EQ(0, toTest.ClientId());
    EXPECT_EQ(13, toTest.TakeBuffer().size());
    EXPECT_EQ(std::vector<uint8_t>({1,2,3,4,5,6,7,8}), payload);
}

TEST_F(TestPacketDelta, EncodeDecodeClient)
{
    uint16_t clientId(4);

    PacketDelta source(
                WrappingCounter<uint16_t>(1),
                WrappingCounter<uint16_t>(2),
                3,
                &clientId,
                {1,2,3,4});

    PacketDelta toTest(source.TakeBuffer());
    std::vector<uint8_t> payload(toTest.GetPayload());

    EXPECT_FALSE(source.IsValid());

    EXPECT_FALSE(toTest.IsFragmented());
    EXPECT_FALSE(toTest.IsLastFragment());
    EXPECT_EQ(0, toTest.FragmentId());
    EXPECT_NE(0, toTest.Size());

    EXPECT_TRUE(toTest.IsValid());
    EXPECT_TRUE(toTest.HasClientId());
    EXPECT_EQ(1, toTest.GetSequence());
    EXPECT_EQ(2, toTest.GetSequenceAck());
    EXPECT_EQ(0xFFFF, toTest.GetSequenceBase());
    EXPECT_EQ(4, toTest.ClientId());
    EXPECT_EQ(11, toTest.TakeBuffer().size());
    EXPECT_EQ(std::vector<uint8_t>({1,2,3,4}), payload);
}

// =================================================
// Fragment testing
// =================================================
TEST_F(TestPacketDelta, FragmentEmpty)
{
    PacketDelta toTest(PacketDelta(), 0, 0);

    TestEmpty(toTest);
    EXPECT_EQ(0, toTest.TakeBuffer().size());
}

TEST_F(TestPacketDelta, FragmentIdPastEnd)
{
    PacketDelta toTest(delta8BytePayloadServer, 4, 100);

    TestEmpty(toTest);
    EXPECT_EQ(0, toTest.TakeBuffer().size());
}

TEST_F(TestPacketDelta, FragmentNotFragmented)
{
    PacketDelta toTest(delta8BytePayloadServer, 1024, 0);

    EXPECT_FALSE(toTest.IsFragmented());
    EXPECT_TRUE(toTest.IsValid());
    EXPECT_EQ(toTest.Size(), delta8BytePayloadServer.Size());
}

TEST_F(TestPacketDelta, Fragments)
{
    PacketDelta toTest1(delta8BytePayloadServer, 10, 0);
    PacketDelta toTest2(delta8BytePayloadServer, 10, 1);
    std::vector<uint8_t> payload1(toTest1.GetPayload());
    std::vector<uint8_t> payload2(toTest2.GetPayload());

    // first
    EXPECT_TRUE(toTest1.IsValid());
    EXPECT_TRUE(toTest1.IsFragmented());
    EXPECT_EQ(0, toTest1.FragmentId());
    EXPECT_FALSE(toTest1.IsLastFragment());
    EXPECT_EQ(toTest1.Size(), 10);
    EXPECT_EQ(7, payload1.size());

    // second
    EXPECT_TRUE(toTest2.IsValid());
    EXPECT_TRUE(toTest2.IsFragmented());
    EXPECT_EQ(1, toTest2.FragmentId());
    EXPECT_TRUE(toTest2.IsLastFragment());
    EXPECT_EQ(toTest2.Size(), 3 + ((delta8BytePayloadServer.Size() - 2) - 7));
}

TEST_F(TestPacketDelta, DefragmentEmpty)
{
    auto buffer = std::vector<PacketDelta>();
    PacketDelta toTest(buffer);

    TestEmpty(toTest);
    EXPECT_EQ(0, toTest.TakeBuffer().size());
}

TEST_F(TestPacketDelta, DefragmentSingleNotFragmented)
{
    auto buffer = std::vector<PacketDelta>(1, delta8BytePayloadServer);

    PacketDelta toTest(buffer);

    TestEmpty(toTest);
    EXPECT_EQ(0, toTest.TakeBuffer().size());
}

TEST_F(TestPacketDelta, DefragmentMultipleNotFragmented)
{
    auto buffer = std::vector<PacketDelta>(3, delta8BytePayloadServer);

    PacketDelta toTest(buffer);

    TestEmpty(toTest);
    EXPECT_EQ(0, toTest.TakeBuffer().size());
}

TEST_F(TestPacketDelta, DefragmentTooMany)
{
    PacketDelta fragment(delta8BytePayloadServer, 10, 0);

    auto buffer = std::vector<PacketDelta>(128, fragment);

    PacketDelta toTest(buffer);

    TestEmpty(toTest);
    EXPECT_EQ(0, toTest.TakeBuffer().size());
}

TEST_F(TestPacketDelta, DefragmentSimple)
{
    std::vector<PacketDelta> fragments;

    // Oh comon, this is c++11, do this using lambdas or iterators or something.
    uint8_t count(0);
    bool go(true);
    while (go)
    {
        PacketDelta result(PacketDelta(delta8BytePayloadServer, 8, count));
        if (result.IsLastFragment())
        {
            go = false;
        }
        fragments.push_back(result);
        count++;
    }

    PacketDelta toTest(fragments);

    EXPECT_EQ(toTest, delta8BytePayloadServer);
}

TEST_F(TestPacketDelta, DefragmentFragmentNoFragmentedFragments)
{
    // expect not fragmented item to be ignored.
    // TODO!
    EXPECT_TRUE(false);
}

TEST_F(TestPacketDelta, DefragmentSequenceInterleavedNotComplete)
{
    // send complete delta fragments, but interleaved with once
    // of a newer sequence, should be invalid returned.
    // TODO!
    EXPECT_TRUE(false);
}

