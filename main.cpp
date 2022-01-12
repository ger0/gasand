#include <SDL2/SDL.h>
#include <cstdio>

#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>

#include <ctime>
#include <vector>

#include "shared.hpp"

// game state
bool isRunning    = true;

// descriptors for clients
std::vector<int> clients;

// map state to send
Type state[MAP_WIDTH * MAP_HEIGHT] = {EMPTY};

void pushToState(unsigned *arr, unsigned &max_iter) {
   Type type = EMPTY;
   for (unsigned i = 0; i < max_iter; i++) {
      if (i >= MAX_SIZE) {
         printf("ERROR DURING A PUSH_TO_STATE: pushing invalid index - %u \n", i);
      } else { 
         // set type
         if (i == 0)
            type = (Type)arr[i];
         else
            state[arr[i]] = type;
      }
   }
}

void readPacket(Packet &packet, int fd) {
   if (packet.opcode == UPDATE) {
      // list of updated cells
      unsigned max_iter = packet.size / sizeof(unsigned);
      pushToState(packet.payload.list, max_iter);

   } else if (packet.opcode == TERMINATE) {
      for (auto it = clients.begin(); it != clients.end(); ) {
         if (*it == fd)
            clients.erase(it);
      }
      close(fd);
   }
}

Packet preparePacket(Opcode opcode) {
   Packet packet;
   packet.opcode = opcode;
   if (opcode == DISPLAY) {
      memcpy(packet.payload.map, state, MAX_SIZE);
   }
   return packet;
}

inline Type *stateGet(int x, int y) {
   if (x >= MAP_WIDTH || y >= MAP_HEIGHT ||
         x < 0 || y < 0) {
      return nullptr;
   }
   return &state[x + y * (MAP_WIDTH)];
}


void mapStateUpdate() {
   // iterating over entire map
   for (int y = MAP_HEIGHT - 1; y >= 0; y--) {
      for (int x = 0; x < MAP_WIDTH; x++) {

         Type *cell = stateGet(x, y);
         if (cell == nullptr) {
            printf("ERROR DURING UPDATING: WRONG ID\n");
         }

         // update sand gravity
         if (*cell == SAND) {

            int mov = rand() % 3 - 1;
            Type *neighbour = stateGet(x, y + 1);

            for (unsigned i = 0; i < 2; i++) {
               if (neighbour == nullptr) {
                  break;

               /*
               } else if (i == 0 && neighbour->type == WALL) {
                  break;
               */

               } else {
                 neighbour = stateGet(x + (i * mov), y + 1);
               }

               if (neighbour != nullptr && *neighbour == EMPTY) {
                  *neighbour = *cell;
                  *cell = EMPTY;
               }
            }
         }
         // update gas state
         else if (*cell == GAS) {

            int x_ran = rand() % 3 - 1;
            int y_ran = rand() % 3 - 1;

            Type *neighbour = stateGet(x, y - 1);

            if (neighbour == nullptr) {
               continue; 

            } else if (*neighbour == SAND) {
               Type temp = *neighbour;

               *neighbour = *cell;
               *cell      = temp;

               continue;

            } else if (*neighbour == WALL && y_ran < 0) {
               continue;
            }

            neighbour = stateGet(x + x_ran, y + y_ran);

            if (neighbour != nullptr && *neighbour == EMPTY) {
               *neighbour = *cell;
               *cell = EMPTY; 
            }
         }
      }
   }
}

void cleanUp(SDL_Window *win, SDL_Renderer *ren) {
   SDL_DestroyWindow(win);
   SDL_DestroyRenderer(ren);
   SDL_Quit();

   printf("Exiting the game...\n");
}
void handleEvents(SDL_Event *e) {
   while (SDL_PollEvent(e) > 0) {
      switch(e->type) {
         case SDL_QUIT:
            isRunning = false;
            break;
      }
   }
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

int main(int argc, char* argv[]) {
   if (argc != 2) {
      printf("Usage: %s <port>", argv[0]);
      return -11;
   }
   sockaddr_in localAddress{
      .sin_family = AF_INET,
      .sin_port   = htons(atoi(argv[1])),
      .sin_addr   = {htonl(INADDR_ANY)}
   };

   int servSock = socket(PF_INET, SOCK_STREAM, 0);

   int res = bind(servSock, (sockaddr*)&localAddress, sizeof(localAddress));
   if (res) {
      printf("Bind failed!\n");
      return -12;
   }
   
   listen(servSock, 4);
   
   // no block
   fcntl(servSock, F_SETFL, fcntl(servSock, F_GETFL) | O_NONBLOCK);

   srand(time(NULL));
   SDL_Window     *window;
   SDL_Renderer   *renderer;
   SDL_Surface    *window_surface;

   if (SDL_Init(SDL_INIT_VIDEO) < 0) {
      printf("Failed to initialize the SDL2 library\n");
      return -1;
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
      return -2;
   }

   renderer = SDL_CreateRenderer(window, -1, 0);
   if (renderer) {
      SDL_SetRenderDrawColor(renderer, 38, 38, 38, 255);
         printf("Created renderer\n");
   }

   window_surface = SDL_GetWindowSurface(window);
   if (!window_surface) {
      printf("Failed to create window surface");
      return -3;
   }
      
   int clientSock;

   SDL_Event event;
   // game loop
   Packet packet;

   while (isRunning) {
      // trying to accept a new client
      clientSock = accept(servSock, nullptr, nullptr);
      if (clientSock != -1) {
         fcntl(clientSock, F_SETFL, fcntl(clientSock, F_GETFL) | O_NONBLOCK);
         clients.push_back(clientSock);
         printf("Accepted a new connection\n");
      }

      // read from clients
      for (int sock : clients) {
         int res = read(sock, &packet, sizeof(Packet));
         if (res > 0)
            readPacket(packet, sock);
      }

      handleEvents(&event);
      mapStateUpdate();

      // sending a state of a map
      packet = preparePacket(DISPLAY);
      for (int sock : clients) {
         write(sock, &packet, sizeof(Packet));
      }
      // debug
      renderMap(window, renderer);

      // TODO: tickrate 
      SDL_Delay(16);
   }
   for (int sock : clients) {
      close(sock);
   }
   // clean up 
   cleanUp(window, renderer);
   return 0;
}
