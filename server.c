#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>

#include <semaphore.h> //semafory
#include <pthread.h>

#include <unistd.h> //usleep
#include <fcntl.h> // O_CREAT O_READ
#include <sys/mman.h> // shm mmap munmap Prot_Read

#define WALL 1
#define GROUND 2
#define SKARB 3
#define BESTIA 4
#define GRACZ 5
#define CAMP 6


// losuje gracza w scianie


enum losowanie {PLAYER,BEAST,TREASURE_COIN,TREASURE_SMALL,TREASURE_BIG};
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
char maze[100][100]={
"sssssssssssssssssssssssssssssssssssssssssssssssssss",//51 kolumn x
"s   s       s               s         s       s   s",
"s s sss sss s sssssssss sss s sssssss sss sssss   s",
"s s   s s s           s s s   s     s     s   s   s",
"s sss s s sss   sssss s s sssss ssssssss ssss sss s",
"s s s   s    ###    s s#s     s       s       s s s",
"s s sssss sss#sssssss s#s s sss sss sss sss s s s s",
"s s         s s       s s s     s   s   s s s     s",
"s s sssssss sss sssssss sssss sss sss sss s sss s s",
"s s s     s   s   s     s   s   s   s         s s s",
"s sss sss sss sss sss sss s sss sssssssssss s s s s",
"s s   s       s     s  A  s s   s s       s s   s s",
"s s ssssss ss   sss sss#sss sss s s sssss s s sss s",
"s s     s   s s   s   s   s   s   s s     s s s   s",
"s s s   s sss sss sss sssssss sss s sss sss s s sss",
"s s s   s       s   s s       s       s     s s s s",
"s s s   sssssss s s s s ss ssss sssss sssssss s s s",
"s s s       s   s s s   s     s   s s   ###  *s   s",
"s sssssssss s sss sssssss sssssss s sssss s   sss s",
"s s       s s     s     s       s   s   s s     s s",
"s s sssss s sssss s s sss sssss sss s s sss sssss s",
"s   s##   s         s ##  s   s     s s   s       s",
"s sss#ssssssss ssssssssssss s sssssss sss s       s",
"s   s                       s           s         s",
"sssssssssssssssssssssssssssssssssssssssssssssssssss",
"e"//25 wierszy y
};

    int skarb_d[50][3];
struct zasoby *game_tab[4];

int max_x,max_y;
int free_space;
void printmapa(void);
struct point losuj(enum losowanie co);
void kolorki(const int i,const int j,const char obiekcik);
void printklient(struct zasoby *tab);
void interakcja(struct zasoby *tab);
void interakcja_accident(struct zasoby *tab1,struct zasoby* tab2);
int main(void){
	initscr();noecho();cbreak();curs_set(0); // lcurses
    srand(time(NULL));

    start_color();
    init_pair(WALL, COLOR_CYAN, COLOR_CYAN);//sciana
    init_pair(GROUND, COLOR_BLACK, COLOR_WHITE);// podloga
    init_pair(SKARB, COLOR_BLACK, COLOR_YELLOW);// skarb
    init_pair(BESTIA, COLOR_RED, COLOR_WHITE);// bestia
    init_pair(GRACZ, COLOR_WHITE, COLOR_MAGENTA);// gracz/bot
    init_pair(CAMP, COLOR_YELLOW, COLOR_GREEN);// gracz/bot

    sem_t *connect = sem_open("connect",O_CREAT,0600,1);
    // tablica dla 4 graczy
    sem_t *sem_tab[4][2];
    char sem_names[4][2][20];
    char game_names[4][20];
    strcpy(game_names[0],"Daredevil_1");strcpy(game_names[1],"Daredevil_2");strcpy(game_names[2],"Daredevil_3");strcpy(game_names[3],"Daredevil_4");
    strcpy(sem_names[0][0],"s1_servdata");strcpy(sem_names[0][1],"s1_clndata");
    strcpy(sem_names[1][0],"s2_servdata");strcpy(sem_names[1][1],"s2_clndata");
    strcpy(sem_names[2][0],"s3_servdata");strcpy(sem_names[2][1],"s3_clndata");
    strcpy(sem_names[3][0],"s4_servdata");strcpy(sem_names[3][1],"s4_clndata");
    // koniec tablicowania
    int fd,fdgame;
    fdgame = shm_open("Tomb_Raider_Prequel",O_CREAT | O_RDWR,0600);
    ftruncate(fdgame,sizeof(struct check));
    struct check *checker= mmap(NULL,sizeof(struct check),PROT_READ|PROT_WRITE,MAP_SHARED,fdgame,0);
    //checker->slots[0]=0;
    checker->PID=getpid();
    checker->actual_players=0;
    char znak;// b
    char playing[4]={0};

    timeout(150);
    while(1){
        printmapa();
        refresh();
        // sprawdzanie czy klient dołącza/wyszedł/zginał
        for(int i=0;i<4;++i){
            if(checker->slots[i]&&playing[i]==0){
                    playing[i]=1;
                    fd = shm_open(game_names[i],O_CREAT| O_RDWR,0600);
                    ftruncate(fd,sizeof(struct zasoby));
                    game_tab[i]=mmap(NULL, sizeof(struct zasoby), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                    // inicializacja
                    game_tab[i]->playing=0;
                    game_tab[i]->score=0;
                    game_tab[i]->actual_score=0;
                    game_tab[i]->zm=STAY;
                    game_tab[i]->mask=i+'1';
                    game_tab[i]->pausa=0;
                    printklient(game_tab[i]);
                    // losowanie miejsca
                    struct point p= losuj(PLAYER);
                    game_tab[i]->previous_char=maze[p.x][p.y];
                    maze[p.x][p.y]=game_tab[i]->mask;
                    game_tab[i]->pos_x=p.x;
                    game_tab[i]->pos_y=p.y;
                    //semafory
                    sem_tab[i][0]=sem_open(sem_names[i][0],O_CREAT,0600,0);//servdata
                    sem_tab[i][1]=sem_open(sem_names[i][1],O_CREAT,0600,0);//clndata
                    sem_post(sem_tab[i][0]);
                }
                if(checker->slots[i]&&playing[i]&&game_tab[i]->playing){
                    struct point p= losuj(PLAYER);
                    game_tab[i]->previous_char=maze[p.x][p.y];
                    maze[p.x][p.y]=game_tab[i]->mask;
                    game_tab[i]->pos_x=p.x;
                    game_tab[i]->pos_y=p.y;
                    game_tab[i]->playing=0;;
                }
            }
            for(int i=0;i<4;++i){
                if(checker->slots[i]&&playing[i]){
                //odebranie klienta
                //ruch i obliczanie
                //ruch
                //sprawdzenie czy uzytkownik jeszcze gra/wyjscie normalne(komunikat)/crash (sprawdzenie Pidu?)


                if(kill(game_tab[i]->PID,0)){
                    playing[i]=0;
                    checker->slots[i]=0;
                    maze[game_tab[i]->pos_x][game_tab[i]->pos_y]=game_tab[i]->previous_char;
                    shm_unlink(game_names[i]);
                    sem_unlink(sem_names[i][0]);
                    sem_unlink(sem_names[i][1]);
                    --checker->actual_players;
                    continue;
                }
                sem_wait(sem_tab[i][1]);
                switch(game_tab[i]->zm){
                    case UP:
                        if(game_tab[i]->pausa==1){break;}
                        if(maze[game_tab[i]->pos_x-1][game_tab[i]->pos_y]!='s'){
                            maze[game_tab[i]->pos_x][game_tab[i]->pos_y]=game_tab[i]->previous_char;
                            game_tab[i]->previous_char=maze[game_tab[i]->pos_x-1][game_tab[i]->pos_y];
                            maze[game_tab[i]->pos_x-1][game_tab[i]->pos_y]=game_tab[i]->mask;
                            --game_tab[i]->pos_x;
                        }
                    break;
                    case RIGHT: // ruch w dul
                        if(game_tab[i]->pausa==1){break;}
                        if(maze[game_tab[i]->pos_x][game_tab[i]->pos_y+1]!='s'){
                            maze[game_tab[i]->pos_x][game_tab[i]->pos_y]=game_tab[i]->previous_char;
                            game_tab[i]->previous_char=maze[game_tab[i]->pos_x][game_tab[i]->pos_y+1];
                            maze[game_tab[i]->pos_x][game_tab[i]->pos_y+1]=game_tab[i]->mask;
                            ++game_tab[i]->pos_y;
                        }
                    break;
                    case DOWN: //w prawo
                        if(game_tab[i]->pausa==1){break;}
                        if(maze[game_tab[i]->pos_x+1][game_tab[i]->pos_y]!='s'){
                            maze[game_tab[i]->pos_x][game_tab[i]->pos_y]=game_tab[i]->previous_char;
                            game_tab[i]->previous_char=maze[game_tab[i]->pos_x+1][game_tab[i]->pos_y];
                            maze[game_tab[i]->pos_x+1][game_tab[i]->pos_y]=game_tab[i]->mask;
                            ++game_tab[i]->pos_x;
                        }
                    break;
                    case LEFT:
                        if(game_tab[i]->pausa==1){break;}
                        if(maze[game_tab[i]->pos_x][game_tab[i]->pos_y-1]!='s'){
                            maze[game_tab[i]->pos_x][game_tab[i]->pos_y]=game_tab[i]->previous_char;
                            game_tab[i]->previous_char=maze[game_tab[i]->pos_x][game_tab[i]->pos_y-1];
                            maze[game_tab[i]->pos_x][game_tab[i]->pos_y-1]=game_tab[i]->mask;
                            --game_tab[i]->pos_y;
                        }
                    break;
                    case QUIT:
                        playing[i]=0;
                        checker->slots[i]=0;
                        maze[game_tab[i]->pos_x][game_tab[i]->pos_y]=game_tab[i]->previous_char;
                        shm_unlink(game_names[i]);
                        sem_unlink(sem_names[i][0]);
                        sem_unlink(sem_names[i][1]);
                        --checker->actual_players;
                    break;
                    default:
                    break;
                }
                    // kalkulacje?
                    if(game_tab[i]->pausa==1) game_tab[i]->pausa=0;
                    else interakcja(game_tab[i]);
                    //pozwolenie na kalkulacje (semafor)
                }
        }
        for(int i=0;i<4;++i){
            if(playing[i]){
                printklient(game_tab[i]);
                sem_post(sem_tab[i][0]);
            }
        }
        znak=getch();
        mvprintw(0,54,"Tomb Raider Prequel");
        mvprintw(1,54,"players playing : %d",checker->actual_players);
        if(znak=='q'||znak=='Q') break;
        if(znak=='c') losuj(TREASURE_COIN);
        if(znak=='t') losuj(TREASURE_SMALL);
        if(znak=='T') losuj(TREASURE_BIG);
        if(znak=='b'||znak=='B') losuj(BEAST);
    }
    endwin();
    shm_unlink("Tomb_Raider_Prequel");
    sem_unlink("connect");
	return 0;
}

void printmapa(void){
    int x=0;
    for(int i=0;;++i){
        for(int j=0;maze[i][j]!='\0';++j){
            if(maze[i][j]=='e') return;
            kolorki(i,j,maze[i][j]);
            if(maze[i][j]==' ') ++x;
            free_space=x;
        }
    }

}
void printklient(struct zasoby *tab){
    for(int i=0,x=tab->pos_x-2;i<5;++i,++x){
        for(int j=0,y=tab->pos_y-2;j<=5;++j,++y){
            if(x<0||y<0) continue;
            tab->tab[i][j]=maze[x][y];
        }
    }
}
struct point losuj(enum losowanie co){
    int zgoda=0;
    struct point result;
    result.x=-1,result.y=-1;
    if(!free_space) return result;
    while(!zgoda){
        result.y=rand()%49+1;//kolumny
        result.x=rand()%23+1;//wiersze
        if(maze[result.x][result.y]==' ') zgoda = 1;
    }
    switch(co){
        case PLAYER: break;
        case BEAST: maze[result.x][result.y]='*'; break;
        case TREASURE_COIN: maze[result.x][result.y]='c'; break;
        case TREASURE_SMALL: maze[result.x][result.y]='t'; break;
        case TREASURE_BIG: maze[result.x][result.y]='T'; break;
    }
    return result;
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
void interakcja(struct zasoby *tab){
    if(isdigit(tab->previous_char)||tab->previous_char=='*'){
        if(tab->previous_char=='*'){
            // warto tutaj bestie ogarnąć by odpowiednia previous_char='D' ale to potem
            tab->playing=1;
            if(tab->actual_score){
                maze[tab->pos_x][tab->pos_y]='D';
                for(int i=0;i<50;i++){
                    if(!skarb_d[i][0]){
                        skarb_d[i][0]=tab->pos_x;
                        skarb_d[i][1]=tab->pos_y;
                        skarb_d[i][2]=tab->actual_score;
                        break;
                    }
                }
                tab->actual_score=0;
            }else{
                maze[tab->pos_x][tab->pos_y]=' ';
            }
        }else{
            switch(tab->previous_char){
                case '1':
                    interakcja_accident(tab,game_tab[0]);
                    break;
                case '2':
                    interakcja_accident(tab,game_tab[1]);
                    break;
                case '3':
                    interakcja_accident(tab,game_tab[2]);
                    break;
                case '4':
                    interakcja_accident(tab,game_tab[3]);
                    break;
            }
        }
    }else{
        switch(tab->previous_char){
            case 'c':
                tab->actual_score+=1;
                tab->previous_char=' ';
                break;
            case 't':
                tab->actual_score+=10;
                tab->previous_char=' ';
                break;
            case 'T':
                tab->actual_score+=100;
                tab->previous_char=' ';
                break;
            case 'A':
                tab->score+=tab->actual_score;
                tab->actual_score=0;
            case '#':
                tab->pausa=1;
                break;
            case 'D':
                for(int i=0;i<50;++i){
                    if(tab->pos_x==skarb_d[i][0]&&tab->pos_y==skarb_d[i][1])
                    tab->actual_score+=skarb_d[i][2];
                    tab->previous_char=' ';
                    skarb_d[i][0]=0;skarb_d[i][1]=0;
                    break;
                }

                break;
            //jak zrobic spowolnienie xd
        }
    }
}
void interakcja_accident(struct zasoby *tab1,struct zasoby *tab2){
    tab1->playing=1;
    tab2->playing=1;
    int auxiliary=tab1->actual_score+tab2->actual_score;
    tab1->actual_score=0; tab2->actual_score=0;
    if(auxiliary){
        maze[tab1->pos_x][tab1->pos_y]='D';
        for(int i=0;i<50;i++){
            if(!skarb_d[i][0]){
                skarb_d[i][0]=tab1->pos_x;
                skarb_d[i][1]=tab1->pos_y;
                skarb_d[i][2]=auxiliary;
                break;
            }
        }
    }else maze[tab1->pos_x][tab1->pos_y]=' ';
}
