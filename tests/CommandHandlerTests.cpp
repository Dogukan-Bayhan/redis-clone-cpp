#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include "../src/commands/CommandHandler.hpp"
#include "../src/db/RedisStore.hpp"
#include "TestHelpers.hpp"

TEST(CommandHandlerTest, PingRespondsWithPong) {
    RedisStore store;
    CommandHandler handler(store);

    auto args = makeArgs({"PING"});
    auto reply = handler.execute(args.views, /*client_fd=*/1);

    EXPECT_EQ("+PONG\r\n", reply.reply);
}

TEST(CommandHandlerTest, SetAndGetRoundTrip) {
    RedisStore store;
    CommandHandler handler(store);

    auto setArgs = makeArgs({"SET", "greeting", "hello"});
    auto setReply = handler.execute(setArgs.views, 1);
    EXPECT_EQ("+OK\r\n", setReply.reply);

    auto getArgs = makeArgs({"GET", "greeting"});
    auto getReply = handler.execute(getArgs.views, 1);
    EXPECT_EQ("$5\r\nhello\r\n", getReply.reply);
}

TEST(CommandHandlerTest, SetPxExpiresKey) {
    RedisStore store;
    CommandHandler handler(store);

    auto setArgs = makeArgs({"SET", "temp", "123", "PX", "5"});
    handler.execute(setArgs.views, 1);

    std::this_thread::sleep_for(std::chrono::milliseconds(15));

    auto getArgs = makeArgs({"GET", "temp"});
    auto getReply = handler.execute(getArgs.views, 1);
    EXPECT_EQ("$-1\r\n", getReply.reply);
}

TEST(CommandHandlerTest, ListRangeReturnsInOrder) {
    RedisStore store;
    CommandHandler handler(store);

    handler.execute(makeArgs({"RPUSH", "numbers", "one", "two", "three"}).views, 1);

    auto rangeReply = handler.execute(makeArgs({"LRANGE", "numbers", "0", "-1"}).views, 1);
    auto items = parseBulkArray(rangeReply.reply);

    ASSERT_EQ(3u, items.size());
    EXPECT_EQ("one", items[0]);
    EXPECT_EQ("two", items[1]);
    EXPECT_EQ("three", items[2]);
}

TEST(CommandHandlerTest, BlpopReturnsImmediateResult) {
    RedisStore store;
    CommandHandler handler(store);

    handler.execute(makeArgs({"LPUSH", "jobs", "job2", "job1"}).views, 1);

    auto blpopReply = handler.execute(makeArgs({"BLPOP", "jobs", "0"}).views, 1);
    auto items = parseBulkArray(blpopReply.reply);

    ASSERT_EQ(2u, items.size());
    EXPECT_EQ("jobs", items[0]);
    EXPECT_EQ("job1", items[1]);
}

TEST(CommandHandlerTest, TypeReflectsStoredObjects) {
    RedisStore store;
    CommandHandler handler(store);

    handler.execute(makeArgs({"SET", "alpha", "1"}).views, 1);
    handler.execute(makeArgs({"LPUSH", "queue", "item"}).views, 1);

    auto typeString = handler.execute(makeArgs({"TYPE", "alpha"}).views, 1);
    EXPECT_EQ("+string\r\n", typeString.reply);

    auto typeList = handler.execute(makeArgs({"TYPE", "queue"}).views, 1);
    EXPECT_EQ("+list\r\n", typeList.reply);

    auto typeNone = handler.execute(makeArgs({"TYPE", "missing"}).views, 1);
    EXPECT_EQ("+none\r\n", typeNone.reply);
}

TEST(CommandHandlerTest, XaddAndXrangeProduceStructuredOutput) {
    RedisStore store;
    CommandHandler handler(store);

    auto addReply = handler.execute(
        makeArgs({"XADD", "mystream", "1-0", "field", "value"}).views, 1);
    EXPECT_EQ("$3\r\n1-0\r\n", addReply.reply);

    auto rangeReply = handler.execute(
        makeArgs({"XRANGE", "mystream", "1-0", "1-0"}).views, 1);

    std::string expected =
        "*1\r\n"
        "*2\r\n"
        "$3\r\n1-0\r\n"
        "*2\r\n"
        "$5\r\nfield\r\n"
        "$5\r\nvalue\r\n";

    EXPECT_EQ(expected, rangeReply.reply);
}

TEST(CommandHandlerTest, XreadWithoutEntriesReturnsNullBulk) {
    RedisStore store;
    CommandHandler handler(store);

    auto reply = handler.execute(makeArgs({"XREAD", "streams", "mystream", "0-0"}).views, 1);
    EXPECT_EQ("$-1\r\n", reply.reply);
}
