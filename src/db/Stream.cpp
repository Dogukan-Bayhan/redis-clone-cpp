#include "./Stream.hpp"
#include <algorithm>

/*
===============================================================================
  returnStreamType()
-------------------------------------------------------------------------------
  Determines which Redis Stream ID format the user provided.

  Supported formats:
    1) "*"                  → AUTO_GENERATED
    2) "<ms>-<seq>"         → EXPLICIT
    3) "<ms>-*"             → AUTO_SEQUENCE
  Redis XADD semantics:
    * EXPLICIT: full ID specified → must be strictly increasing
    * AUTO_SEQUENCE: timestamp fixed, sequence auto-generated
    * AUTO_GENERATED: timestamp & sequence generated automatically

  Return:
    A StreamIdType enum representing the ID category.
===============================================================================
*/
StreamIdType Stream::returnStreamType(const std::string &id)
{

    if (id == "*")
        return StreamIdType::AUTO_GENERATED;

    size_t pos = id.find('-');
    if (pos == std::string::npos)
    {
        return StreamIdType::INVALID;
    }

    std::string left = id.substr(0, pos);
    std::string right = id.substr(pos + 1);

    // "*-*" form
    if (left == "*" && right == "*")
        return StreamIdType::AUTO_GENERATED;

    // "<ms>-*" form
    if (left != "*" && right == "*")
    {

        for (char c : left)
        {
            if (!isdigit(c))
                return StreamIdType::INVALID;
        }

        return StreamIdType::AUTO_SEQUENCE;
    }

    // Check numeric left and right → EXPLICIT
    bool left_number = !left.empty() &&
                       std::all_of(left.begin(), left.end(), ::isdigit);

    bool right_number = !right.empty() &&
                        std::all_of(right.begin(), right.end(), ::isdigit);

    if (left_number && right_number)
        return StreamIdType::EXPLICIT;

    return StreamIdType::INVALID;
}

/*
===============================================================================
  parseIdToTwoInteger()
-------------------------------------------------------------------------------
  Parses a Redis-style ID of format "ms-seq" into two integers:

      ms  → millisecond timestamp (long long)
      seq → sequence number       (long long)

  Returns:
      true   → parsing succeeded
      false  → malformed input or conversion error

  Purpose:
      A common helper for ValidateID, XADD logic, and sequence generation.
===============================================================================
*/
bool Stream::parseIdToTwoInteger(const std::string &id, long long &ms, long long &seq)
{
    size_t pos = id.find('-');

    if (pos == std::string::npos)
        return false;

    try
    {
        ms = std::stoll(id.substr(0, pos));
        seq = std::stoll(id.substr(pos + 1));
    }
    catch (...)
    {
        return false;
    }

    return true;
}

/*
===============================================================================
  validateId()
-------------------------------------------------------------------------------
  Ensures that a given EXPLICIT ID obeys Redis ordering rules.

  Rules:
    • ID must be of form "ms-seq"
    • ID must be > last_id in the stream
    • ID cannot be "0-0"

  Errors match Redis error messages for strict correctness.

  Return:
    true   → ID valid, safe to append
    false  → assign error message and reject
===============================================================================
*/
bool Stream::validateId(const std::string &id, std::string &err)
{
    long long ms, seq;

    if (!parseIdToTwoInteger(id, ms, seq))
    {
        err = "-ERR The ID specified in XADD is equal or smaller than the target stream top item\r\n";
        return false;
    }

    if (ms == 0 && seq == 0)
    {
        err = "-ERR The ID specified in XADD must be greater than 0-0\r\n";
        return false;
    }

    if (entries.empty())
        return true;

    // Get last entry
    StreamEntry &lastStream = entries.back();

    long long last_ms, last_seq;

    parseIdToTwoInteger(lastStream.id, last_ms, last_seq);

    if (ms < last_ms)
    {
        err = "-ERR The ID specified in XADD is equal or smaller than the target stream top item\r\n";
        return false;
    }

    if (ms == last_ms && seq <= last_seq)
    {
        err = "-ERR The ID specified in XADD is equal or smaller than the target stream top item\r\n";
        return false;
    }

    return true;
}


/*
===============================================================================
  addStream()
-------------------------------------------------------------------------------
  Appends a new entry to the stream.

  NOTE:
      This function assumes the ID is already validated or generated.
      It simply inserts the entry and updates the index map.

  Behavior:
      - Appends {id, fields} to entries vector
      - Inserts id → index in O(1)

  Return:
      The final ID (Redis returns the ID as success response).
===============================================================================
*/
std::string Stream::addStream(const std::string &id, const std::vector<std::pair<std::string, std::string>> &fields)
{
    StreamEntry entry;
    entry.id = id;
    entry.fields = fields;

    // Fill ms/seq
    parseIdToTwoInteger(id, entry.ms, entry.seq);

    entries.push_back(entry);
    idToIndex[id] = entries.size() - 1;
    return id;
}


/*
===============================================================================
  getById()
-------------------------------------------------------------------------------
  Fast O(1) lookup of a stream entry by its ID.

  Uses:
      idToIndex → unordered_map<string, index>

  Returns:
      pointer to StreamEntry  → found
      nullptr                 → ID not present
===============================================================================
*/
const StreamEntry *Stream::getById(const std::string &id)
{
    auto it = idToIndex.find(id);
    if (it == idToIndex.end())
        return nullptr;

    return &entries[it->second];
}


/*
===============================================================================
  addSequenceToId()
-------------------------------------------------------------------------------
  Handles ID format: "<ms>-*"

  Redis semantics:
     • If stream empty       → "<ms>-1"
     • Else:
          if ms < last_ms   → ERR
          if ms > last_ms   → seq = 0
          if ms == last_ms  → seq = last_seq + 1

  Converts the wildcard sequence '*' into the correct integer and rewrites ID.

  Return:
      true  → success
      false → assign error msg
===============================================================================
*/
bool Stream::addSequenceToId(std::string &id, std::string &err)
{
    size_t pos = id.find('-');
    if (pos == std::string::npos)
    {
        err = "-ERR The ID specified in XADD is equal or smaller than the target stream top item\r\n";
        return false;
    }

    std::string ms_str = id.substr(0, pos);
    long long new_ms;

    try
    {
        new_ms = std::stoll(ms_str);
    }
    catch (...)
    {
        err = "-ERR The ID specified in XADD is equal or smaller than the target stream top item\r\n";
        return false;
    }

    // Stream empty → "<ms>-1"
    if (entries.empty())
    {
        id = ms_str + "-1";
        return true;
    }

    // Compare to last entry
    StreamEntry &last = entries.back();

    long long last_ms, last_seq;
    if (!parseIdToTwoInteger(last.id, last_ms, last_seq))
    {
        err = "-ERR The ID specified in XADD is equal or smaller than the target stream top item\r\n";
        return false;
    }

    if (new_ms < last_ms)
    {
        err = "-ERR The ID specified in XADD is equal or smaller than the target stream top item\r\n";
        return false;
    }

    long long new_seq;

    if (new_ms > last_ms)
    {
        new_seq = 0;
    }
    else
    {
        new_seq = last_seq + 1;
    }

    id = ms_str + "-" + std::to_string(new_seq);
    return true;
}

/*
===============================================================================
  createUniqueId()
-------------------------------------------------------------------------------
  Handles AUTO_GENERATED ("*") ID mode.

  Redis logic:
      now_ms = current Unix timestamp

      Case A: stream empty
          id = "<now_ms>-0"

      Case B: now_ms > last_ms
          id = "<now_ms>-0"

      Case C: now_ms == last_ms
          id = "<last_ms>-(last_seq + 1)"

      Case D: now_ms < last_ms
          (clock moved backwards)
          id = "<last_ms>-(last_seq + 1)"
          This preserves monotonically increasing IDs.

  Result:
      Stream ID is always increasing even if system clock changes.
===============================================================================
*/
bool Stream::createUniqueId(std::string &id, std::string& err)
{
    long long now_ms = getUnixTimeMs();

    if (entries.empty())
    {
        id = std::to_string(now_ms) + "-0";
        return true;
    }

    StreamEntry &last = entries.back();

    long long last_ms, last_seq;

    if (!parseIdToTwoInteger(last.id, last_ms, last_seq))
    {
        err = "-ERR The ID specified in XADD is equal or smaller than the target stream top item\r\n";
        return false;
    }

    long long new_ms, new_seq;

    if (now_ms > last_ms)
    {
        new_ms = now_ms;
        new_seq = 0;
    }
    else if (now_ms == last_ms)
    {
        new_ms = last_ms;
        new_seq = last_seq + 1;
    }
    else
    {
        // monotonic guarantee (clock moved backwards)
        new_ms = last_ms;
        new_seq = last_seq + 1;
    }

    id = std::to_string(new_ms) + "-" + std::to_string(new_seq);
    return true;
}
