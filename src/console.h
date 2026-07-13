#ifndef CONSOLE_H
#define CONSOLE_H

#include "types.h"
#include "uefi/uefi.h"
#include "io/affichage.h"
#include "string.h"
#include "vecteur.h"



typedef struct {
    char caractere;
    Couleur couleur_texte;
    Couleur couleur_fond;

} Caractere_colore;

DEFINIR_VECTEUR_TYPE(Caractere_colore, Ligne_caracteres);
DEFINIR_VECTEUR_TYPE(Ligne_caracteres *, Lignes_caracteres)

typedef enum Padding_texte {
    ZEROS,
    ESPACES

} Padding_texte;

typedef struct Console
{
    Couleur couleur_fond;
    Couleur couleur_texte;

    Lignes_caracteres *lignes;
    u16 ligne_fenetre;
    u16 ligne_curseur;
    u16 colonne_curseur;

    u8 multiplicateur_taille;
    u16 largeur_console;
    u16 hauteur_console;

    u16 x;
    u16 y;
    u16 largeur;
    u16 hauteur;

} Console;

extern Console console_principale;
extern Console console_informations;



void init_consoles(Couleur fond, Couleur texte, u8 multiplicateur_taille);

void effacer_console(Console *console);

void effacer_ligne(Console *console, u64 ligne);

void placer_curseur(Console *console, u64 ligne, u64 colonne);

void afficher_console(const Console *console);

void defiler(Console *console, bool vers_bas);

bool caractere_affichable(char c);

bool caractere_controle(char c);

bool afficher_caractere(Console *console, char c);

bool curseur_visible(const Console *console);

void placer_caractere(Console *console, u64 ligne, u64 colonne, char c);



#endif