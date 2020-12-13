/**
*threes.c
*Released under MIT license by Harsh Vakharia (@harshjv)
*https://github.com/harshjv/threes-c
*
*RT-Thread community by Meco
*https://github.com/mysterywolf/threes
*
*2020-09-06     Meco Man   First version
*2020-09-07     Meco Man   fixed memory leak
*/

#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <termios.h>
#include <finsh.h>

static struct termios old, new_;
static int possible_values[] = { 1, 2, 3, 6 };
static int sizev = 4;
static int TILE_WIDTH = 10;
static int BLANK_WIDTH = 2;
static int TOTAL_WIDTH = (10 * 4) + (2 * 5);
static int** board;
static int background[] = { 69, 207, 255 };
static int foreground[] = { 255, 255, 0 };
static int shadow[] = { 63, 201, 11 };
static struct winsize window;

static void initTermios(int echo) {
  tcgetattr(0, &old);
  new_ = old;
  new_.c_lflag &= ~ICANON;
  new_.c_lflag &= echo ? ECHO : ~ECHO;
  tcsetattr(0, TCSANOW, &new_);
}

static void resetTermios(void) {
  tcsetattr(0, TCSANOW, &old);
}

static char getch_(int echo) {
  char ch;
  initTermios(echo);
  ch = getchar();
  resetTermios();
  return ch;
}

static char getch(void) {
  return getch_(0);
}

typedef struct {
    int* array;
    size_t used;
    size_t size;
} Array;

static Array* initArray(size_t initialSize) {
    Array *a = (Array*) calloc(sizeof(Array), 1);

    a->array = (int *) calloc(sizeof(int), initialSize);
    a->used = 0;
    a->size = initialSize;
    return a;
}

static void insertArray(Array *a, int element) {
    if (a->used == a->size) {
        a->size *= 2;
        a->array = (int *) realloc(a->array, a->size * sizeof(int));
    }
    a->array[a->used++] = element;
}

static void freeArray(Array *a) {
    free(a->array);
    a->array = NULL;
    a->used = a->size = 0;
    free(a);
}

static int getColor(int colors[], int number) {
    switch(number) {
        case 0:
            return 153;
        case 1:
            return colors[0];
        case 2:
            return colors[1];
        default:
            return colors[2];
    }
}

static int intLength(int number) {
    if(number == 0) return 1;
    return floorf(log10f(fabs((float)number))) + 1;
}

static void colorReset() {
    printf("\x1b[m");
}

static void setColorScheme(int bg, int fg) {
    printf("\x1b[38;5;%d;48;5;%dm", fg, bg);
}

static void blanks() {
    printf("%*s", TILE_WIDTH, "");
}

static void printCenter(char* str) {
    printf("%*s%s", (int) ((window.ws_col - strlen(str))/2), " ", str);
}

static void divider() {
    setColorScheme(getColor(background, 0), getColor(foreground, 0));
    printf("%*s", BLANK_WIDTH, "");
    colorReset();
}

static void centerBlank() {
    printf("%*s", (window.ws_col - TOTAL_WIDTH)/2, "");
    divider();
}

static void printTileCenter(int number) {
    int t;
    t = TILE_WIDTH - intLength(number);
    printf("%*s%d%*s", t - t/2, "", number, t/2, "");
}

static void tileLine(int i) {
    int j;
    for(j = 0; j < 4; j++) {
        if(j == 0) {
            centerBlank();
        }
        setColorScheme(getColor(background, board[i][j]), getColor(foreground, board[i][j]));
        blanks();
        colorReset();
        divider();
    }
    printf("\n");
}

static void shadowLine(int i) {
    int j;
    for(j = 0; j < 4; j++) {
        if(j == 0) {
            centerBlank();
        }
        setColorScheme(getColor(shadow, board[i][j]), getColor(foreground, board[i][j]));
        blanks();
        colorReset();
        divider();
    }
    printf("\n");
}

static void dummyLine() {
    int j;
    for(j = 0; j < 4; j++) {
        if(j == 0) {
            centerBlank();
        }
        setColorScheme(getColor(background, 0), getColor(foreground, 0));
        blanks();
        colorReset();
        divider();
    }
    printf("\n");
}

static void clearScreen() {
    printf("\x1b[1;1H\x1b[2J\x1b[3J");
}

static int drawBoard() {
    int i, j;
    
    clearScreen();

    ioctl(STDOUT_FILENO, TIOCGWINSZ, &window);
    
    printf("\n");
    printCenter("^____^");
    printf("\n\n");

    for(i = 0; i < 4; i++) {
        if(i == 0) {
            dummyLine();
        }
        tileLine(i);
        tileLine(i);
        tileLine(i);
        for(j = 0; j < 4; j++) {
            if(j == 0) {
                centerBlank();
            }
            setColorScheme(getColor(background, board[i][j]), getColor(foreground, board[i][j]));
            printTileCenter(board[i][j]);
            colorReset();
            divider();
        }
        printf("\n");
        tileLine(i);
        tileLine(i);
        shadowLine(i);
        dummyLine();
    }

    printf("\n");
    printCenter("a(left), w(up), d(right), s(down) or q(quit)");
    printf("\n");
    return 1;
}

static int** allocateBoard() {
    int** board;
    int i;

    board = (int **) calloc(sizeof(int *), 4);
    for(i = 0; i < 4; i++) {
        board[i] = (int *) calloc(sizeof(int), 4);
    }

    return board;
}

static void freeBoard(void)
{
    unsigned char i;
    
    for(i=0;i<4;i++)
    {
        free(board[i]);
    }
    free(board);
}

static int bindNumber(int i, int j) {
    return (i * 10) + j;
}

static Array* getAvailableTiles() {
    int i, j;
    Array* cells = initArray(4);

    for(i = 0; i < 4; i++) {
        for(j = 0; j < 4; j++) {
            if(board[i][j] == 0) {
                insertArray(cells, bindNumber(i, j));
            }
        }
    }
    return cells;
}

static int addRandomTile() {
    int r;
    Array* cells;

    cells = getAvailableTiles();

    r = cells->used;

    if(r == 0) 
    {   
        freeArray(cells);
        return 0;
    }

    r = rand() % r;

    r = cells->array[r];

    if(board[r / 10][r % 10] != 0) {
        printf("Error: %d\n", r);
        freeArray(cells);
        return 0;
    }

    board[r / 10][r % 10] = possible_values[rand() % sizev];

    freeArray(cells);
    return 1;
}

static void swap(int* current, int* next) {
    if(*current == 0) {
        *current = *next;
        *next = 0;
    } else {
        if((*current + *next == 3) || ((*current == *next) && (*current != 1 && *current != 2))) {
            *current = *current + *next;
            *next = 0;
        }
    }
}

static void moveUp() {
    int i, j;
    for(i = 0; i < 3; i++) {
        for(j = 0; j < 4; j++) {
            swap(&(board[i][j]), &(board[i + 1][j]));
        }
    }
}

static void moveDown() {
    int i, j;
    for(i = 3; i > 0; i--) {
        for(j = 0; j < 4; j++) {
            swap(&(board[i][j]), &(board[i - 1][j]));
        }
    }
}

static void moveLeft() {
    int i, j;
    for(i = 0; i < 4; i++) {
        for(j = 0; j < 3; j++) {
            swap(&(board[i][j]), &(board[i][j + 1]));
        }
    }
}

static void moveRight() {
    int i, j;
    for(i = 0; i < 4; i++) {
        for(j = 3; j > 0; j--) {
            swap(&(board[i][j]), &(board[i][j - 1]));
        }
    }
}

#define ANSI_SHOW_CUR   "\033[?25h"
#define ANSI_HIDE_CUR   "\033[?25l"

static int threes_main() {
    char c;
    char scorestr[17];
    unsigned int i, j, score = 0;
    unsigned char flag;

    printf(ANSI_HIDE_CUR);
    
    srand(time(NULL));

    board = allocateBoard();

    addRandomTile();
    addRandomTile();
    addRandomTile();
    addRandomTile();
    addRandomTile();
    addRandomTile();
    addRandomTile();
    addRandomTile();

    flag = (addRandomTile() != 0 && drawBoard());
    while(flag && (c = getch()) != 'q') {
        switch(c) {
            case 'w':
                moveUp();
                flag = (addRandomTile() != 0 && drawBoard());
                break;
            case 's':
                moveDown();
                flag = (addRandomTile() != 0 && drawBoard());
                break;
            case 'a':
                moveLeft();
                flag = (addRandomTile() != 0 && drawBoard());
                break;
            case 'd':
                moveRight();
                flag = (addRandomTile() != 0 && drawBoard());
                break;
        } 
    }

    for(i = 0; i < 4; i++) {
        for(j = 0; j < 4; j++) {
            score += powf(3, log2f(board[i][j]/3) + 1);
        }
    }

    sprintf(scorestr, "Score: %u", score);

    printf("\033[A\033[2K");
    printCenter(scorestr);
    printf("\n\n");

    printCenter("Threes C (https://github.com/harshjv/threes-c)\n");
    printCenter("threes   (https://github.com/mysterywolf/threes)\n");
    printCenter("Released under MIT license by Harsh Vakharia (@harshjv)\n");

    freeBoard();
    
    printf(ANSI_SHOW_CUR);
    
    return 0;
}
MSH_CMD_EXPORT_ALIAS(threes_main, threes, an indie puzzle video game);
