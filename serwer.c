#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include "wspolne.h"
#include <sys/msg.h>
#include <stdlib.h>
#include <string.h>

struct player{
    int rzeczy[6];
    int koszary[5][2];
    int iloscRekrutowanych;
};
void wyslijKomunikat(int komu,char wiadomosc[],int dlugosc) {
    struct komunikatTekstowy komunikat;
    komunikat.mtype = 600;
    strcpy(komunikat.wiadomosc,wiadomosc);
    komunikat.dlugosc=dlugosc;
    int msgid=msgget(320+komu,0);
    msgsnd(msgid,&komunikat,sizeof(struct komunikatTekstowy)-sizeof(long),0);
}
int dodajGraczowiRzecz(struct player *gracz, int *semaforyGraczy, int ktoremu, int co, int ile){
    struct sembuf p[1]={{co,-1*MAXSEM,0}};
    semop(semaforyGraczy[ktoremu],p,1);
    gracz[ktoremu].rzeczy[co]+=ile;
    if(ile){
        int msgid=msgget(320+ktoremu,0);
        struct komunikatZmianyIlosci komunikat={430,co,gracz[ktoremu].rzeczy[co]};
        msgsnd(msgid,&komunikat,sizeof(struct komunikatZmianyIlosci)-sizeof(long),0);
    }
    struct sembuf v[1]={{co,MAXSEM,0}};
    semop(semaforyGraczy[ktoremu],v,1);
    return gracz[ktoremu].rzeczy[co];
};
void wyslijKoszary(struct player gracz[],int ktory){
    struct komunikatKoszary komunikat;
    komunikat.mtype = 700;
    komunikat.ilosc=gracz[ktory].iloscRekrutowanych;
    for(int i=0;i<komunikat.ilosc;i++){
        for(int j=0;j<2;j++){
            komunikat.koszary[i][j]=gracz[ktory].koszary[i][j];
        }
    }
    int msgid=msgget(320+ktory,0);
    msgsnd(msgid,&komunikat,sizeof(struct komunikatKoszary)-sizeof(long),0);
}
int funkcjaKoszar(struct player gracz[], int semaforyGraczy[],int ktory,int mode){
    /* 0-ile rekrutowanych
     * 1-coRekrutowac
     * 2-usunzKoszar
     * */

    int a=0;
    struct sembuf p[1]={{7,-1*MAXSEM,0}};
    semop(semaforyGraczy[ktory],p,1);
    switch(mode){
        case 0:
            a=gracz[ktory].iloscRekrutowanych;
            break;
        case 1:
            a=gracz[ktory].koszary[0][0];
            break;
        case 2:
            if(gracz[ktory].koszary[0][1]==1){
                gracz[ktory].iloscRekrutowanych--;
                for(int i=0;i<gracz[ktory].iloscRekrutowanych+1;i++) {
                    for (int j = 0; j < 2; j++) {
                        gracz[ktory].koszary[i][j] = gracz[ktory].koszary[i + 1][j];
                    }
                }
            }
            else{
                gracz[ktory].koszary[0][1]--;
            }
            wyslijKoszary(gracz,ktory);
            break;
    }
    struct sembuf v[1]={{7,MAXSEM,0}};
    semop(semaforyGraczy[ktory],v,1);
    return a;
}

void dodajDoKoszar(struct player gracz[], int semaforyGraczy[],int ktory,int co,int ile){
    struct sembuf p[1]={{7,-1*MAXSEM,0}};
    semop(semaforyGraczy[ktory],p,1);
    gracz[ktory].koszary[gracz[ktory].iloscRekrutowanych][0]=co;
    gracz[ktory].koszary[gracz[ktory].iloscRekrutowanych][1]=ile;
    gracz[ktory].iloscRekrutowanych++;
    wyslijKoszary(gracz,ktory);
    struct sembuf v[1]={{7,MAXSEM,0}};
    semop(semaforyGraczy[ktory],v,1);
}
void surowceWszystkich(struct player gracz[], int semaforyGraczy[],int* koniecGry){
    for(int i=0;i<LICZBA_GRACZY;i++){
        if(!fork()){
            while(!*koniecGry){
                usleep(1000*1000);
                int robotnicy=dodajGraczowiRzecz(gracz,semaforyGraczy,i,robo,0);
                dodajGraczowiRzecz(gracz,semaforyGraczy,i,sur,50+robotnicy*5);
            }
        }
    }
}

void rekrutacjaWszystkich(struct player gracz[], int semaforyGraczy[],int* koniecGry){
    unsigned int czasy[4]={2,3,5,2};
    for(int i=0;i<LICZBA_GRACZY;i++){
        if(!fork()){
            int msgid=msgget(320+i,0);
            struct komunikatZmianyIlosci komunikat={430,sur,0};
            while(!*koniecGry){
                usleep(10*1000);
                if(funkcjaKoszar(gracz,semaforyGraczy,i,0)){
                    int co=funkcjaKoszar(gracz,semaforyGraczy,i,1);
                    sleep(czasy[co]);
                    komunikat.co=co;
                    komunikat.nowaWartosc=dodajGraczowiRzecz(gracz,semaforyGraczy,i,co,1);
                    funkcjaKoszar(gracz,semaforyGraczy,i,2);
                    msgsnd(msgid,&komunikat,sizeof(struct komunikatZmianyIlosci)-sizeof(long),0);
                }
            }
        }
    }
}


void rektutujLogika(struct player *gracz, int *semaforyGraczy, int ktory, int co, int ile){

    int koszty[4]={100,250,550,150};
    int ileMaSurowca=dodajGraczowiRzecz(gracz,semaforyGraczy,ktory,sur,0);
    int ileKosztuje=koszty[co]*ile;
    if(ileKosztuje<=ileMaSurowca){
        if(funkcjaKoszar(gracz,semaforyGraczy,ktory,0)<5){
            wyslijKomunikat(ktory,"Rozpoczeto rekrutacje",21);
            dodajGraczowiRzecz(gracz,semaforyGraczy,ktory,sur,-ileKosztuje);
            dodajDoKoszar(gracz,semaforyGraczy,ktory,co,ile);
        }
        else
        {
            wyslijKomunikat(ktory,"Rekrutacja nieudana - Kolejka rekrutacji zapelniona",53);
        }
    }
    else{
        wyslijKomunikat(ktory,"Nie stac Cie",12);
    }

}
void ataktujLogika(struct player *gracz, int *semaforyGraczy, int kto, int *j,
                   int *koniecGry){
    float ataki[3]={1.0,1.5,3.5};
    float obrony[4]={1.2,3.0,1.2};
    j[0]--;
    if(kto!=j[0]&&j[0]>0&&j[0]<4){
        int l,c,ja;
        l=dodajGraczowiRzecz(gracz,semaforyGraczy,kto,lPiechota,0);
        c=dodajGraczowiRzecz(gracz,semaforyGraczy,kto,cPiechota,0);
        ja=dodajGraczowiRzecz(gracz,semaforyGraczy,kto,jaz,0);
        if(l>=j[1]&&c>=j[2]&&ja>=j[3]){
            wyslijKomunikat(kto,"Wyslano atak",12);
            wyslijKomunikat(j[0],"Wyslano na Ciebie atak",22);
            usleep(5000*1000);
            int o[3];
            float atak=0.0,obrona=0.0;
            for(int i=0;i<3;i++){
                dodajGraczowiRzecz(gracz,semaforyGraczy,kto,i,-j[i+1]);
                o[i]=dodajGraczowiRzecz(gracz,semaforyGraczy,j[0],i,0);
                atak+=j[i+1]*ataki[i];
                obrona+=o[i]*obrony[i];
            }

            if(atak>obrona){
                wyslijKomunikat(kto,"Atak udany",10);
                wyslijKomunikat(j[0],"Przegrana obrona",16);
                int a=dodajGraczowiRzecz(gracz,semaforyGraczy,kto,udAt,1);
                if(a>=5) {
                    struct komunikatKonca nareszcie={900,kto};
                    for(int i=0;i<3;i++){
                        int msgid=msgget(320+i,0);
                        msgsnd(msgid,&nareszcie,sizeof(struct komunikatKonca)-sizeof(long),0);
                    }
                    sleep(1);
                    *koniecGry=1;
                }
                for(int i=0;i<3;i++){
                    dodajGraczowiRzecz(gracz, semaforyGraczy, j[0], i, -o[i]);
                }
            }
            else{
                wyslijKomunikat(kto,"Atak nieudany",13);
                wyslijKomunikat(j[0],"Wygrana obrona",14);
                for(int i=0;i<3;i++){
                    dodajGraczowiRzecz(gracz, semaforyGraczy, j[0], i, (int) (-o[i] * atak / obrona));
                }
            }
            for(int i=0;i<3;i++){
                dodajGraczowiRzecz(gracz, semaforyGraczy, kto, i, (int) (j[i + 1] - j[i + 1] * obrona / atak));
            }

        }
        else{
            wyslijKomunikat(kto,"Nie masz dosc wojska",21);
        }
    }
    else{
        wyslijKomunikat(kto,"Zly cel ataku",13);
    }
}
void obrobkaLogika(struct player *gracz, int *semaforyGraczy, int ktory, struct komunikatTekstowy komunikat,
                    int *koniecGry){

    if(komunikat.dlugosc==3&&komunikat.wiadomosc[1]==' '){
        int co,ile=komunikat.wiadomosc[2]-48;
        switch(komunikat.wiadomosc[0]){
            case 'j':
                co=jaz;
                break;
            case 'l':
                co=lPiechota;
                break;
            case 'c':
                co=cPiechota;
                break;
            case 'r':
                co=robo;
                break;
            default:
                wyslijKomunikat(ktory,"Niepoprawny komunikat",21);
                return;
        }
        if (!(ile>0&&ile<10)) {
            wyslijKomunikat(ktory,"Mozna rekrutwac od 1 do 9 jednostek na raz",42);
            return;
        }
        if(!fork()) rektutujLogika(gracz, semaforyGraczy, ktory, co, ile);
    }
    else{
        if(komunikat.dlugosc>1&&komunikat.wiadomosc[0]=='a'&&komunikat.wiadomosc[1]==' '){
            int j[4]={0,0,0,0},l=0;
            for (int i=2;i<komunikat.dlugosc;i++){
                if(komunikat.wiadomosc[i]!=' '){
                    if(komunikat.wiadomosc[i]>='0'&&komunikat.wiadomosc[i]<='9'){
                        j[l]*=10;
                        j[l]+=komunikat.wiadomosc[i]-48;
                    }
                    else{
                        return;
                    }
                }
                else{
                    l++;
                }
            };
            if(!fork())ataktujLogika(gracz, semaforyGraczy, ktory, j, koniecGry);
        }
        else{
            wyslijKomunikat(ktory,"Niepoprawny komunikat",21);
        }
    }
}
void ciagle(struct player *gracz, int *semaforyGraczy, int *koniecGry){
surowceWszystkich( gracz,  semaforyGraczy, koniecGry);
rekrutacjaWszystkich(gracz,  semaforyGraczy, koniecGry);
}

void komunikacja(struct player gracz[], int semaforyGraczy[],int* koniecGry){
    for (int i=0;i<LICZBA_GRACZY;i++){
        if(!fork()){
            int msgid=msgget(320+i,IPC_CREAT|0640);
            struct komunikatTekstowy komunikat;
            size_t size=sizeof(komunikat)-sizeof(long);
            while(!*koniecGry){
                msgrcv(msgid,&komunikat,size,320,0);
                if(!fork())obrobkaLogika(gracz, semaforyGraczy, i, komunikat, koniecGry);
            }
            break;
        }
    }
}

void usunKolejki(){
    int msgid;
    for(int i=0;i<3;i++){
        msgid=msgget(320+i,0);
        msgctl(msgid,IPC_RMID,NULL);
    }
}
int main(int argc, char* argv[])
{
    printf("Serwer\n");
    int shmid =shmget(0,sizeof(struct player[3]),IPC_CREAT|0640);
    struct player* gracz=shmat(shmid,0,0);
    for(int i=0;i<3;i++){
        for(int j=0;j<6;j++) gracz[i].rzeczy[j]=0;
        gracz[i].rzeczy[sur]=300;
    }

    int semaforyGraczy[3];
    unsigned short arrayRzeczyDoSemaforow[]={};
    for (int i=0;i<MAXCECH;i++) arrayRzeczyDoSemaforow[i]=MAXSEM;

    for (int i=0;i<3;i++){
        semaforyGraczy[i]=semget(i,MAXCECH,0);
        semctl(semaforyGraczy[i],0,SETALL,arrayRzeczyDoSemaforow);
    }

    shmid =shmget(1,sizeof(int),IPC_CREAT|0640);
    int *koniecGry=shmat(shmid,0,0);
    *koniecGry=0;

    if(!fork()){
    komunikacja(gracz,semaforyGraczy,koniecGry);
    }
    else{
        ciagle(gracz, semaforyGraczy, koniecGry);
    }
    while(!*koniecGry)sleep(10);
    usunKolejki();
    return 0;
}