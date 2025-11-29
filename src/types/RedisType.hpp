#pragma once

#include <iostream>
#include <variant>

#include "../db/List.hpp"
#include "../db/Stream.hpp"

enum class RedisType {STRING, LIST, STREAM};

struct RedisObj {
    RedisType type;
    std::variant<std::string, List, Stream> value;
};