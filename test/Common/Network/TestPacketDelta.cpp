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

TEST_F(TestPacketDelta, FragmentZeroMaxSize)
{
    /*
    PacketDelta toTest(PacketDelta(), 0, 0);

    TestEmpty(toTest);
    EXPECT_EQ(0, toTest.TakeBuffer().size());
    */
    // RAM: TODO!
}
