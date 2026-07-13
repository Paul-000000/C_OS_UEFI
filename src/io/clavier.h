#ifndef CLAVIER_H
#define CLAVIER_H

#include "../types.h"



#define TAILLE_BUFFER_CARACTERES 16

typedef enum {
    CHAR = -128,
    UNDEF,
    ECHAP,
    ALT,
    CTRLG,
    SHIFTG,
    SHIFTD,
    MAJ,
    F1,
    F2,
    F3,
    F4,
    F5, 
    F6,
    F7,
    F8,
    F9,
    F10, 
    F11,
    F12,
    NUMLK,
    ²,
    é,
    è,
    ç,
    à,
    INSERT,
    IMPR_ECR,
    FIN,
    WINDOWS,
    ALTGR,
    CTRLD,
    FLECHE_G,
    FLECHE_D,
    FLECHE_H,
    FLECHE_B

} Touche;



extern char buffer_caracteres[TAILLE_BUFFER_CARACTERES];
extern u8 index_ecriture;
extern u8 index_lecture;
extern u8 nb_elements;



static inline u8 inb(u16 port);

u8 taille_buffer();

void gestion_clavier(u64 code);

char recuperer_caractere();

void afficher_touches();



#endif