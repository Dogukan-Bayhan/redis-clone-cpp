Mini Redis Clone in C++

A lightweight, educational Redis clone implemented from scratch in modern C++.
The goal of this project is to deeply understand how Redis works internally — from sockets to event loops, from RESP encoding to data structures, from blocking operations to stream mechanics.

This implementation follows a modular, extensible architecture and currently supports a growing subset of real Redis behavior.

Features Implemented So Far
RESP Protocol Support

Full parsing of RESP Arrays (*), Bulk Strings ($), Simple Strings (+), Integers (:), and Errors (-)

Clean separation between parsing, dispatching, and execution

Custom bulk string encoder (respBulk) and optimized version (valueReturnResp)

Protocol-correct responses for all implemented commands

Key-Value Command Support

PING → +PONG

ECHO <msg>

SET key value

GET key

Error handling for malformed input, missing arguments, invalid types, etc.

List Operations (Work in Progress)

Internal list representation using std::deque

LPUSH, RPUSH, LLEN, LRANGE basics working

Command handler routes list commands to dedicated handlers

Blocking List Operations (BLPOP)

Initial design + partial implementation:

Tracking blocked clients (BlockedClient structure)

Waking up clients when list becomes non-empty

Timeout handling mechanism prepared (checkTimeout stub)

Architecture is compatible with Redis-style blocking semantics

Stream (XADD / XRANGE) Foundations

You started implementing internal Redis Stream mechanics:

XADD ID parsing logic (parseIdToTwoInteger)

Validation of stream entry IDs (no 0-0, strictly increasing)

Beginning of Stream::addStream() implementation

Handling of the last entry’s timestamp/sequence logic

Stream entry storage structure (StreamEntry)

This creates the foundation for:

XADD auto-ID

XRANGE and XREVRANGE iteration

Consumer groups in the future

Command Dispatching Architecture

A clean, maintainable command handler:

Central CommandHandler::execute() that properly routes commands

All commands return RESP-encoded strings (no raw text)

ExecResult abstraction keeps response + metadata together

Non-blocking and blocking clients share a unified interface

RedisStore used as central data store (strings, lists, streams, etc.)