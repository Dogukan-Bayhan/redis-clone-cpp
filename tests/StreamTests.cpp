#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

#include "../src/db/Stream.hpp"

TEST(StreamTest, ClassifiesStreamIdFormats) {
    Stream stream;
    EXPECT_EQ(StreamIdType::AUTO_GENERATED, stream.returnStreamType("*"));
    EXPECT_EQ(StreamIdType::EXPLICIT, stream.returnStreamType("1-0"));
    EXPECT_EQ(StreamIdType::AUTO_SEQUENCE, stream.returnStreamType("1-*"));
    EXPECT_EQ(StreamIdType::INVALID, stream.returnStreamType("abc"));
}

TEST(StreamTest, RejectsNonIncreasingIds) {
    Stream stream;
    std::string err;
    std::vector<std::pair<std::string, std::string>> fields = {{"f", "1"}};

    EXPECT_TRUE(stream.validateId("1-0", err));
    stream.addStream("1-0", fields);

    EXPECT_FALSE(stream.validateId("1-0", err));
    EXPECT_FALSE(err.empty());
}

TEST(StreamTest, AutoSequenceFillsMissingSequence) {
    Stream stream;
    std::string err;
    std::vector<std::pair<std::string, std::string>> fields = {{"f", "1"}};

    stream.addStream("5-0", fields);

    std::string id = "5-*";
    EXPECT_TRUE(stream.addSequenceToId(id, err));
    EXPECT_EQ("5-1", id);
}

TEST(StreamTest, CreateUniqueIdIsMonotonic) {
    Stream stream;
    std::string err;

    std::string id1 = "*";
    EXPECT_TRUE(stream.createUniqueId(id1, err));
    stream.addStream(id1, {{"f", "x"}});

    std::string id2 = "*";
    EXPECT_TRUE(stream.createUniqueId(id2, err));

    auto parse = [](const std::string& id) {
        size_t pos = id.find('-');
        long long ms = std::stoll(id.substr(0, pos));
        long long seq = std::stoll(id.substr(pos + 1));
        return std::pair<long long, long long>{ms, seq};
    };

    auto [ms1, seq1] = parse(id1);
    auto [ms2, seq2] = parse(id2);

    EXPECT_TRUE(ms2 > ms1 || (ms2 == ms1 && seq2 > seq1));
}

TEST(StreamTest, RangeQueriesReturnExpectedEntries) {
    Stream stream;
    std::string err;

    auto fields = [](const std::string& v) {
        return std::vector<std::pair<std::string, std::string>>{{"f", v}};
    };

    stream.addStream("1-0", fields("a"));
    stream.addStream("2-0", fields("b"));
    stream.addStream("3-0", fields("c"));

    auto subset = stream.getPairsInRange(err, "1-0", "2-0");
    ASSERT_TRUE(err.empty());
    ASSERT_EQ(2u, subset.size());
    EXPECT_EQ("1-0", subset[0].first);
    EXPECT_EQ("2-0", subset[1].first);
    EXPECT_EQ("a", subset[0].second[0].second);

    auto fromStart = stream.getPairsFromStartToId(err, "2-0");
    ASSERT_TRUE(err.empty());
    ASSERT_EQ(2u, fromStart.size());

    auto toEnd = stream.getPairsFromIdToEnd(err, "2-0");
    ASSERT_TRUE(err.empty());
    ASSERT_EQ(2u, toEnd.size());
    EXPECT_EQ("2-0", toEnd[0].first);
    EXPECT_EQ("3-0", toEnd[1].first);
}
