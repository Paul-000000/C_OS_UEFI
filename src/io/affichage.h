#ifndef AFFICHAGE_H
#define AFFICHAGE_H

#include "../types.h"
#include "../uefi/uefi.h"
#include "../vecteur.h"
#include "../kernel.h"



extern u16 resolution_x;
extern u16 resolution_y;
extern u16 resolution_x_reele;
extern u32 *buffer_ptr;



typedef struct Rectangle {
    u32 x;
    u32 y;
    u16 largeur;
    u16 hauteur;

} Rectangle;

typedef union Couleur {

    struct __attribute__((packed)) {
        u8 b,g,r,a;
    };

    u32 couleur;

} Couleur;

DEFINIR_VECTEUR_TYPE(Rectangle, Vec_Rectangle)



#define RGB_32(r, g, b) ((Couleur){ .couleur = ((r) << 16) | ((g) << 8) | (b) })

#define COULEUR_NOIR    RGB_32(0, 0, 0)
#define COULEUR_BLANC   RGB_32(255, 255, 255)
#define COULEUR_GRIS    RGB_32(128, 128, 128)
#define COULEUR_GRIS_C  RGB_32(192, 192, 192)
#define COULEUR_GRIS_F  RGB_32(32, 32, 32)
#define COULEUR_ROUGE   RGB_32(255, 0, 0)
#define COULEUR_VERT    RGB_32(0, 255, 0)
#define COULEUR_BLEU    RGB_32(0, 0, 255)
#define COULEUR_BLEU_C  RGB_32(0, 215, 215)



bool init_affichage(const Parametres_video *infos, bool buffer_image);

void pixel(u32 x, u32 y, Couleur c);

void rectangle(u32 x, u32 y, u16 largeur, u16 hauteur, Couleur c);

void carre(u32 x, u32 y, u16 taille, Couleur c);

void colorer_ecran(Couleur c);

void caractere(u32 x, u32 y, char c, Couleur couleur, Couleur couleur_fond, u8 taille);

void ajouter_caractere_debug(char c);

void mise_a_jour_affichage();

void afficher_affichage();



Couleur trois_niveaux(u8 n1, u8 n2, u8 n3);

Couleur couleur_gris(u8 niveau_gris);

Couleur couleur_rgb(u8 r, u8 g, u8 b);

Couleur couleur_bgr(u8 b, u8 g, u8 r);



#endif