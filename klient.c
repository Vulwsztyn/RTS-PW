#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <ncurses.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "wspolne.h"
#define liniaPisania 23
#define liniaRekrutacji 10
#define liniaWiadomosci 17
void przesunKursor(int a){
    int x,y;
    getyx(stdscr, y, x);
    move(y+a,0*x);
}
void clearLineFrom(int y2,int x2){
    int x,y;
    getyx(stdscr, y, x);
    move(y2, x2);
    clrtoeol();
    move(y,x);
}
void nadpisz(int co, int ile){
    int jakDaleko=17;
    clearLineFrom(co+1,jakDaleko);
    mvprintw(co+1,jakDaleko,"%d\n",ile);
    move(liniaPisania,0);
    refresh();
}

void odbieranieZmianyIlosci(int msgid, int *koniecGry){
    struct komunikatZmianyIlosci komunikat;
    size_t size=sizeof(struct komunikatZmianyIlosci)-sizeof(long);
    while(!*koniecGry){
        msgrcv(msgid,&komunikat,size,430,0);
        nadpisz(komunikat.co,komunikat.nowaWartosc);
    }
}

void clearLine(int a){
    int x,y;
    getyx(stdscr, y,x);
    clearLineFrom(y+a,0*x);
    przesunKursor(a);
}
void ogarnijZnak(int napisanyZnak,struct komunikatTekstowy *komunikat,int msgid){
    if(napisanyZnak<0||!((napisanyZnak>='0'&&napisanyZnak<='9')||(napisanyZnak>='a'&&napisanyZnak<='z')||(napisanyZnak>='A'&& napisanyZnak<='Z')||napisanyZnak==10||napisanyZnak==127||napisanyZnak==32)) return;
    if (napisanyZnak==127){
        if(komunikat->dlugosc>0)komunikat->wiadomosc[komunikat->dlugosc--]=0;
    }
    else if(napisanyZnak==10||napisanyZnak==113){
        if(napisanyZnak==10&&komunikat->dlugosc>0){
            int a=msgsnd(msgid,komunikat,sizeof(struct komunikatTekstowy)-sizeof(long),0);
            printw("%d ",a);
        }
        strcpy(komunikat->wiadomosc,"");
        komunikat->dlugosc=0;
    }

    else{
        if(komunikat->dlugosc>=66){
            strcpy(komunikat->wiadomosc,"");
            komunikat->dlugosc=0;
        }
        komunikat->wiadomosc[komunikat->dlugosc++]=napisanyZnak;
    }
}
int main(int argc, char* argv[])
{
    const int numerGracza = atoi(argv[1]);
    int msgid=msgget(320+numerGracza,IPC_CREAT|0640);
    int shmid =shmget(numerGracza+6,sizeof(int),IPC_CREAT|0640);
    int *koniecGry=shmat(shmid,0,0);
    *koniecGry=0;

    int semaforKursora=semget(789+numerGracza,1,0);
    int a[]={1};
    semctl(semaforKursora,0,SETALL,a);

    struct sembuf p[1]={{0,-1,0}};
    struct sembuf v[1]={{0,-1,0}};

    int napisanyZnak;
    initscr();
    start_color();
    nodelay(stdscr,1);

    printw("Gracz %d\n",numerGracza+1);
    printw("Lekka piechota:  %d\n",0);
    printw("Ciezka piechota: %d\n",0);
    printw("Jazda:           %d\n",0);
    printw("Robotnicy:       %d\n",0);
    printw("Surowiec:        %d\n",300);
    printw("Udane ataki:     %d\n",0);
    move(liniaPisania,0);

    if(!fork())odbieranieZmianyIlosci(msgid, koniecGry);

    struct komunikatTekstowy komunikat,wiadomosc;
    komunikat.mtype=320;
    strcpy(komunikat.wiadomosc,"");
    komunikat.dlugosc=0;

    struct komunikatKoszary kojekaRek;
    struct komunikatKonca nareszcie;
    char wiadomosci[3][66];
    int dlugosciWiadomosci[3]={0,0,0};
    for(int i=0;i<3;i++) strcpy(wiadomosci[i],"");

    while(!*koniecGry){
        usleep(1000*10);
        napisanyZnak=getch();
        if(napisanyZnak>=0){
            semop(semaforKursora,p,1);
            ogarnijZnak(napisanyZnak,&komunikat,msgid);
            clearLine(-1);
            clearLine(0);
            for(int i=0;i<komunikat.dlugosc;i++) printw("%c",komunikat.wiadomosc[i]);
            printw("\n");
            refresh();
            semop(semaforKursora,v,1);
        }
        if(msgrcv(msgid,&wiadomosc,sizeof(struct komunikatTekstowy)-sizeof(long),600,IPC_NOWAIT)>0){
            semop(semaforKursora,p,1);
            for(int i=2;i>0;i--){
                strcpy(wiadomosci[i],wiadomosci[i-1]);
                dlugosciWiadomosci[i]=dlugosciWiadomosci[i-1];
            }
            strcpy(wiadomosci[0],wiadomosc.wiadomosc);
            dlugosciWiadomosci[0]=wiadomosc.dlugosc;
            move(liniaWiadomosci,0);
            for(int i=0;i<3;i++){
                for(int j=0;j<dlugosciWiadomosci[i];j++)printw("%c",wiadomosci[i][j]);
                printw("\n");
            }
            move(liniaPisania,0);
            semop(semaforKursora,v,1);
        }
        if(msgrcv(msgid,&kojekaRek,sizeof(struct komunikatKoszary)-sizeof(long),700,IPC_NOWAIT)>0){
            semop(semaforKursora,p,1);
            move(liniaRekrutacji,0);
            printw("Kolejka rekrutacji:\n");
            for(int i=0;i<kojekaRek.ilosc;i++){
                printw("%d ",i+1);
                switch(kojekaRek.koszary[i][0]){
                    case lPiechota:
                        printw("lekka Piechota  ");
                        break;
                    case cPiechota:
                        printw("ciezka Piechota ");
                        break;
                    case jaz:
                        printw("Jazda           ");
                        break;
                    case robo:
                        printw("Robotnicy ");
                        break;
                }
                printw("%d\n",kojekaRek.koszary[i][1]);
            }
            for(int i=kojekaRek.ilosc;i<5;i++)printw("                         \n");\
            move(liniaPisania,0);
            semop(semaforKursora,v,1);
        }
        if(msgrcv(msgid,&nareszcie,sizeof(struct komunikatKonca)-sizeof(long),900,IPC_NOWAIT)>0){
            semop(semaforKursora,p,1);
            move(16,0);
            init_pair(1, COLOR_RED, COLOR_BLACK);
            attron(COLOR_PAIR(1));
            printw("Wygral gracz %d\n",nareszcie.zwyciezca+1);
            attroff(COLOR_PAIR(1));
            move(liniaPisania,0);
            semop(semaforKursora,v,1);
            sleep(1);
        }
    }
    nodelay(stdscr,0);
    getch();
    endwin();
    return 0;
}


