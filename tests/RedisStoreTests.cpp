#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include "../src/db/RedisStore.hpp"
#include "../src/db/List.hpp"

TEST(RedisStoreTest, SetAndGetStringValue) {
    RedisStore store;
    store.setString("foo", "bar");

    std::string out;
    EXPECT_TRUE(store.getString("foo", out));
    EXPECT_EQ("bar", out);
}

TEST(RedisStoreTest, SetWithTtlExpires) {
    RedisStore store;
    store.setString("temp", "value", 5);

    std::this_thread::sleep_for(std::chrono::milliseconds(15));

    std::string out;
    EXPECT_FALSE(store.getString("temp", out));
}

TEST(RedisStoreTest, DeleteRemovesValueAndTtl) {
    RedisStore store;
    store.setString("foo", "bar", 2000);
    EXPECT_TRUE(store.del("foo"));

    std::string out;
    EXPECT_FALSE(store.getString("foo", out));
}

TEST(ListTest, PushBackAndRange) {
    List list;
    list.PushBack("one");
    list.PushBack("two");
    list.PushBack("three");

    auto elements = list.GetElementsInRange(0, 2);
    ASSERT_EQ(3u, elements.size());
    EXPECT_EQ("one", elements[0]);
    EXPECT_EQ("two", elements[1]);
    EXPECT_EQ("three", elements[2]);
}

TEST(ListTest, SupportsNegativeIndices) {
    List list;
    list.PushBack("a");
    list.PushBack("b");
    list.PushBack("c");
    list.PushBack("d");

    auto elements = list.GetElementsInRange(-3, -1);
    ASSERT_EQ(3u, elements.size());
    EXPECT_EQ("b", elements[0]);
    EXPECT_EQ("c", elements[1]);
    EXPECT_EQ("d", elements[2]);
}

TEST(ListTest, PopOperationsHandleEmptyList) {
    List list;
    EXPECT_EQ("", list.POPFront());
    EXPECT_EQ("", list.POPBack());

    list.PushFront("front");
    list.PushBack("back");

    EXPECT_EQ("front", list.POPFront());
    EXPECT_EQ("back", list.POPBack());
    EXPECT_TRUE(list.Empty());
}
