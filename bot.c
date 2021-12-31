#include <stdio.h>
#include <ncurses.h>
#include <pthread.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <semaphore.h> //semafory

#include <unistd.h> //usleep
#include <fcntl.h> // O_CREAT O_READ
#include <stdlib.h>
#include <sys/mman.h> // shm mmap munmap Prot_Read

#define WALL 1
#define GROUND 2
#define SKARB 3
#define BESTIA 4
#define GRACZ 5
#define CAMP 6

enum ruch{UP,RIGHT,DOWN,LEFT,STAY,QUIT};

struct check{
    pid_t PID;
    char actual_players;
    char slots[4];
};
struct point{
    int x;
    int y;
};
struct zasoby{
    pid_t PID;
    int playing;//zmienic na ilosc smierci
    int score;
    int actual_score;
    enum ruch zm;
    int pos_y;
    int pos_x;
    char mask;
    char previous_char;
    char tab[5][5];
    char pausa;
};

void printmapa(struct zasoby *tab);
void kolorki(const int i,const int j,const char obiekcik);
int surrounding(struct zasoby *game);
int check(struct zasoby *game);
enum ruch przeciw(enum ruch zm);

int main(void){
    initscr();noecho();cbreak();curs_set(0);//ncurses
    srand(time(NULL));

    start_color();
    init_pair(WALL, COLOR_CYAN, COLOR_CYAN);//sciana
    init_pair(GROUND, COLOR_BLACK, COLOR_WHITE);// podloga
    init_pair(SKARB, COLOR_BLACK, COLOR_YELLOW);// skarb
    init_pair(BESTIA, COLOR_RED, COLOR_WHITE);// bestia
    init_pair(GRACZ, COLOR_WHITE, COLOR_MAGENTA);// gracz/bot
    init_pair(CAMP, COLOR_YELLOW, COLOR_GREEN);// gracz/bot

    // tablica dla 4 graczy
    struct zasoby *game;
    sem_t *sem_tab[2];
    char sem_names[4][2][20];
    char game_names[4][20];
    sem_t *connect = sem_open("connect",O_CREAT,0600,1);
    strcpy(game_names[0],"Daredevil_1");strcpy(game_names[1],"Daredevil_2");strcpy(game_names[2],"Daredevil_3");strcpy(game_names[3],"Daredevil_4");
    strcpy(sem_names[0][0],"s1_servdata");strcpy(sem_names[0][1],"s1_clndata");
    strcpy(sem_names[1][0],"s2_servdata");strcpy(sem_names[1][1],"s2_clndata");
    strcpy(sem_names[2][0],"s3_servdata");strcpy(sem_names[2][1],"s3_clndata");
    strcpy(sem_names[3][0],"s4_servdata");strcpy(sem_names[3][1],"s4_clndata");
    // koniec tablicowania
    int fdgame = shm_open("Tomb_Raider_Prequel", O_CREAT | O_RDWR, 0600);
    ftruncate(fdgame, sizeof(struct check));
    struct check *checker = mmap(NULL, sizeof(struct check), PROT_READ | PROT_WRITE, MAP_SHARED, fdgame, 0);
    int fd;
    sem_wait(connect);
    if(checker->actual_players==4) {
        printw("Server is full, sorry! :(\npress anything to quit");
        sem_post(connect);
        getch();
        endwin();
        return 1;
    }
    for(int i=0;i<4;++i){
        if(!checker->slots[i]){
            ++checker->actual_players;
            checker->slots[i]=1;
            fd=shm_open(game_names[i],O_CREAT | O_RDWR, 0600);
            ftruncate(fd, sizeof(struct zasoby));
            game= mmap(NULL, sizeof(struct zasoby), PROT_READ | PROT_WRITE, MAP_SHARED, fd,0);
            game->PID = getpid();
            sem_tab[0]=sem_open(sem_names[i][0],O_CREAT,0600,0);//servdata
            sem_tab[1]=sem_open(sem_names[i][1],O_CREAT,0600,0);//clndata
            break;
        }
    }
    sem_post(connect);
    int znak='d';
    sem_wait(sem_tab[0]);
    timeout(1);
    enum ruch memory=STAY;
    while(1){
        // ruch bota
        znak =getch();
        //przypadek gdy widzi bestie

        //przypadek gdy nie widzi bestii
        int zgoda=0,free_spaces=surrounding(game);
        enum ruch decyzja;
        while(!zgoda){
            decyzja = rand()%4;
            if(free_spaces==1){
                game->zm=decyzja;
                zgoda=check(game);
            }else{
                if(memory==decyzja) continue;
                game->zm=decyzja;
                zgoda=check(game);
            }
        }
        memory=przeciw(decyzja);
        // ruch bota
        if(znak=='q') game->zm=QUIT;
        sem_post(sem_tab[1]);
        if(game->zm==QUIT)break;
        if(kill(checker->PID,0)){
            if (sem_trywait(connect) == 0) {
                for(int i=0;i<4;++i){
                    shm_unlink(game_names[i]);
                    sem_unlink(sem_names[i][0]);
                    sem_unlink(sem_names[i][1]
                    );
                    shm_unlink("Tomb_Raider_Prequel");
                    sem_unlink("connect");
                }
            }else{}
            endwin();
            return 0;
        }
        sem_wait(sem_tab[0]);
        //drukowanie
        clear();
        printmapa(game);
        mvprintw(0,56,"Tomb Raider Prequel");
        mvprintw(1,56,"PID %d",game->PID);
        mvprintw(2,56,"Skarbonka %d",game->score);
        mvprintw(3,56,"Hajs %d",game->actual_score);
        refresh();
    }
    close(fd);
    close(fdgame);
    sem_close(sem_tab[0]);
    sem_close(sem_tab[1]);
    sem_close(connect);
    endwin();
    return 0;
}
void printmapa(struct zasoby *tab){
    for(int i=0,x=tab->pos_x-2;i<5;i++,x++){
        for(int j=0,y=tab->pos_y-2;j<5;j++,y++){
            kolorki(x,y,tab->tab[i][j]);
        }
    }
}
void kolorki(const int i,const int j,const char obiekcik){
     if(obiekcik=='s'){
            attron(COLOR_PAIR(WALL));
            mvaddch(i,j,obiekcik);
            attroff(COLOR_PAIR(WALL));
        }
        if(obiekcik=='*'){
            attron(COLOR_PAIR(BESTIA));
            mvaddch(i,j,obiekcik);
            attroff(COLOR_PAIR(BESTIA));
        }
        if(obiekcik=='c'||obiekcik=='t'||obiekcik=='T'||obiekcik=='D'){
            attron(COLOR_PAIR(SKARB));
            mvaddch(i,j,obiekcik);
            attroff(COLOR_PAIR(SKARB));
        }
        if(obiekcik=='A'){
            attron(COLOR_PAIR(CAMP));
            mvaddch(i,j,obiekcik);
            attroff(COLOR_PAIR(CAMP));
        }
        if(isdigit(obiekcik)){
            attron(COLOR_PAIR(GRACZ));
            mvaddch(i,j,obiekcik);
            attroff(COLOR_PAIR(GRACZ));
        }
        if(obiekcik=='#'||obiekcik==' '){
            attron(COLOR_PAIR(GROUND));
            mvaddch(i,j,obiekcik);
            attroff(COLOR_PAIR(GRACZ));
        }
}
int surrounding(struct zasoby *game){
    int result=4;
    if(game->tab[2+1][2]=='s') --result;
    if(game->tab[2-1][2]=='s') --result;
    if(game->tab[2][2+1]=='s') --result;
    if(game->tab[2][2-1]=='s') --result;
    return result;
}
enum ruch przeciw(enum ruch zm){
    if(zm==DOWN) return UP;
    if(zm==UP) return DOWN;
    if(zm==LEFT) return RIGHT;
    if(zm==RIGHT) return LEFT;
    return STAY;
}
int check(struct zasoby *game){
    int result=0;
    switch(game->zm){
        case UP:
        if(game->tab[1][2]!='s') result=1;
        break;
        case DOWN:
        if(game->tab[3][2]!='s') result=1;
        break;
        case LEFT:
        if(game->tab[2][1]!='s') result=1;
        break;
        case RIGHT:
        if(game->tab[2][3]!='s') result=1;
        break;
        default:
        break;
    }
    return result;
}
