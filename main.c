#define SDL_MAIN_HANDLED
#define SCREEN_W 640
#define SCREEN_H 480
#define CELL_SIZE 10
#define SCREEN_SCALE 1
#define SCREEN_NAME "Game Of Life"
#define FPS 10

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "SDL.h"

//function declarations
void gameHandleEvents(void);
void gameInit(void);
void gameQuit(void);
void quitOnSDL_QUIT(SDL_Event *event);


// X,Y coordinate
typedef struct Coord{
    int x;
    int y;
}Coord;

// Node for linked list
typedef struct Node{
    Coord *coord;
    struct Node *next;
}Node;

// Define global Game object
// inspired by tutorial at
// www.stephenmeier.net
static struct {

    SDL_bool running;

    struct {
        unsigned int w;
        unsigned int h;
        const char* name;
        SDL_Window* window;
        SDL_Renderer* renderer;

    } screen;

    // define "methods"
    void (*init)(void);
    void (*quit)(void);
    void (*handleEvents) (void);

} Game = {
    SDL_FALSE,
    {
        SCREEN_SCALE*SCREEN_W,
        SCREEN_SCALE*SCREEN_H,
        SCREEN_NAME,
        NULL,
        NULL
    },
    gameInit,
    gameQuit,
    gameHandleEvents
};


void gameHandleEvents(void){
    SDL_Event event;
    while(SDL_PollEvent(&event)) {

        quitOnSDL_QUIT(&event);

        const Uint8* currentKeyStates = SDL_GetKeyboardState( NULL );

        if( currentKeyStates[SDL_SCANCODE_Q]){
            Game.running = SDL_FALSE;
        }
    }
}

void quitOnSDL_QUIT(SDL_Event *event){
    switch(event->type) {
        case SDL_QUIT: {
            Game.running = SDL_FALSE;
        } break;
    }
}

void gameInit(void) {
    printf("game_init()n");
    if(SDL_Init(SDL_INIT_EVERYTHING)!=0) {
        printf("SDL error -> %sn", SDL_GetError());
        exit(1);
    }

    unsigned int w = Game.screen.w;
    unsigned int h = Game.screen.h;
    const char* name = Game.screen.name;

    Game.screen.window = SDL_CreateWindow(
        name,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        w, h, 0
    );
    Game.screen.renderer = SDL_CreateRenderer(
        Game.screen.window, -1,
        SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC
    );

    Game.running = SDL_TRUE;

}



void gameQuit(void) {
    printf("game_quit()\n");
    SDL_DestroyRenderer(Game.screen.renderer);
    SDL_DestroyWindow(Game.screen.window);

    Game.screen.window = NULL;
    Game.screen.renderer = NULL;

    SDL_Quit();
    Game.running = SDL_FALSE;

}

void render(int *arr, int m, int n, SDL_Renderer *renderer){

    // Set background color
    SDL_SetRenderDrawColor( Game.screen.renderer, 0, 0, 255, 255 );
    SDL_RenderClear(Game.screen.renderer);
    //Set cell color
    SDL_SetRenderDrawColor(Game.screen.renderer, 0, 255, 0, 255 );

    //Render all live cells
    for (int i = 0; i < m; i++){
        for (int j = 0; j < n; j++){
            int isAlive = *((arr+i*n) + j);
            if (isAlive == 1){
                SDL_Rect r;
                r.x = i * CELL_SIZE;
                r.y = j * CELL_SIZE;
                r.w = r.h = CELL_SIZE;

                SDL_RenderFillRect(renderer, &r );
            }
        }
    }
    SDL_RenderPresent(Game.screen.renderer);
}

void toggle(int *cellValue){
    if(*cellValue == 0) *cellValue = 1;
    else *cellValue = 0;
}

void iterateToggleList(Node *head, int *arr, int columns){
    Node *current = head;

    //loop through toggle list
    while(current->coord != NULL){
        Node *tmp = current;

        // toggle cell
        int x = tmp->coord->x;
        int y = tmp->coord->y;
        toggle(((arr+x*columns) + y));

        //move to next element
        current = tmp->next;

        // free memory
        free(tmp->coord);
        free(tmp);
    }
    // free mem for last element
    free(current->coord);
    free(current);
}

void nextGeneration(int *arr, int m, int n){

    //create list of cells to toggle
    Node *head = NULL;
    Node *current = malloc(sizeof(Node));
    for (int i=0; i<m; i++){
        for (int j=0; j<n; j++){
            int cntNeighbours = 0;
            for(int k=-1; k<2; k++){
                for(int l=-1; l<2; l++){

                    /* find neighbours coords
                     * we wrap around array so that height and
                     * width is neighbour to 0
                     */
                    int nX = (i+k)%(m-1);
                    int nY = (j+l)%(n-1);
                    cntNeighbours += *( arr+ (nX*n) + nY );
                }
            }
            int cell = *((arr+i*n) + j);
            // subtract self from count
            cntNeighbours -= cell;


            if ((cell == 1 && cntNeighbours < 2) || // dies from lack of neghbours
                (cell == 1 && cntNeighbours > 3) ||  // dies from over population
                (cell == 0 && cntNeighbours ==3) ) {  // new cell is born

                //Add to list of cells to toggle
                if (head == NULL) head = current;

                // Get current coordinate
                Coord *coord = malloc(sizeof(Coord));
                coord->x = i;
                coord->y = j;

                //Creeate new element in list of cells to toggle
                current->next = malloc(sizeof(Node));
                current->coord = coord;
                current = current->next;

            }
        }
    }
    // Mark end of list
    current->coord = NULL;
    iterateToggleList(head, arr, n);
}



int main()
{
    // Initialize srand
    srand(time(NULL));

    const int countX = (SCREEN_W / CELL_SIZE);
    const int countY = (SCREEN_H / CELL_SIZE);

    // cells will be 0(dead) or 1(alive);
    int cells[countX][countY];

    // Cointoss for each cell to decide if it's live or dead
    for (int x=0; x<countX; x++){
        for (int y=0; y<countY; y++){
            cells[x][y] = rand() % 2;
        }
    }

    // Initialize game
    Game.init();

    Uint32 frameStart;
    int frameTime;
    int frameDelay = 1000 / FPS;
    // Game loop
    while(Game.running) {
        // timer start
        frameStart = SDL_GetTicks();

        Game.handleEvents();
        render((int *)cells, countX, countY, Game.screen.renderer);
        nextGeneration((int *)cells, countX, countY);

        //Timer end
        frameTime = SDL_GetTicks() - frameStart;
        if (frameDelay > frameTime){
            //Delay as much as needed for constant frametime
            SDL_Delay(frameDelay - frameTime);
        }
    }

    Game.quit();

    return 0;
}
