#pragma once
#include <cstdint>

using byte = uint8_t;

// max cells in a map
constexpr uint16_t MAX_SIZE       = 216 * 216;
constexpr uint16_t PACKET_SIZE    = 2048;
// max size - 1 byte
constexpr uint16_t MAX_LIST_SIZE  = (PACKET_SIZE - 1) / sizeof(uint16_t);

typedef uint16_t stateId;

// opcodes
enum Opcode : byte {
   CONFIGURE   = 1,
   TERMINATE   = 10,
   UPDATE      = 20,
   DISPLAY     = 30,
   CLEAR       = 40,
   FULL_SYNC   = 90
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

struct UpdatedCell {
   stateId  id;
   Type     type;
};

union Payload {
   UpdatedCell map[PACKET_SIZE];
   IDlist      list;
};

struct Packet {
   Opcode   opcode;
   uint16_t size;
   Payload  payload;
};

char sizeFlag[] = "--set-size";
