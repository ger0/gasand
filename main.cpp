#include <cstdio>
#include <cstring>
#include <ctime>

#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>

#include <vector>
#include <algorithm>

#include <csignal>

#include "shared.hpp"

constexpr double TICK_RATE = 60;

// period in microseconds
constexpr double PERIOD = SEC2USEC / TICK_RATE;

// game state
bool isRunning    = true;
// server socket
int servSock;

// descriptors for clients
std::vector<int> clients;

// map state to send
Type state[MAP_WIDTH * MAP_HEIGHT] = {};

void pushToState(unsigned *arr, unsigned &max_iter) {
   Type type = EMPTY;
   for (unsigned i = 0; i < max_iter; i++) {
      if (i >= MAX_SIZE) {
         printf("ERROR DURING A PUSH_TO_STATE: pushing invalid index - %u \n", i);
      } else { 
         // set type
         if (i == 0)
            type = (Type)arr[i];
         else {
            state[arr[i]] = type;
         }
      }
   }
}

void readPacket(Packet &packet, int fd) {
   if (packet.opcode == UPDATE) {
      // list of updated cells
      unsigned max_iter = packet.size / sizeof(unsigned);
      pushToState(packet.payload.list, max_iter);

   } else if (packet.opcode == TERMINATE) {
      clients.erase(std::remove(clients.begin(), clients.end(), fd), clients.end());

   } else if (packet.opcode == CLEAR) {
      for (unsigned i = 0; i < MAX_SIZE; i++)
         state[i] = EMPTY;
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

            // choose random direction
            int mov = rand() % 3 - 1;
            Type *neighbour = stateGet(x, y + 1);

            for (unsigned i = 0; i < 2; i++) {
               if (neighbour != nullptr) {
                  neighbour = stateGet(x + (i * mov), y + 1);

                  if (neighbour != nullptr && *neighbour == EMPTY) {
                     *neighbour = *cell;
                     *cell = EMPTY;
                  }
               }
            }
         }
         // update gas state TODO: change logic
         else if (*cell == GAS) {

            // random direction
            int x_ran = rand() % 3 - 1;
            int y_ran = rand() % 3 - 1;

            Type *neighbour = stateGet(x, y - 1);

            if (neighbour != nullptr) {
               if (*neighbour == SAND) {
                  Type temp = *neighbour;

                  *neighbour = *cell;
                  *cell      = temp;

               } else if (!(*neighbour == WALL && y_ran < 0)) {
                  neighbour = stateGet(x + x_ran, y + y_ran);

                  if (neighbour != nullptr && *neighbour == EMPTY) {
                     *neighbour = *cell;
                     *cell = EMPTY; 
                  }
               }
            }
         }
      }
   }
}

void handle_sigint(int sig) {
   isRunning = false;

   Packet packet = preparePacket(TERMINATE);
   for (int sock : clients) {
      write(sock, &packet, sizeof(Packet));
      close(sock);
   }
   printf("Closing server...\n");
   close(servSock);
}

int main(int argc, char* argv[]) {
   signal(SIGINT, handle_sigint);
   if (argc != 2) {
      printf("Usage: %s <port>", argv[0]);
      return -11;
   }
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
   return 0;
}
