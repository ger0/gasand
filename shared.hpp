#pragma once
#include <cstdint>

using byte = uint8_t;

constexpr double SEC2USEC  = 1'000'000;

constexpr unsigned SCREEN_WIDTH   = 800;
constexpr unsigned SCREEN_HEIGHT  = 800;
// pixels per particle
constexpr int SCALE   = 4;
constexpr unsigned MAP_WIDTH   = SCREEN_WIDTH / SCALE;
constexpr unsigned MAP_HEIGHT  = SCREEN_HEIGHT / SCALE;

constexpr unsigned MAX_SIZE       = MAP_WIDTH * MAP_HEIGHT;
constexpr unsigned MAX_LIST_SIZE  = (MAX_SIZE / sizeof(unsigned)) - 1;

// opcodes
enum Opcode : byte {
//   CONFIGURE   = 1,
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
