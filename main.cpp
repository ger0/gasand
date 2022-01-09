#include <SDL2/SDL.h>
#include <cstdio>
#include <ctime>

typedef unsigned char byte;

const unsigned SCREEN_WIDTH   = 800;
const unsigned SCREEN_HEIGHT  = 800;
// pixels per particle
const int SCALE   = 4;

bool isRunning    = true;

bool isMousePressed  = false;
int mpos_x = 0, mpos_y = 0;

// ----------- data ------------
enum Type {
   EMPTY = 0,
   WALL  = 1,
   SAND  = 2,
   GAS   = 3,
};

struct Cell {
   Type        type     = Type::EMPTY;
   SDL_Colour  colour   = SDL_Colour{0, 0, 0, 255};
};
// -----------------------------

Type brush_state = EMPTY;

// map
Cell map[(SCREEN_WIDTH / SCALE) * (SCREEN_HEIGHT / SCALE)] = {
   Cell {
      .type    = Type::EMPTY,
      .colour  = SDL_Colour{0, 0, 0, 255}
   }
};

inline Cell *mapGet(int x, int y) {
   if (x < 0 || y < 0 || 
       x >= SCREEN_WIDTH / SCALE || 
       y >= SCREEN_HEIGHT / SCALE) {
      return nullptr;
   }
   return &map[x + y * (SCREEN_WIDTH / SCALE)];
}

void mapPutCell(int x, int y, Cell cell) {
   *(mapGet(x, y)) = cell;
}

void mapStateUpdate() {
   // iterating over entire map
   for (int y = SCREEN_HEIGHT / SCALE - 1; y >= 0; y--) {
      for (int x = 0; x < SCREEN_WIDTH / SCALE; x++) {

         Cell *cell = mapGet(x, y);
         if (cell == nullptr) {
            printf("ERROR DURING UPDATING: WRONG ID\n");
         }

         // update sand gravity
         if (cell->type == SAND) {

            int mov = rand() % 3 - 1;
            Cell *neighbour = mapGet(x, y + 1);

            for (unsigned i = 0; i < 2; i++) {
               if (neighbour == nullptr) {
                  break;

               } else if (i == 0 && neighbour->type == WALL) {
                  break;

               } else {
                 neighbour = mapGet(x + (i * mov), y + 1);
               }

               if (neighbour != nullptr && neighbour->type == EMPTY) {
                  *neighbour = *cell;
                  *cell = Cell();
               }
            }
         }
         // update gas state
         else if (cell->type == GAS) {

            int x_ran = rand() % 3 - 1;
            int y_ran = rand() % 3 - 1;

            Cell *neighbour = mapGet(x, y - 1);

            if (neighbour == nullptr) {
               continue; 

            } else if (neighbour->type == SAND) {
               Cell temp = *neighbour;

               *neighbour = *cell;
               *cell      = temp;

               continue;

            } else if (neighbour->type == WALL && y_ran < 0) {
               continue;
            }

            neighbour = mapGet(x + x_ran, y + y_ran);

            if (neighbour != nullptr && neighbour->type == EMPTY) {
               *neighbour = *cell;
               *cell = Cell();
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
void tryDrawing() {
   if (isMousePressed) {
      if (brush_state == WALL) {
         mapPutCell(mpos_x, mpos_y, 
               Cell{WALL, SDL_Colour{150, 150, 150, 255}});

      } else if (brush_state == SAND) {
         mapPutCell(mpos_x, mpos_y, 
               Cell{SAND, SDL_Colour{255, 255, 50, 255}});

      } else if (brush_state == GAS) {
         mapPutCell(mpos_x, mpos_y, 
               Cell{GAS, SDL_Colour{50, 20, 100, 255}});

      } else if (brush_state == EMPTY) {
         mapPutCell(mpos_x, mpos_y, 
               Cell{EMPTY, SDL_Colour{5, 0, 0, 255}});
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
            mpos_x = e->motion.x / SCALE;
            mpos_y = e->motion.y / SCALE;
            tryDrawing();
            //printf("Mouse moved to (%d, %d)\n", mpos_x, mpos_y);
            break;

         case SDL_MOUSEBUTTONUP:
            isMousePressed = false;
            break;

         case SDL_KEYDOWN:
            printf("Scancode: 0x%02X\n", e->key.keysym.scancode);
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

void renderMap(SDL_Window *window, SDL_Renderer *renderer) {
   for (int y = 0; y < SCREEN_HEIGHT / SCALE; y++) {
      for (int x = 0; x < SCREEN_HEIGHT / SCALE; x++) {
         // drawing the cell
         SDL_Rect rect = {x * SCALE, y * SCALE, SCALE, SCALE};

         const SDL_Colour *col = &mapGet(x, y)->colour;
         SDL_SetRenderDrawColor(renderer, col->r, col->g, col->b, col->a);
         SDL_RenderFillRect(renderer, &rect);
      }
   }
   SDL_RenderPresent(renderer);
}

int main() {
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
      
   SDL_Event event;
   // game loop
   while (isRunning) {
      handleEvents(&event);
      tryDrawing();
      mapStateUpdate();
      renderMap(window, renderer);
      // todo sync
      SDL_Delay(16);
   }
   // clean up 
   cleanUp(window, renderer);
   return 0;
}

