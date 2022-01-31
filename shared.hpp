#pragma once
#include <cstdint>

using byte = uint8_t;

// max cells in a map
constexpr uint32_t MAX_SIZE       = 200 * 200;
// max size - 1 byte
constexpr uint32_t MAX_LIST_SIZE  = (MAX_SIZE - 1) / sizeof(uint32_t);

typedef uint32_t stateId;

// opcodes
enum Opcode : byte {
   CONFIGURE   = 1,
   TERMINATE   = 10,
   UPDATE      = 20,
   DISPLAY     = 30,
   CLEAR       = 40,
   PAUSE       = 100
};

enum Type : byte {
   EMPTY = 0,
   WALL  = 1,
   SAND  = 2,
   GAS   = 3
};

struct IDlist {
   Type        brushType;
   stateId    data[MAX_LIST_SIZE];
};

union Payload {
   Type     map[MAX_SIZE];
   IDlist   list;
};

struct Packet {
   Opcode   opcode;
   uint32_t size;
   Payload  payload;
};

char sizeFlag[] = "--set-size";
