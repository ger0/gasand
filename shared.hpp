#pragma once

typedef unsigned char byte;

const unsigned SCREEN_WIDTH   = 800;
const unsigned SCREEN_HEIGHT  = 800;
// pixels per particle
const int SCALE   = 4;
const unsigned MAP_WIDTH   = SCREEN_WIDTH / SCALE;
const unsigned MAP_HEIGHT  = SCREEN_HEIGHT / SCALE;

const unsigned MAX_SIZE       = MAP_WIDTH * MAP_HEIGHT;
const unsigned MAX_LIST_SIZE  = (MAX_SIZE / sizeof(unsigned)) - 1;

// opcodes
// chec otrzymania konfiguracji 
// wysylanie checi zakonczenia dzialania
// wysylanie zmian do serwera
// wysylanie stanu mapy
enum Opcode : byte {
   CONFIGURE   = 1,
   TERMINATE   = 10,
   UPDATE      = 20,
   DISPLAY     = 30
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
