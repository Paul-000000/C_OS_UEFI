#include "../types.h"
#include "../uefi/uefi.h"
#include "../font.h"
#include "affichage.h"
#include "../memoire/memoire.h"
#include "../interruptions/interruptions.h"
#include "../kernel.h"
#include "../console.h"
#include "io.h"



u16 resolution_x;
u16 resolution_y;
u16 resolution_x_reele;
u32 *buffer_ptr; 
u32 *frame_buffer_ptr;
bool buffer_image_actif;
Vec_Rectangle *rectangles_a_modifier;
bool tout_a_modifier;
u16 nb_lignes_debug;
u16 nb_colonnes_debug;
u16 ligne;
u16 colonne;



void ajouter_rectangle(u32 x, u32 y, u16 largeur, u16 hauteur);

bool init_affichage(const Parametres_video *infos, bool buffer_image) {

    resolution_x_reele = infos->resolution_horizontale_reele;
    resolution_x = infos->resolution_horizontale;
    resolution_y = infos->resolution_verticale;

    nb_lignes_debug = resolution_y / hauteur_police;
    nb_colonnes_debug = resolution_x / largeur_police;
    ligne = 0;
    colonne = 0;

    buffer_image_actif = buffer_image;
    buffer_ptr = (u32 *)infos->adresse_buffer_image; 
    frame_buffer_ptr = buffer_image ? (u32 *)malloc(resolution_x_reele * resolution_y * sizeof(u32)) : buffer_ptr;

    tout_a_modifier = false;
    rectangles_a_modifier = buffer_image ? creer_Vec_Rectangle(8) : Null;

    if (buffer_image) {

        if (frame_buffer_ptr == Null || rectangles_a_modifier == Null) {
            return false;
        }
    }

    return true;
}



void pixel(u32 x, u32 y, Couleur c) {

    if (x >= resolution_x || y >= resolution_y) return;
    
    frame_buffer_ptr[y * resolution_x_reele + x] = c.couleur;

    if (buffer_image_actif) {
        ajouter_rectangle(x, y, 1, 1);
    }
}

void rectangle(u32 x, u32 y, u16 largeur, u16 hauteur, Couleur c) {
    
    if (x >= resolution_x || y >= resolution_y) return;
    if (x + largeur > resolution_x) largeur = resolution_x - x;
    if (y + hauteur > resolution_y) hauteur = resolution_y - y;

    for (u32 i = 0; i < hauteur; i++) {
        for (u32 j = 0; j < largeur; j++) {
            frame_buffer_ptr[(y + i) * resolution_x_reele + (x + j)] = c.couleur;
        }
    }

    if (buffer_image_actif) {
        ajouter_rectangle(x, y, largeur, hauteur);
    }
}

void carre(u32 x, u32 y, u16 taille, Couleur c) {
    
    if (x >= resolution_x || y >= resolution_y) return;
    u16 largeur =  x + taille > resolution_x ? resolution_x - x : taille;
    u16 hauteur = y + taille > resolution_y ? resolution_y - y : taille;

    for (u32 i = 0; i < hauteur; i++) {
        for (u32 j = 0; j < largeur; j++) {
            frame_buffer_ptr[(y + i) * resolution_x_reele + (x + j)] = c.couleur;
        }
    }

    if (buffer_image_actif) {
        ajouter_rectangle(x, y, largeur, hauteur);
    }
}

void colorer_ecran(Couleur c) {

    u64 total_pixels = resolution_x_reele * resolution_y;

    for (u64 i = 0; i < total_pixels; i++) {
        frame_buffer_ptr[i] = c.couleur;
    }

    tout_a_modifier = true;
}

void afficher_caractere_generique(u32 x, u32 y, char c, Couleur couleur, Couleur couleur_fond, u8 taille, bool buffer_image, u32 *frame_buffer_ptr) {

    if (x + (largeur_police * taille) > resolution_x || y + (hauteur_police * taille) > resolution_y) {
        return;
    }

    for (u8 i = 0; i < hauteur_police; i++) {
        for (u8 j = 0; j < largeur_police; j++) {
            
            bool est_pixel_texte = police_8x16[(u8)c][i] & (1 << (largeur_police - j - 1));
            Couleur couleur_pixel = est_pixel_texte ? couleur : couleur_fond;
            
            for (u32 k = 0; k < taille; k++) {
                for (u32 l = 0; l < taille; l++) {
                    frame_buffer_ptr[(y + (taille * i) + k) * resolution_x_reele + (x + (taille * j) + l)] = couleur_pixel.couleur;
                }
            }
        }
    }

    if (buffer_image_actif) {
        ajouter_rectangle(x, y, largeur_police * taille, hauteur_police * taille);
    }
}

void caractere(u32 x, u32 y, char c, Couleur couleur, Couleur couleur_fond, u8 taille) {
    
    afficher_caractere_generique(x, y, c, couleur, couleur_fond, taille, buffer_image_actif, frame_buffer_ptr);
}

void ajouter_caractere_debug(char c) {

    if (!caractere_affichable(c) && c != '\n') {
        return;
    }

    if (c != '\n') {
        afficher_caractere_generique(colonne * largeur_police, ligne * hauteur_police, c, COULEUR_BLANC, COULEUR_NOIR, 1, false, buffer_ptr);
        colonne++;
    }

    if (c == '\n' || colonne >= nb_colonnes_debug) {
        colonne = 0;
        ligne++;
        if (ligne >= nb_lignes_debug) {
            ligne = 0;
        }
    }
}



void ajouter_rectangle(u32 x, u32 y, u16 largeur, u16 hauteur) {
    
    Rectangle rectangle = { x, y, largeur, hauteur };

    ajouter_Vec_Rectangle(rectangles_a_modifier, rectangle);
}





void copier_rectangle(Rectangle *r) {

    u32 x = r->x;
    u32 y = r->y;
    u16 largeur = r->largeur;
    u16 hauteur = r->hauteur;

    for (u32 i = 0; i < hauteur; i++) {
        for (u32 j = 0; j < largeur; j++) {
            
            buffer_ptr[(y + i) * resolution_x_reele + (x + j)] = frame_buffer_ptr[(y + i) * resolution_x_reele + (x + j)];
        }
    }


}

void mise_a_jour_affichage() {

    if (!buffer_image_actif) {
        return;
    }

    u64 etat = sauvegarder_et_desactiver_interruptions();

    if (tout_a_modifier) {

        memcpy(buffer_ptr, frame_buffer_ptr, resolution_x_reele * resolution_y * sizeof(u32));

    } else {

        for (u64 i = 0; i < rectangles_a_modifier->taille; i++) {
            
            Rectangle r = get_Vec_Rectangle(rectangles_a_modifier, i);
            
            copier_rectangle(&r);
        }
    }

    tout_a_modifier = false;
    vider_Vec_Rectangle(rectangles_a_modifier);

    restaurer_interruptions(etat);
}

void afficher_affichage() {

    placer_curseur(&console_informations, 4, 0);

    Ligne_caracteres *ligne = get_Lignes_caracteres(console_informations.lignes, 4);
    vider_Ligne_caracteres(ligne);

    effacer_ligne(&console_informations, 4);

    afficher(&console_informations, "%F%C[affichage]%F%C %u rectangles",
        COULEUR_BLEU,
        COULEUR_BLANC,
        COULEUR_BLANC,
        COULEUR_GRIS_F,
        rectangles_a_modifier->taille
    );
}



Couleur trois_niveaux(u8 n1, u8 n2, u8 n3) {
    
    Couleur c;
    c.r = n1;
    c.g = n2;
    c.b = n3;
    return c;
}

Couleur couleur_gris(u8 niveau_gris) {

	return trois_niveaux(niveau_gris, niveau_gris, niveau_gris);
}

Couleur couleur_rgb(u8 r, u8 g, u8 b) {

	return trois_niveaux(r, g, b);
}

Couleur couleur_bgr(u8 b, u8 g, u8 r) {

	return trois_niveaux(r, g, b);
}
