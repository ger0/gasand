#include <SDL2/SDL.h>
#include <cstdio>
#include <ctime>
#include <cassert>

#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <vector>

#include "shared.hpp"

bool isRunning = true;

// mouse brush
Type brush_state = EMPTY;
bool isMousePressed  = false;
struct {int x = 0; int y = 0;} mousePos;

// list of updated cells sent to server
std::vector<unsigned> updatedCells;

// map state received from server 
Type inState[MAP_WIDTH * MAP_HEIGHT] = {EMPTY};

inline unsigned getId(int x, int y) {
   return unsigned(x) + unsigned(y * MAP_HEIGHT);
}

void readPacket(Packet &packet) {
   if (packet.opcode == DISPLAY) {
      memcpy(inState, packet.payload.map, MAX_SIZE);
   }
}

Packet preparePacket(Opcode opcode) {
   Packet packet;
   packet.opcode = opcode;

   if (opcode == UPDATE) {
      packet.payload.list[0] = brush_state;
      unsigned size = updatedCells.size() * sizeof(unsigned);

      if (size > MAX_SIZE - 1) {
         // saving 4 bytes for a brush
         memcpy(packet.payload.list + 1, updatedCells.data(), MAX_SIZE - sizeof(unsigned));
         packet.size = MAX_SIZE;

      } else {
         memcpy(packet.payload.list + 1, updatedCells.data(), size);
         packet.size = size;
      }
   }
   return packet;
}

void statePutCell(int &x, int &y) {
   if (x >= 0 && y >= 0 &&
         x < MAP_WIDTH && y < MAP_HEIGHT) {
      updatedCells.push_back(getId(x, y));
   }
}

void tryDrawing() {
   if (isMousePressed) {
      for (int x = mousePos.x - 1; x <= mousePos.x; x++) {
         for (int y = mousePos.y - 1; y <= mousePos.y; y++) {
            statePutCell(x, y);
         }
      }
   }
}

void handleEvents(SDL_Event *e) {
   while (SDL_PollEvent(e) > 0) {
      switch(e->type) {
         case SDL_QUIT:
            isRunning = false;
            break;

         case SDL_MOUSEBUTTONDOWN:
            isMousePressed = true;
         case SDL_MOUSEMOTION:
            mousePos.x = e->motion.x / SCALE;
            mousePos.y = e->motion.y / SCALE;
            tryDrawing();
            //printf("Mouse moved to (%d, %d)\n", mousePos.x, mousePos.y);
            break;

         case SDL_MOUSEBUTTONUP:
            isMousePressed = false;
            break;

         case SDL_KEYDOWN:
            //printf("Scancode: 0x%02X\n", e->key.keysym.scancode);
            if (e->key.keysym.scancode == 0x1A) {
               brush_state = WALL;
            } else if (e->key.keysym.scancode == 0x16) {
               brush_state = SAND;
            } else if (e->key.keysym.scancode == 0x0A) {
               brush_state = GAS;
            } else {
               brush_state = EMPTY;
            }
            break;

      }
   }
}

inline Type *stateGet(int x, int y) {
   if (x >= MAP_WIDTH || y >= MAP_HEIGHT ||
         x < 0 || y < 0) {
      return nullptr;
   }
   return &inState[x + y * (MAP_WIDTH)];
}

void renderMap(SDL_Window *window, SDL_Renderer *renderer) {
   for (int y = 0; y < MAP_HEIGHT; y++) {
      for (int x = 0; x < MAP_WIDTH; x++) {
         // drawing the cell
         SDL_Rect rect = {x * SCALE, y * SCALE, SCALE, SCALE};
         
         SDL_Colour col;
         Type *type = stateGet(x, y);

         if (*type == EMPTY) {
            col = SDL_Colour{0, 0, 0, 255};

         } else if (*type == WALL) {
            col = SDL_Colour{100, 100, 100, 255};

         } else if (*type == SAND) {
            col = SDL_Colour{255, 255, 50, 255};

         } else if (*type == GAS) {
            col = SDL_Colour{50, 20, 100, 255};

         // debug
         } else {
            col = SDL_Colour{0, 255, 0, 255};
         }
         SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, col.a);
         SDL_RenderFillRect(renderer, &rect);
      }
   }
   SDL_RenderPresent(renderer);
}

// RAII
template<typename T, auto Destructor>
struct Scope_Handle {
   T* ptr;
   Scope_Handle() {
      ptr = nullptr;
   }
   Scope_Handle(Scope_Handle&& rhs) {
      this->ptr = nullptr;
      *this = move(rhs);
   }
   ~Scope_Handle() {
      reset();
   }
   Scope_Handle& operator=(T* rhs) {
      assert(ptr == nullptr);
      ptr = rhs;
      return *this;
   }
   Scope_Handle& operator=(Scope_Handle&& rhs) {
      assert(ptr == nullptr);
      ptr = rhs.ptr;
      rhs.ptr = nullptr;
      return *this;
   }
   void reset() {
      if (ptr != nullptr) {
         Destructor(ptr);
         ptr = nullptr;
      }
   }
   operator T*()    const { return ptr; }
   T* operator->()  const { return ptr; }
};

int main(int argc, char* argv[]) {
   if (argc != 3) {
      printf("Incorrect usage, please input address and port:\n");
      printf("%s <address> <port>\n", argv[0]);
      return -10;
   }

   addrinfo hints {};
   hints.ai_family   = AF_INET;
   hints.ai_protocol = IPPROTO_TCP;
   addrinfo *resolved;

   if (int err = getaddrinfo(argv[1], argv[2], &hints, &resolved)) {
      printf("Resolving address failed: %s\n", gai_strerror(err));
      return -1;
   }

   int sock = socket(resolved->ai_family, resolved->ai_socktype, resolved->ai_protocol);
   if (connect(sock, resolved->ai_addr, resolved->ai_addrlen)) {
      printf("Failed to connect\n");
      return -2;
   }
   freeaddrinfo(resolved);

   Scope_Handle<SDL_Window,   SDL_DestroyWindow>   window;
   Scope_Handle<SDL_Renderer, SDL_DestroyRenderer> renderer;
   SDL_Surface    *window_surface;

   if (SDL_Init(SDL_INIT_VIDEO) < 0) {
      printf("Failed to initialize the SDL2 library\n");
      return -3;
   }

   window = SDL_CreateWindow(
         "gasand",
         SDL_WINDOWPOS_CENTERED,
         SDL_WINDOWPOS_CENTERED,
         SCREEN_WIDTH, SCREEN_HEIGHT,
         0
   );
   if (!window) {
      printf("Failed to create window\n");
      return -4;
   }

   renderer = SDL_CreateRenderer(window, -1, 0);
   if (renderer) {
      SDL_SetRenderDrawColor(renderer, 38, 38, 38, 255);
         printf("Created renderer\n");
   }

   window_surface = SDL_GetWindowSurface(window);
   if (!window_surface) {
      printf("Failed to create window surface");
      return -5;
   }
      
   SDL_Event event;
   // game loop
   Packet packet;
   while (isRunning) {
      // handle events
      handleEvents(&event);
      
      // again...
      tryDrawing();
      // try to send updates
      if (updatedCells.size() > 0) { 
         packet = preparePacket(UPDATE);
         write(sock, &packet, sizeof(Packet));
         updatedCells.clear();
      }
      // read a new state
      read(sock, &packet, sizeof(Packet));
      readPacket(packet);
      renderMap(window, renderer);
      // todo sync
   }
   // close socket
   packet = preparePacket(TERMINATE);
   write(sock, &packet, sizeof(Packet));
   close(sock);
   SDL_Quit();
   // clean up 
   return 0;
}
