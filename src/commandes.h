#ifndef COMMANDES_H
#define COMMANDES_H

#include "string.h"



typedef struct {

    Str *nom;
    void (*fonction)();

} Commande;



void interpreteur_commandes();

void ajouter_commande(Str *nom, void (*fonction)());



#endif