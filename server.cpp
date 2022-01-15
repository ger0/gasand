#include <cstdio>
#include <cstring>
#include <ctime>
#include <cstdint>

#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>

#include <vector>
#include <algorithm>

#include <csignal>

#include "shared.hpp"

// default values to be set later
uint32_t MAP_WIDTH   = 200;
uint32_t MAP_HEIGHT  = 200;

constexpr double TICK_RATE = 50;

// period in microseconds
constexpr double SEC2USEC = 1'000'000;
constexpr double PERIOD =  SEC2USEC / TICK_RATE;

// game state
bool isRunning    = true;
// server socket
int servSock;
// descriptors for clients
std::vector<int> clients;

// map state to send
Type state[MAX_SIZE]          = {};
// gas update guard
bool updatedState[MAX_SIZE]   = {};

void pushToState(IDlist &list, unsigned &max_iter) {
   Type type = list.brushType;

   for (unsigned i = 0; i < max_iter; i++) {
      if (list.data[i] >= MAX_SIZE) {
         printf("INVALID INDEX DURING A PUSH_TO_STATE: %u \n", list.data[i]);

      } else { 
         state[list.data[i]] = type;
      }
   }
}

void readPacket(Packet &packet, int fd) {
   if (packet.opcode == UPDATE) {
      // list of updated cells
      unsigned max_iter = packet.size / sizeof(stateId);
      pushToState(packet.payload.list, max_iter);

   } else if (packet.opcode == TERMINATE) {
      clients.erase(std::remove(clients.begin(), clients.end(), fd), clients.end());
      close(fd);
      printf("Client disconnected...\n");

   } else if (packet.opcode == CLEAR) {
      for (uint32_t i = 0; i < MAX_SIZE; i++) {
         state[i] = EMPTY;
      }
   }
}

Packet preparePacket(Opcode opcode) {
   Packet packet;
   packet.opcode = opcode;
   if (opcode == DISPLAY) {
      memcpy(packet.payload.map, state, MAX_SIZE);

   } else if (opcode == CONFIGURE) {
      packet.payload.list.data[0] = MAP_WIDTH;
      packet.payload.list.data[1] = MAP_HEIGHT;
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

inline bool *stateUpdatedGet(int x, int y) {
   if (x >= MAP_WIDTH || y >= MAP_HEIGHT ||
         x < 0 || y < 0) {
      return nullptr;
   }
   return &updatedState[x + y * (MAP_WIDTH)];
}

void mapStateUpdate() {
   // iterating over entire map
   for (int y = MAP_HEIGHT - 1; y >= 0; y--) {
      for (int x = 0; x < MAP_WIDTH; x++) {
         Type *cell        = stateGet(x, y);
         bool *cellUpdated = stateUpdatedGet(x, y);
         Type *neighbour   = nullptr;
         
         // update sand gravity
         if (*cell == SAND) {
            // choose random direction
            int mov = rand() % 3 - 1;
            neighbour = stateGet(x, y + 1);

            for (uint32_t i = 0; i < 2; i++) {
               if (neighbour != nullptr) {
                  neighbour = stateGet(x + (i * mov), y + 1);

                  if (neighbour != nullptr && *neighbour == EMPTY) {
                     *neighbour = *cell;
                     *cell = EMPTY;
                  }
               }
            }
         } else if (*cell == GAS) {
            // random direction
            int x_ran = rand() % 3 - 1;
            int y_ran = rand() % 3 - 1;

            // check the cell above 
            neighbour = stateGet(x, y - 1);

            if (neighbour != nullptr && *neighbour == SAND) {
               Type temp = *neighbour;

               *neighbour = *cell;
               *cell      = temp;

            // check if a random neighbour is accessible
            } else {
               neighbour = stateGet(x + x_ran, y + y_ran);

               if (neighbour != nullptr && !(*neighbour == WALL && y_ran < 0)) {
                  cellUpdated = stateUpdatedGet(x + x_ran, y + y_ran);

                  // check if the neighbour has been updated before
                  if (*neighbour == EMPTY && *cellUpdated == false) {
                     *neighbour = *cell;
                     *cell = EMPTY; 
                     *cellUpdated = true;
                  }
               }
            }
         }
      }
   }
   for (uint32_t i = 0; i < MAX_SIZE; i++) {
      updatedState[i] = false;
   }
}

void closeProgram(int sig) {
   isRunning = false;
}

int main(int argc, char* argv[]) {
   // check for "--set-size"
   if (argc == 5 && strcmp(sizeFlag, argv[2]) == 0) { 
      uint32_t width    = atoi(argv[3]);
      uint32_t height   = atoi(argv[4]);

      if (!(width <= 0  || width > 200 ||
            height <= 0 || height > 200)) {
         MAP_WIDTH   = width;
         MAP_HEIGHT  = height;
      } else {
         printf("Map dimensions should be between 1 and 200\n");
         printf("Setting to default dimensions 200 x 200...\n");
      }
   } else if (argc != 2) {
      printf("Usage: %s <port>\n\t", argv[0]);
      printf("or: %s <port> %s <width> <height>\n", argv[0], sizeFlag);
      return -11;
   } 

   signal(SIGINT, closeProgram);

   sockaddr_in localAddress {
      .sin_family = AF_INET,
      .sin_port   = htons(atoi(argv[1])),
      .sin_addr   = {htonl(INADDR_ANY)}
   };

   servSock = socket(PF_INET, SOCK_STREAM, 0);

   int res = bind(servSock, (sockaddr*)&localAddress, sizeof(localAddress));
   if (res) {
      printf("Bind failed!\n");
      return -12;
   }
   listen(servSock, 4);
   
   // no block
   fcntl(servSock, F_SETFL, fcntl(servSock, F_GETFL) | O_NONBLOCK);

   srand(time(NULL));
      
   int clientSock;

   // game loop
   Packet packet;
   clock_t timePoint = clock();
   double  deltaTime = 0.0;

   while (isRunning) {
      timePoint = clock();

      // trying to accept a new client
      clientSock = accept(servSock, nullptr, nullptr);
      if (clientSock != -1) {
         fcntl(clientSock, F_SETFL, fcntl(clientSock, F_GETFL) | O_NONBLOCK);
         clients.push_back(clientSock);
         // send map dimensions 
         packet = preparePacket(CONFIGURE);
         write(clientSock, &packet, sizeof(Packet));

         printf("Accepted a new connection\n");
      }

      // read from clients
      for (int sock : clients) {
         while (read(sock, &packet, sizeof(Packet)) > 0) {
            readPacket(packet, sock);
         }
      }
      mapStateUpdate();

      // sending a state of a map
      packet = preparePacket(DISPLAY);
      for (int sock : clients) {
         write(sock, &packet, sizeof(Packet));
      }

      // tick
      timePoint = clock() - timePoint;
      deltaTime = PERIOD - (double)timePoint * SEC2USEC / CLOCKS_PER_SEC;
      if (deltaTime > 0.0) {
         usleep(deltaTime);
      } else {
         printf("SERVER LAGGING BEHIND: %f\n", -deltaTime);
      }
   }
   // clean up 
   packet = preparePacket(TERMINATE);
   for (int sock : clients) {
      write(sock, &packet, sizeof(Packet));
      close(sock);
   }
   printf("Closing the server...\n");
   close(servSock);
   return 0;
}
