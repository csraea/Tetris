#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <ncurses.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#define ROWS 24
#define COLMS 13

#define TRUE 1
#define FALSE 0


int score = 0;
char GameOn = TRUE;
double timer = 300000; //half second
//////
typedef WINDOW Window;
// This function is not very general, but could easily be changed as needed
int renderMenu(Window* W, int menuWidth, char* title, char* subtitle, int numOptions, char** options){
    wattron(W,A_REVERSE);
    mvwprintw(W,1,(menuWidth-strlen(title))/2,title);
    wattroff(W,A_REVERSE);
    mvwprintw(W,3,2,subtitle);

    int highlight = 0;
    while(true){
        for(int i = 0; i < numOptions; ++i){
            if(i==highlight){
                wattron(W,A_BOLD);
                mvwprintw(W,5+i,5,"*");
            }
            else
                mvwprintw(W,5+i,5," ");
            mvwprintw(W,5+i,6,options[i]);
            wattroff(W,A_BOLD);
        }
        int choice = wgetch(W);

        switch(choice){
            case KEY_DOWN:
                if(highlight < 2) highlight++;
                break;
            case KEY_UP:
                if(highlight > 0) highlight--;
            default:
                break;
        }
        refresh();
        // wgetch 10  -> enter -> no more rendering
        wrefresh(W);
        if(choice==10) break;
    }
    return highlight;
}
typedef struct {
    char **array;
    int width, row, col;
} Shape;
Shape current;

const Shape ShapesArray[10]= {

    {(char *[]){(char []){1,0,1},(char []){1,1,1}, (char []){0,0,0}}, 3},   //1 extra detail
    {(char *[]){(char []){0,0,0},(char []){0,1,0}, (char []){0,0,0}}, 3},   //2 extra detail
    {(char *[]){(char []){0,1,0},(char []){0,1,1}, (char []){0,0,0}}, 3},   //3 extra detail
    {(char *[]){(char []){0,1,1},(char []){1,1,0}, (char []){0,0,0}}, 3},                           //S_shape     
    {(char *[]){(char []){1,1,0},(char []){0,1,1}, (char []){0,0,0}}, 3},                           //Z_shape     
    {(char *[]){(char []){0,1,0},(char []){1,1,1}, (char []){0,0,0}}, 3},                           //T_shape     
    {(char *[]){(char []){0,0,1},(char []){1,1,1}, (char []){0,0,0}}, 3},                           //L_shape     
    {(char *[]){(char []){1,0,0},(char []){1,1,1}, (char []){0,0,0}}, 3},                           //ML_shape    
    {(char *[]){(char []){1,1},(char []){1,1}}, 2},                                                   //SQ_shape
    {(char *[]){(char []){0,0,0,0}, (char []){1,1,1,1}, (char []){0,0,0,0}, (char []){0,0,0,0}}, 4} //R_shape
};

Shape CopyShape(Shape shape){
    Shape new_shape = shape;
    char **copyshape = shape.array;
    new_shape.array = (char**)malloc(new_shape.width*sizeof(char*));
    int i, j;
    for(i = 0; i < new_shape.width; i++){
        new_shape.array[i] = (char*)malloc(new_shape.width*sizeof(char));
        for(j=0; j < new_shape.width; j++) {
            new_shape.array[i][j] = copyshape[i][j];
        }
    }
    return new_shape;
}

int CheckPosition(Shape shape);
void GetNewShape();
void draw_logo();
void ManipulateCurrent(int action);
void DeleteShape(Shape shape);
void PrintTable();
void Halleluyah_Baby();
void WriteToTable();
void RotateShape(Shape shape);
void exit_game();

int main() {
    srand(time(0));
    score = 0;
    int c, a;
    initscr();
    char rules[9][45] = {
        
        {"G A M E    I N F O"},
        {"                  "},
        {"'W' -- rotate item"},
        {"'S' -- accelerate item"},
        {"'A','D' -- move item"},
        {"'P' -- pause game, 10 sec "},
        {"'Q' -- exit game"},
        {"                  "},
        {"Good luck! It won't help you."}
    };

    
    curs_set(FALSE);
    draw_logo();
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_WHITE, COLOR_BLACK);
    init_pair(4, COLOR_RED, COLOR_BLACK);
    refresh();
    int yMax, xMax, menuHeight=10, menuWidth=40;


    getmaxyx(stdscr,yMax,xMax);
    Window *menuwin = newwin(menuHeight,menuWidth,(yMax-menuHeight)/2,(xMax-menuWidth)/2);
    keypad(menuwin,TRUE);
    refresh();
    box(menuwin,0,0);
  

    char* difficulties[3] = { "Easy", "Medium", "Hard" };
    int choice, highlight = renderMenu(menuwin,menuWidth,"Welcome to the game, TETRIS!","Select difficulty:",3,difficulties);
    delwin(menuwin);
    refresh();
    if(yMax <= menuHeight || xMax <= menuWidth){
        printw("Terminal window too small!");
        getch();
        endwin();
        return -1;
    }
    switch(highlight){
        case 0:
            a = 1;
            attron(COLOR_PAIR(1));
            timer = 350000;
            halfdelay(3.5);
            break;
        case 1:
            a = 2;
            attron(COLOR_PAIR(2));
            timer = 250000;
            halfdelay(2.5);
            break;
        
        case 2:
            a = 3;
            attron(COLOR_PAIR(3));
            timer = 150000;
            halfdelay(1.5);
            break;
    }

    struct timeval before, after;
    gettimeofday(&before, NULL);
    nodelay(stdscr, TRUE);
    GetNewShape();
    PrintTable();
    char line[ROWS];

        while(GameOn){
            int length = 4;
            attron(COLOR_PAIR(3));
            for(int row_index = 0; row_index < 9; row_index++){
                mvprintw(length, COLMS*2 + 4, "%s",rules[row_index]);
                refresh();
                length++;
            }
            attron(COLOR_PAIR(a));
            for(int i = 0; i < ROWS+2-1; i++){
                line[i] = '#';
                mvprintw(i, COLMS*2, "%c", line[i]);
            }
            for(int i = 0; i < COLMS*2; i++){
                line[i] = '#';
                mvprintw(ROWS, i, "%c", line[i]);   

            }
                        
            if ((c = getch()) != ERR) {
                ManipulateCurrent(c);
            }
            gettimeofday(&after, NULL);
            if (((double)after.tv_sec*1000000 + (double)after.tv_usec)-((double)before.tv_sec*1000000 + (double)before.tv_usec) > timer){ //time difference in microsec accuracy
                before = after;
                ManipulateCurrent('s');
            }
        }
    
    printw("\nGame over\n");
    DeleteShape(current);
    endwin();
    return 0;
}

char Table[ROWS][COLMS] = {0};

void GetNewShape(){ //returns random shape
    Shape new_shape = CopyShape(ShapesArray[rand()%10]);

    new_shape.col = rand()%(COLMS-new_shape.width+1);
    new_shape.row = 0;
    DeleteShape(current);
    current = new_shape;
    if(!CheckPosition(current)){
        usleep( 2 * 900000 );
        exit_game();
    }
}

void draw_logo(){
    clear();
    char logo[8][55] = {
        
        {"                                                    "},
        {"                                    by Korotetskyi© "},
        {" ________  _______  ________    _____  __    _____  "},
        {"|__    __||   ____||__    __| / ___  |(__) /  ____| "},
        {"   |  |   |  |____    |  |   | (___| | __ |  \\___  "},
        {"   |  |   |   ____|   |  |    \\   _  ||  | \\___  \\"},
        {"   |  |   |  |____    |  |    /  / | ||  | ____/  | "},
        {"   |__|   |_______|   |__|   /__/  |_||__||______/  "}
    };

    int center = COLS/2 - strlen(logo[0])/2;

    int target = 16;

    for( int row_count = 8; row_count >= 1; row_count-- ){

        for( int y = 1; y <= target; y++ ){
            move(y, center);
            printw("%s", logo[row_count]);
            refresh();
            usleep( 1 * 60000 );

            // clear
            move(y,center);
            printw(logo[0]);
        }

        move(target,center);
        printw(logo[row_count]);

        target--;
    }
    usleep( 2 * 600000 );
    clear();
}

void ManipulateCurrent(int action){
    Shape temp = CopyShape(current);
    switch(action){
        case 's': case 'S': 
            temp.row++;  //move down
            if(CheckPosition(temp))
                current.row++;
            else {
                WriteToTable();
                Halleluyah_Baby(); //check full lines, after putting it down
                GetNewShape();
            }
            break;
        case 'd': case 'D':
            temp.col++;  //move right
            if(CheckPosition(temp))
                current.col++;
            break;
        case 'a': case 'A': 
            temp.col--;  //move left
            if(CheckPosition(temp))
                current.col--;
            break;
        case 'w': case 'W': 
            RotateShape(temp);  //yes
            if(CheckPosition(temp))
                RotateShape(current);
            break;
        case 'q': case 'Q':
            clear();
            curs_set(TRUE);
            endwin();
            exit(0);
        case 'p': case 'P':
        	int rp = 0;
        	while(getch(rp) != 'p' || getch(rp) != 'P')
            usleep( 1 * 10000 );
            break;
    }         
    DeleteShape(temp);
    PrintTable();
}
void DeleteShape(Shape shape){

    int i;
    for(i = 0; i < shape.width; i++){
        free(shape.array[i]);
    }
    free(shape.array);
}

void PrintTable(){
    char Buffer[ROWS][COLMS] = {0};
    int i, j;
    for(i = 0; i < current.width ;i++){
        for(j = 0; j < current.width ; j++){
            if(current.array[i][j])
                Buffer[current.row+i][current.col+j] = current.array[i][j];
        }
    }
    clear();
    for(i = 0; i < ROWS ;i++){
        for(j = 0; j < COLMS ; j++){
            printw("%c ", (Table[i][j] + Buffer[i][j])? 'O': '.');
        }
        printw("\n");
    }
    printw("\nScore: %d\n", score);
}

void Halleluyah_Baby(){ //checks lines
    int i, j, sum, count=0;
    for(i=0;i<ROWS;i++){
        sum = 0;
        for(j=0;j< COLMS;j++) {
            sum+=Table[i][j];
        }
        if(sum==COLMS){
            count++;
            int l, k;
            for(k = i;k >=1;k--)
                for(l=0;l<COLMS;l++)
                    Table[k][l]=Table[k-1][l];
            for(l=0;l<COLMS;l++)
                Table[k][l]=0;
        }
    }
    timer-=1000; score += 100*count;
}
void WriteToTable(){
    int i, j;
    for(i = 0; i < current.width ;i++){
        for(j = 0; j < current.width ; j++){
            if(current.array[i][j])
                Table[current.row+i][current.col+j] = current.array[i][j];
        }
    }
}
void RotateShape(Shape shape){ //rotates clockwise
    Shape temp = CopyShape(shape);
    int i, j, k, width;
    width = shape.width;
    for(i = 0; i < width ; i++){
        for(j = 0, k = width-1; j < width ; j++, k--){
                shape.array[i][j] = temp.array[k][i];
        }
    }
    DeleteShape(temp);
}
int CheckPosition(Shape shape){ //Check the position of the copied shape
    char **array = shape.array;
    int i, j;
    for(i = 0; i < shape.width;i++) {
        for(j = 0; j < shape.width ;j++){
            if((shape.col+j < 0 || shape.col+j >= COLMS || shape.row+i >= ROWS)){ //Out of borders
                if(array[i][j]) //but is it just a phantom?
                    return FALSE;
            }
            else if(Table[shape.row+i][shape.col+j] && array[i][j])
                return FALSE;
        }
    }
    return TRUE;
}

void exit_game(){
    char answer;
    clear();
    char logo[14][50] = {
        
        {"                                             "},
        {"    ______   _____   ___    ___  ______      "},
        {"   / _____\\ / ___ \\ |   \\  /   ||  ____|  "},
        {"  | | ____ | /___\\ || |\\ \\/ /| || |___    "},
        {"  | | \\_  \\|  ___  || | \\__/ | ||  ___|   "},
        {"  | \\___/ || |   | || |      | || |____     "},
        {"   \\_____/ |_|   |_||_|      |_||______|    "},
        {"     ____  __      __ _______  ______        "},
        {"    / __ \\ \\ \\    / /|  _____||  ___ \\   "},
        {"   | /  \\ | \\ \\  / / | |____  | [___) |   "},
        {"   | |  | |  \\ \\/ /  |  ____| |  __  /     "},
        {"   | \\__/ |   \\  /   | |_____ | |  \\ \\   "},
        {"    \\____/     \\/    |_______||_|   \\_\\  "},
        {"                                             "}

    };

    int center = COLS/2 - strlen(logo[0])/2;

    int target = 4;

    attron(COLOR_PAIR(4));
    for( int row_count = 1; row_count <= 14; row_count++ ){

        for( int y = LINES-1; y >= target; y-- ){
            move(y, center);
            printw("%s", logo[row_count]);
            refresh();
            usleep( 1 * 25000 );

            // clear
            move(y,center);
            printw(logo[0]);
        }

        move(target,center);
        printw(logo[row_count]);

        target++;
    }

    refresh();
    attron(COLOR_PAIR(3));
    mvprintw(target+2, COLS/2-28, "Your score was %d. Next time you'll be better. Perhaps.\n", score);
    refresh();
    usleep( 5 * 1000000 );

    clear();
    curs_set(TRUE);
    endwin();


    exit(0);
}
