#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>

#include "../utils/time.cpp"

/*
------------------------------------------------------------------------------
  STREAM ID TYPES (Redis-Compatible Semantics)
------------------------------------------------------------------------------

Redis streams use three different ID input modes:

1) EXPLICIT
     User provides an exact ID such as "1526919030474-0".
     Server must validate:
        - ID format (ms-seq)
        - ID > last_entry_id

2) AUTO_SEQUENCE (ID ends with "-*")
     Example: "1526919030474-*"
     Meaning:
        - Timestamp is fixed
        - Sequence will be auto-incremented based on previous entries
     Used rarely but Redis supports it.

3) AUTO_GENERATED ("*")
     Redis generates both timestamp and sequence automatically:
        - ms  = current Unix time in ms
        - seq = (last_entry.ms == ms) ? last_seq+1 : 0
     This is the default XADD behavior.

4) INVALID
     Any malformed or non-parsable ID string.

This enum allows the command handler to perform correct ID logic
before mutating the stream.
------------------------------------------------------------------------------
*/
enum class StreamIdType {
    EXPLICIT,
    AUTO_SEQUENCE,
    AUTO_GENERATED,
    INVALID
};

/*
------------------------------------------------------------------------------
  STREAM ENTRY
------------------------------------------------------------------------------

Each entry holds:
    id     → unique Redis-style ID ("ms-seq")
    fields → list of key-value pairs (similar to a small hash)

A Redis Stream is conceptually:
    Ordered append-only log of StreamEntry items.

The fields vector keeps insertion order, matching Redis behavior.
------------------------------------------------------------------------------
*/
struct StreamEntry {
    std::string id;
    long long ms;
    long long seq;
    std::vector<std::pair<std::string, std::string>> fields;
};

/*
------------------------------------------------------------------------------
  STREAM CLASS (Minimal Redis Stream Implementation)
------------------------------------------------------------------------------

CORE RESPONSIBILITIES:

  • Validate and parse IDs (ms-seq format)
  • Append entries while maintaining strict ordering
  • Generate IDs for AUTO mode
  • Provide random access via ID → index lookup (idToIndex map)
  • Provide a clean interface for XADD / XRANGE / XREVRANGE

DATA STRUCTURES:

  entries:      vector of StreamEntry (append-only)
  idToIndex:    map from string ID → position in entries vector
                Enables O(1) lookup for XRANGE and similar ops.

PRIVATE FUNCTIONALITY:
  parseIdToTwoInteger()
      Converts a "ms-seq" string into two integers.

PUBLIC API:
  • returnStreamType()   → Classifies given ID
  • validateId()         → Ensures ID ordering rules
  • addStream()          → Main XADD logic
  • getById()            → Fetch entry by ID
  • addSequenceToId()    → Handles "ms-*"
  • createUniqueId()     → Handles "*"

------------------------------------------------------------------------------
*/
class Stream {
private:
    std::vector<StreamEntry> entries;
    std::unordered_map<std::string, std::size_t> idToIndex;

    // Parses "ms-seq" into two long long integers.
    // Returns true on success, false on malformed input.
    bool parseIdToTwoInteger(const std::string&, long long&, long long&);

public:
    // Determines the type of ID supplied by the user.
    StreamIdType returnStreamType(const std::string& id);
   
    // Validates whether an ID is syntactically correct
    // AND maintains strict monotonic ordering relative to last entry.
    bool validateId(const std::string& id, std::string& err);
    
    // Appends a new entry to the stream.
    // Handles EXPLICIT, AUTO_SEQUENCE, AUTO_GENERATED.
    std::string addStream(const std::string& id,const std::vector<std::pair<std::string,std::string>>& fields);    
    
    // Fetches an entry by its ID in O(1).
    const StreamEntry* getById(const std::string& id);
    
    // Handles "ms-*". Generates the smallest valid next sequence number.
    bool addSequenceToId(std::string& id, std::string& err);
    
    // Handles "*" (AUTO_GENERATED).
    // Generates a unique ID based on Unix time + sequence logic.
    bool createUniqueId(std::string& id, std::string& err);

    std::vector<std::pair<std::string, std::string>> getPairsInRange();
};