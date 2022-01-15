#pragma once
#include <cstdint>

using byte = uint8_t;

// max cells in a map
constexpr unsigned MAX_SIZE       = 200 * 200;
constexpr unsigned MAX_LIST_SIZE  = (MAX_SIZE / sizeof(unsigned)) - 1;

// opcodes
enum Opcode : byte {
   CONFIGURE   = 1,
   TERMINATE   = 10,
   UPDATE      = 20,
   DISPLAY     = 30,
   CLEAR       = 40
};

enum Type : byte {
   EMPTY = 0,
   WALL  = 1,
   SAND  = 2,
   GAS   = 3
};

union Payload {
   byte     map[MAX_SIZE];
   unsigned list[MAX_LIST_SIZE];
};

struct Packet {
   Opcode   opcode;
   unsigned size;
   Payload  payload;
};

char sizeFlag[] = "--set-size";
