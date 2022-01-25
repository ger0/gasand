#include <cstdio>
#include <cstring>
#include <cstdint>

#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>

#include <vector>
#include <algorithm>
#include <random>

#include <csignal>

#include "shared.hpp"

// default values to be set later
uint16_t MAP_WIDTH   = 200;
uint16_t MAP_HEIGHT  = 200;

constexpr double TICK_RATE = 50;

// period in microseconds
constexpr double SEC2USEC = 1'000'000;
constexpr double PERIOD =  SEC2USEC / TICK_RATE;

// socket descriptors
int servSock;
std::vector<int> clients;

std::vector<UpdatedCell> deltaState; 

// game state
bool isRunning    = true;
// map state to send
Type state[MAX_SIZE]          = {};
Type prevState[MAX_SIZE]      = {};
// TODO: change, gas update guard
bool updatedState[MAX_SIZE]   = {};

bool sendPacket(int &sock, Packet &packet) {
   // handling broken pipes 
   if (send(sock, &packet, sizeof(Packet), MSG_NOSIGNAL) < 0 &&
         errno == EPIPE) {
      return false;
   }
   else return true;
}

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

Packet preparePacket(Opcode opcode) {
   Packet packet;
   packet.opcode = opcode;
   if (opcode == FULL_SYNC) {
      memcpy(packet.payload.map, state, MAX_SIZE);

   } else if (opcode == DISPLAY) {
      uint16_t size = deltaState.size() * sizeof(UpdatedCell);
      packet.size = size;

      if (size > PACKET_SIZE) {
         memcpy(packet.payload.map, deltaState.data(), PACKET_SIZE);

      } else {
         memcpy(packet.payload.map, deltaState.data(), size);
      }

   } else if (opcode == CONFIGURE) {
      packet.payload.list.data[0] = MAP_WIDTH;
      packet.payload.list.data[1] = MAP_HEIGHT;
   }
   return packet;
}

void terminateClient(int sock) {
   Packet packet = preparePacket(TERMINATE);
   sendPacket(sock, packet);
   clients.erase(std::remove(clients.begin(), clients.end(), sock), clients.end());
   close(sock);
   printf("Client disconnected...\n");
}

void readPacket(Packet &packet, int sock) {
   if (packet.opcode == UPDATE) {
      // list of updated cells
      unsigned max_iter = packet.size / sizeof(stateId);
      pushToState(packet.payload.list, max_iter);

   } else if (packet.opcode == TERMINATE) {
      terminateClient(sock);

   } else if (packet.opcode == CLEAR) {
      for (uint16_t i = 0; i < MAX_SIZE; i++) {
         state[i] = EMPTY;
      }
   }
}

inline Type *stateGet(int x, int y) {
   if (x >= MAP_WIDTH || y >= MAP_HEIGHT ||
         x < 0 || y < 0) {
      return nullptr;
   }
   return &state[x + y * (MAP_WIDTH)];
}

inline uint16_t getId(int x, int y) {
   return x + y * (MAP_WIDTH);
}

inline bool *stateUpdatedGet(int x, int y) {
   if (x >= MAP_WIDTH || y >= MAP_HEIGHT ||
         x < 0 || y < 0) {
      return nullptr;
   }
   return &updatedState[x + y * (MAP_WIDTH)];
}

void calculateDelta() {
   for (uint16_t id = 0; id < MAX_SIZE; id++) {
      if (state[id] != prevState[id]) {
         deltaState.push_back(UpdatedCell{
               .id = id, 
               .type = state[id]
               });
      }
   }
}

void mapStateUpdate() {
   // iterating over entire map
   std::random_device   randDev;
   std::mt19937         generator(randDev());
   std::uniform_int_distribution<int> distr(-1, 1);

   for (int y = MAP_HEIGHT - 1; y >= 0; y--) {
      for (int x = 0; x < MAP_WIDTH; x++) {
         Type *cell        = stateGet(x, y);
         bool *cellUpdated = stateUpdatedGet(x, y);
         Type *neighbour   = nullptr;
         
         // update sand gravity
         if (*cell == SAND) {
            // choose random direction
            int mov = distr(generator);
            neighbour = stateGet(x, y + 1);

            for (int i = 0; i < 2; i++) {
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
            int x_ran = distr(generator);
            int y_ran = distr(generator);

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
      uint16_t width    = atoi(argv[3]);
      uint16_t height   = atoi(argv[4]);

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
      exit(1);
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
      perror("Bind failed! ");
      exit(1);
   }
   listen(servSock, 4);
   
   // no block
   fcntl(servSock, F_SETFL, fcntl(servSock, F_GETFL) | O_NONBLOCK);

   // game loop
   int clientSock;
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
         if (!sendPacket(clientSock, packet)) {
            // failure
            terminateClient(clientSock);
         } else {
            // send entire map
            if (send(clientSock, state, sizeof(Type) * MAX_SIZE, MSG_NOSIGNAL) < 0 
               && errno == EPIPE) {
               terminateClient(clientSock);
            }
            printf("Accepted a new connection\n");
         }
      }

      // read from clients
      for (int sock : clients) {
         while (read(sock, &packet, sizeof(Packet)) > 0) {
            readPacket(packet, sock);
         }
      }
      mapStateUpdate();
      // which cells have changed since last update
      calculateDelta();

      // sending a state of the map for each client
      packet = preparePacket(DISPLAY);
      for (int sock : clients) {
         if (!sendPacket(sock, packet)) {
            printf("One of the clients is unreachable...\n");
            terminateClient(sock);
         }
      }
      deltaState.clear();
      memcpy(prevState, state, MAX_SIZE);
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
      sendPacket(sock, packet);
      close(sock);
   }
   printf("Closing the server...\n");
   close(servSock);
   return 0;
}
