//
// Created by artur on 2/7/18.
//

#ifndef PWPROJEKT_WSPOLNE_H
#define PWPROJEKT_WSPOLNE_H
#define LICZBA_GRACZY 3
#define MAXSEM 1
#define MAXCECH 10
#define lPiechota 0
#define cPiechota 1
#define jaz 2
#define robo 3
#define sur 4
#define udAt 5

struct komunikatZmianyIlosci{
    long mtype;
    int co;
    int nowaWartosc;
};
struct komunikatTekstowy{
    long mtype;
    char wiadomosc[66];
    int dlugosc;
};
struct komunikatKoszary{
    long mtype;
    int koszary[5][2];
    int ilosc;
};
struct komunikatKonca{
    long mtype;
    int zwyciezca;
};
#endif //PWPROJEKT_WSPOLNE_H
