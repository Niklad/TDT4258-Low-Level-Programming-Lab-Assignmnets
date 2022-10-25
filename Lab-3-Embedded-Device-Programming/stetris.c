#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/joystick.h>
#include <linux/fb.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <poll.h>
#include <sys/mman.h>
#include <stdint.h>


// The game state can be used to detect what happens on the playfield
#define GAMEOVER    0
#define ACTIVE      (1 << 0)
#define ROW_CLEAR   (1 << 1)
#define TILE_ADDED  (1 << 2)

// If you extend this structure, either avoid pointers or adjust
// the game logic allocate/deallocate and reset the memory
typedef struct {
    // Using the 16-bit colour as an indicator of occupation
    uint16_t colour;   // i.e. no colour --> false --> not occupied
} tile;

typedef struct {
    unsigned int x;
    unsigned int y;
} coord;

typedef struct {
    coord const grid;                         // playfield bounds
    unsigned long const uSecTickTime;         // tick rate
    unsigned long const rowsPerLevel;         // speed up after clearing rows
    unsigned long const initNextGameTick;     // initial value of nextGameTick

    unsigned int tiles; // number of tiles played
    unsigned int rows;  // number of rows cleared
    unsigned int score; // game score
    unsigned int level; // game level

    tile *rawPlayfield; // pointer to raw memory of the playfield
    tile **playfield;   // This is the play field array
    unsigned int state;
    coord activeTile;   // current tile

    unsigned long tick;                 // incremeted at tickrate, wraps at nextGameTick
                                        // when reached 0, next game state calculated
    unsigned long nextGameTick;         // sets when tick is wrapping back to zero
                                        // lowers with increasing level, never reaches 0
} gameConfig;

gameConfig game = {
    .grid = {8, 8},
    .uSecTickTime = 10000,
    .rowsPerLevel = 2,
    .initNextGameTick = 50,
};

typedef struct {
    uint8_t red;   // 5 bit value
    uint8_t green; // 6 bit value
    uint8_t blue;  // 5 bit value
} colour;

int frame_buffer_fd;
struct fb_fix_screeninfo finfo_fb;
int joystick_fd;
tile *fb_vmap;


/*  This function is called on the start of your application
    Here you can initialize what ever you need for your task
    return false if something fails, else true
*/
bool initializeSenseHat() {
    srand(time(NULL));

    // Open frame buffer with id "RPi-Sense FB"
    char frame_buffer_path[32];
    uint8_t i = 0;
    // Iterate through frame buffer devices
    while (true) {
        sprintf(frame_buffer_path, "/dev/fb%d", i);
        // Open frame buffer file descriptor
        frame_buffer_fd = open(frame_buffer_path, O_RDWR);
        if (frame_buffer_fd < 0) {
            i++;
            continue;
        }
        if (ioctl(frame_buffer_fd, FBIOGET_FSCREENINFO, &finfo_fb) < 0) {
            printf("Error reading fixed information.\n");
            return false;
        }
        if (strcmp(finfo_fb.id, "RPi-Sense FB") == 0) {
            break;
        }
        if (i == 255) {
            printf("Sense Hat not found.\n");
            return false;
        }
    }

    // Open joystick device with id "Raspberry Pi Sense HAT Joystick"
    char joystick_path[32];
    char joystick_name[32];
    i = 0;
    // Iterate through frame buffer devices
    while (true) {
        sprintf(joystick_path, "/dev/input/event%d", i);
        // Open frame buffer file descriptor
        joystick_fd = open(joystick_path, O_RDWR);
        if (joystick_fd < 0) {
            printf("\n\nJoystick not found, trying next one. \n\n");
            i++;
            continue;
        }
        if (ioctl(joystick_fd, EVIOCGNAME(sizeof(joystick_name)), joystick_name) < 0) {
            printf("Error reading joystick name.\n");
            return false;
        }
        if (strcmp(joystick_name, "Raspberry Pi Sense HAT Joystick") == 0) {
            break;
        }
        if (i == 255) {
            printf("Joystick not found.\n");
            return false;
        }
    }

    // Make a virtual memory mapping to the framebuffer
    fb_vmap = (tile *)mmap(NULL, finfo_fb.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, frame_buffer_fd, 0);
    if (fb_vmap == MAP_FAILED) {
        printf("\nError mapping framebuffer\n");
        return false;
    }

    // Clear the screen
    memset((tile *)fb_vmap, 0, finfo_fb.smem_len);

    return true;
}

/*  This function is called when the application exits
    Here you can free up everything that you might have opened/allocated
*/
void freeSenseHat() {
    munmap(fb_vmap, finfo_fb.smem_len);
    close(frame_buffer_fd);
    close(joystick_fd);
}

/*  This function should return the key that corresponds to the joystick press
    KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, with the respective direction
    and KEY_ENTER, when the the joystick is pressed
    !!! when nothing was pressed you MUST return 0 !!!
*/
int readSenseHatJoystick() {
    struct pollfd evpoll = {
             .fd = joystick_fd,
             .events = POLLIN
    };
    struct input_event ev[64];
	int rd;
    if (poll(&evpoll, 1, 0)){
        rd = read(evpoll.fd, ev, sizeof(struct input_event) * 64);
        for (int i = 0; i < rd / sizeof(struct input_event); i++) {
            if (ev[i].type == EV_KEY && (ev[i].value == 1 || ev[i].value == 2)) {
                return ev[i].code;
            }
        }
    }
    return 0;
}

/*  This function should render the gamefield on the LED matrix. It is called
    every game tick. The parameter playfieldChanged signals whether the game logic
    has changed the playfield */
void renderSenseHatMatrix(bool const playfieldChanged) {
    if(playfieldChanged){
        memcpy((tile *)fb_vmap, game.rawPlayfield, 128);
    }
}

void flashyflashy(){
    for(int i = 0; i < 10; i++) {
        memset((tile *)fb_vmap, 0, 128);
        usleep(100000);
        memcpy((tile *)fb_vmap, game.rawPlayfield, 128);
        usleep(100000);
    }
}

/*  The game logic uses only the following functions to interact with the playfield.
    if you choose to change the playfield or the tile structure, you might need to
    adjust this game logic <> playfield interface
*/

static inline void newTile(coord const target) {
    colour tile_colour;
    tile_colour.red = rand() % 32;
    tile_colour.blue = rand() % 32;
    tile_colour.green = 63 - tile_colour.red - tile_colour.blue;    // To ensure that the tile is visible
    uint16_t tile_colour_comb = (tile_colour.red << 11) | (tile_colour.green << 5) | (tile_colour.blue);
    game.playfield[target.y][target.x].colour = tile_colour_comb;
}

static inline void copyTile(coord const to, coord const from) {
    memcpy((void *) &game.playfield[to.y][to.x], (void *) &game.playfield[from.y][from.x], sizeof(tile));
}

static inline void copyRow(unsigned int const to, unsigned int const from) {
    memcpy((void *) &game.playfield[to][0], (void *) &game.playfield[from][0], sizeof(tile) * game.grid.x);
}

static inline void resetTile(coord const target) {
    memset((void *) &game.playfield[target.y][target.x], 0, sizeof(tile));
}

static inline void resetRow(unsigned int const target) {
    memset((void *) &game.playfield[target][0], 0, sizeof(tile) * game.grid.x);
}

static inline bool tileOccupied(coord const target) {
    return game.playfield[target.y][target.x].colour;
}

static inline bool rowOccupied(unsigned int const target) {
    for (unsigned int x = 0; x < game.grid.x; x++) {
        coord const checkTile = {x, target};
        if (!tileOccupied(checkTile)) {
            return false;
        }
    }
    return true;
}


static inline void resetPlayfield() {
    for (unsigned int y = 0; y < game.grid.y; y++) {
        resetRow(y);
    }
}

/*  Below here comes the game logic. Keep in mind: You are not allowed to change how the game works!
    that means no changes are necessary below this line! And if you choose to change something
    keep it compatible with what was provided to you!
*/

bool addNewTile() {
    game.activeTile.y = 0;
    game.activeTile.x = (game.grid.x - 1) / 2;
    if (tileOccupied(game.activeTile))
        return false;
    newTile(game.activeTile);
    return true;
}

bool moveRight() {
    coord const newTile = {game.activeTile.x + 1, game.activeTile.y};
    if (game.activeTile.x < (game.grid.x - 1) && !tileOccupied(newTile)) {
        copyTile(newTile, game.activeTile);
        resetTile(game.activeTile);
        game.activeTile = newTile;
        return true;
    }
    return false;
}

bool moveLeft() {
    coord const newTile = {game.activeTile.x - 1, game.activeTile.y};
    if (game.activeTile.x > 0 && !tileOccupied(newTile)) {
        copyTile(newTile, game.activeTile);
        resetTile(game.activeTile);
        game.activeTile = newTile;
        return true;
    }
    return false;
}


bool moveDown() {
    coord const newTile = {game.activeTile.x, game.activeTile.y + 1};
    if (game.activeTile.y < (game.grid.y - 1) && !tileOccupied(newTile)) {
        copyTile(newTile, game.activeTile);
        resetTile(game.activeTile);
        game.activeTile = newTile;
        return true;
    }
    return false;
}


bool clearRow() {
    if (rowOccupied(game.grid.y - 1)) {
        for (unsigned int y = game.grid.y - 1; y > 0; y--) {
            copyRow(y, y - 1);
        }
        resetRow(0);
        return true;
    }
    return false;
}

void advanceLevel() {
    game.level++;
    switch(game.nextGameTick) {
    case 1:
        break;
    case 2 ... 10:
        game.nextGameTick--;
        break;
    case 11 ... 20:
        game.nextGameTick -= 2;
        break;
    default:
        game.nextGameTick -= 10;
    }
}

void newGame() {
    game.state = ACTIVE;
    game.tiles = 0;
    game.rows = 0;
    game.score = 0;
    game.tick = 0;
    game.level = 0;
    resetPlayfield();
}

void gameOver() {
    static bool flashy = false;
    game.state = GAMEOVER;
    game.nextGameTick = game.initNextGameTick;
    if(flashy){
        flashyflashy();
        return;
    }
    flashy = true;
}


bool sTetris(int const key) {
    bool playfieldChanged = false;

    if (game.state & ACTIVE) {
        // Move the current tile
        if (key) {
            playfieldChanged = true;
            switch(key) {
            case KEY_LEFT:
                moveLeft();
                break;
            case KEY_RIGHT:
                moveRight();
                break;
            case KEY_DOWN:
                while (moveDown()) {};
                game.tick = 0;
                break;
            default:
                playfieldChanged = false;
            }
        }

        // If we have reached a tick to update the game
        if (game.tick == 0) {
            // We communicate the row clear and tile add over the game state
            // clear these bits if they were set before
            game.state &= ~(ROW_CLEAR | TILE_ADDED);

            playfieldChanged = true;
            // Clear row if possible
            if (clearRow()) {
                game.state |= ROW_CLEAR;
                game.rows++;
                game.score += game.level + 1;
                if ((game.rows % game.rowsPerLevel) == 0) {
                    advanceLevel();
                }
            }

            // if there is no current tile or we cannot move it down,
            // add a new one. If not possible, game over.
            if (!tileOccupied(game.activeTile) || !moveDown()) {
                if (addNewTile()) {
                    game.state |= TILE_ADDED;
                    game.tiles++;
                } else {
                    gameOver();
                }
            }
        }
    }

    // Press any key to start a new game
    if ((game.state == GAMEOVER) && key) {
        playfieldChanged = true;
        newGame();
        addNewTile();
        game.state |= TILE_ADDED;
        game.tiles++;
    }

    return playfieldChanged;
}

int readKeyboard() {
    struct pollfd pollStdin = {
             .fd = STDIN_FILENO,
             .events = POLLIN
    };
    int lkey = 0;

    if (poll(&pollStdin, 1, 0)) {
        lkey = fgetc(stdin);
        if (lkey != 27)
            goto exit;
        lkey = fgetc(stdin);
        if (lkey != 91)
            goto exit;
        lkey = fgetc(stdin);
    }
    exit:
        switch (lkey) {
            case 10: return KEY_ENTER;
            case 65: return KEY_UP;
            case 66: return KEY_DOWN;
            case 67: return KEY_RIGHT;
            case 68: return KEY_LEFT;
        }
    return 0;
}

void renderConsole(bool const playfieldChanged) {
    if (!playfieldChanged)
        return;

    // Goto beginning of console
    fprintf(stdout, "\033[%d;%dH", 0, 0);
    for (unsigned int x = 0; x < game.grid.x + 2; x ++) {
        fprintf(stdout, "-");
    }
    fprintf(stdout, "\n");
    for (unsigned int y = 0; y < game.grid.y; y++) {
        fprintf(stdout, "|");
        for (unsigned int x = 0; x < game.grid.x; x++) {
            coord const checkTile = {x, y};
            fprintf(stdout, "%c", (tileOccupied(checkTile)) ? '#' : ' ');
        }
        switch (y) {
            case 0:
                fprintf(stdout, "| Tiles: %10u\n", game.tiles);
                break;
            case 1:
                fprintf(stdout, "| Rows:  %10u\n", game.rows);
                break;
            case 2:
                fprintf(stdout, "| Score: %10u\n", game.score);
                break;
            case 4:
                fprintf(stdout, "| Level: %10u\n", game.level);
                break;
            case 7:
                fprintf(stdout, "| %17s\n", (game.state == GAMEOVER) ? "Game Over" : "");
                break;
        default:
                fprintf(stdout, "|\n");
        }
    }
    for (unsigned int x = 0; x < game.grid.x + 2; x++) {
        fprintf(stdout, "-");
    }
    fflush(stdout);
}


inline unsigned long uSecFromTimespec(struct timespec const ts) {
    return ((ts.tv_sec * 1000000) + (ts.tv_nsec / 1000));
}

int main(int argc, char **argv) {
    (void) argc;
    (void) argv;
    /*  This sets the stdin in a special state where each
        keyboard press is directly flushed to the stdin and additionally
        not outputted to the stdout
    */
    {
        struct termios ttystate;
        tcgetattr(STDIN_FILENO, &ttystate);
        ttystate.c_lflag &= ~(ICANON | ECHO);
        ttystate.c_cc[VMIN] = 1;
        tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
    }

    // Allocate the playing field structure
    game.rawPlayfield = (tile *) malloc(game.grid.x * game.grid.y * sizeof(tile));
    game.playfield = (tile**) malloc(game.grid.y * sizeof(tile *));
    if (!game.playfield || !game.rawPlayfield) {
        fprintf(stderr, "ERROR: could not allocate playfield\n");
        return 1;
    }
    for (unsigned int y = 0; y < game.grid.y; y++) {
        game.playfield[y] = &(game.rawPlayfield[y * game.grid.x]);
    }

    // Reset playfield to make it empty
    resetPlayfield();
    // Start with gameOver
    gameOver();

    if (!initializeSenseHat()) {
        fprintf(stderr, "ERROR: could not initilize sense hat\n");
        return 1;
    };

    // Clear console, render first time
    fprintf(stdout, "\033[H\033[J");
    renderConsole(true);
    renderSenseHatMatrix(true);

    while (true) {
        struct timeval sTv, eTv;
        gettimeofday(&sTv, NULL);

        int key = readSenseHatJoystick();
        if (!key)
            key = readKeyboard();
        if (key == KEY_ENTER)
            break;

        bool playfieldChanged = sTetris(key);
        renderConsole(playfieldChanged);
        renderSenseHatMatrix(playfieldChanged);

        // Wait for next tick
        gettimeofday(&eTv, NULL);
        unsigned long const uSecProcessTime = ((eTv.tv_sec * 1000000) + eTv.tv_usec) - ((sTv.tv_sec * 1000000 + sTv.tv_usec));
        if (uSecProcessTime < game.uSecTickTime) {
            usleep(game.uSecTickTime - uSecProcessTime);
        }
        game.tick = (game.tick + 1) % game.nextGameTick;
    }

    freeSenseHat();
    free(game.playfield);
    free(game.rawPlayfield);

    return 0;
}
