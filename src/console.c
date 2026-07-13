#include "console.h"
#include "io/affichage.h"
#include "types.h"
#include "uefi/uefi.h"
#include <stdarg.h>
#include "font.h"
#include "string.h"
#include "interruptions/interruptions.h"



Console console_principale;
Console console_informations;



#define LIGNES_RESERVEES 6



void afficher_console(const Console *console);

bool caractere_affichable(char c);

bool caractere_controle(char c);




Console creer_console(Couleur fond, Couleur texte, u8 multiplicateur_taille, u16 x, u16 y, u16 largeur, u16 hauteur) {

    Console c;

    c.couleur_fond = fond;
    c.couleur_texte = texte;

    c.x = x;
    c.y = y;
    c.largeur = largeur;
    c.hauteur = hauteur;

    c.largeur_console = largeur / (largeur_police * multiplicateur_taille);
    c.hauteur_console = hauteur / (hauteur_police * multiplicateur_taille);

    c.lignes = creer_Lignes_caracteres(c.hauteur_console);
    c.ligne_fenetre = 0;
    c.ligne_curseur = 0;
    c.colonne_curseur = 0;

    c.multiplicateur_taille = multiplicateur_taille;

    return c;
}

void init_consoles(Couleur fond, Couleur texte, u8 multiplicateur_taille) {

    console_principale = creer_console(fond, texte, multiplicateur_taille, 0, hauteur_police * LIGNES_RESERVEES, resolution_x, resolution_y - hauteur_police * LIGNES_RESERVEES);

    console_informations = creer_console(texte, fond, multiplicateur_taille, 0, 0, resolution_x, hauteur_police * LIGNES_RESERVEES);

    effacer_console(&console_principale);
    effacer_console(&console_informations);

    for (u16 i = 0; i < console_informations.hauteur_console; i++) {

        ajouter_Lignes_caracteres(console_informations.lignes, creer_Ligne_caracteres(1));
    }


    afficher_console(&console_principale);
    afficher_console(&console_informations);
}



bool curseur_visible(const Console *console) {

    return console->ligne_curseur >= console->ligne_fenetre && console->ligne_curseur < console->ligne_fenetre + console->hauteur_console;
}

void afficher_console(const Console *console) {

    u64 etat = sauvegarder_et_desactiver_interruptions();

    rectangle(console->x, console->y, console->largeur, console->hauteur, console->couleur_fond);

    for (u16 i = 0; i < console->hauteur_console; i++) {
        
        Ligne_caracteres *ligne = get_Lignes_caracteres(console->lignes, console->ligne_fenetre + i);

        if (ligne == Null) {
            continue;
        }

        for (u16 j = 0; j < ligne->taille; j++) {

            if (caractere_affichable(ligne->elements[j].caractere)) {
                caractere(
                    console->x + j * largeur_police * console->multiplicateur_taille,
                    console->y + i * hauteur_police * console->multiplicateur_taille,
                    ligne->elements[j].caractere,
                    ligne->elements[j].couleur_texte,
                    ligne->elements[j].couleur_fond,
                    console->multiplicateur_taille
                );
            }
        }
    }

    if (curseur_visible(console)) {

        caractere(
            console->x + console->colonne_curseur * largeur_police * console->multiplicateur_taille,
            console->y + (console->ligne_curseur - console->ligne_fenetre) * hauteur_police * console->multiplicateur_taille,
            '_',
            console->couleur_texte, 
            console->couleur_fond,
            console->multiplicateur_taille
        );
    }

    mise_a_jour_affichage();

    restaurer_interruptions(etat);
}

void defiler(Console *console, bool vers_bas) {

    u64 etat = sauvegarder_et_desactiver_interruptions();

    if (vers_bas) {
        
        if (console->ligne_fenetre + console->hauteur_console < console->lignes->taille) {
            console->ligne_fenetre++;
        }

    } else {
        if (console->ligne_fenetre > 0) {
            console->ligne_fenetre--;
        }
    }
    afficher_console(console);

    restaurer_interruptions(etat);
}



bool caractere_affichable(char c) {

    return (c >= 32 && c <= 126);
}

bool caractere_controle(char c) {

    return (c < 32 || c == 127);
}

void effacer_console(Console *console) {

    u64 etat = sauvegarder_et_desactiver_interruptions();

    for (u16 i = 0; i < console->lignes->taille; i++) {


        Ligne_caracteres *ligne = get_Lignes_caracteres(console->lignes, i);
        if (ligne != Null) {
            liberer_Ligne_caracteres(ligne);
        }
    }

    console->lignes->taille = 0;
    console->ligne_fenetre = 0;
    console->ligne_curseur = 0;
    console->colonne_curseur = 0;

    ajouter_Lignes_caracteres(console->lignes, creer_Ligne_caracteres(1));

    restaurer_interruptions(etat);
}

void placer_caractere(Console *console, u64 ligne, u64 colonne, char c) {

    caractere(
        console->x + colonne * largeur_police * console->multiplicateur_taille,
        console->y + (ligne - console->ligne_fenetre) * hauteur_police * console->multiplicateur_taille,
        c,
        console->couleur_texte, 
        console->couleur_fond,
        console->multiplicateur_taille
    );
}

void effacer_ligne(Console *console, u64 ligne) {
    
    u64 etat = sauvegarder_et_desactiver_interruptions();

    if (ligne >= console->lignes->taille || 
        ligne < console->ligne_fenetre || 
        ligne >= console->ligne_fenetre + console->hauteur_console) {
        return;
    }

    u64 ligne_relative = ligne - console->ligne_fenetre;
    u16 hauteur_ligne_pixels = hauteur_police * console->multiplicateur_taille;

    rectangle(
        console->x, 
        console->y + (ligne_relative * hauteur_ligne_pixels), 
        console->largeur,
        hauteur_ligne_pixels,
        console->couleur_fond
    );

    restaurer_interruptions(etat);
}

bool afficher_caractere(Console *console, char c) {

    bool mise_a_jour_graphique = true;

    if (curseur_visible(console)) {
        placer_caractere(console, console->ligne_curseur, console->colonne_curseur, ' ');
    }
    
    placer_curseur(console, console->ligne_curseur, console->colonne_curseur);
    Ligne_caracteres *ligne_actuelle = get_Lignes_caracteres(console->lignes, console->ligne_curseur);

    if (caractere_affichable(c)) {

        ligne_actuelle->elements[console->colonne_curseur].caractere = c;
        ligne_actuelle->elements[console->colonne_curseur].couleur_texte = console->couleur_texte;
        ligne_actuelle->elements[console->colonne_curseur].couleur_fond = console->couleur_fond;
        
        if (curseur_visible(console)) {
            placer_caractere(console, console->ligne_curseur, console->colonne_curseur, c);
        }

        console->colonne_curseur++;
        
        if (console->colonne_curseur >= console->largeur_console) {
            console->colonne_curseur = 0;
            console->ligne_curseur++;

            if (console->ligne_curseur >= console->ligne_fenetre + console->hauteur_console) {
                placer_curseur(console, console->ligne_curseur, console->colonne_curseur);
                defiler(console, true);
            }
        }
    }

    else if (caractere_controle(c)) {

        switch (c) {
            case '\b': {
                
                if (console->colonne_curseur > 0) {
                    console->colonne_curseur--;
                    if (console->colonne_curseur < ligne_actuelle->taille) {
                        supprimer_Ligne_caracteres(ligne_actuelle, console->colonne_curseur);
                    }
                } else if (console->ligne_curseur > 0) {
                    // CORRECTION : Remonter à la ligne précédente proprement
                    console->ligne_curseur--;
                    Ligne_caracteres *str_prec = get_Lignes_caracteres(console->lignes, console->ligne_curseur);
                    
                    if (str_prec->taille > 0) {
                        // On se positionne sur le DERNIER caractère existant (largeur - 1)
                        console->colonne_curseur = str_prec->taille - 1;
                        // On supprime ce caractère du vecteur
                        supprimer_Ligne_caracteres(str_prec, console->colonne_curseur);
                    } else {
                        console->colonne_curseur = 0;
                    }
                }

                if (curseur_visible(console)) {
                    placer_caractere(console, console->ligne_curseur, console->colonne_curseur, ' ');
                }
                break;
            }

            case '\t': {

                u8 deccalage = 8 - console->colonne_curseur % 8;
                for (u8 i = 0; i < deccalage; i++) {
                    afficher_caractere(console, ' '); // Cela gère automatiquement le wrapping
                }
                break;
            }

            case '\n': {
                console->colonne_curseur = 0;
                console->ligne_curseur++;

                if (console->ligne_curseur >= console->ligne_fenetre + console->hauteur_console) {
                    placer_curseur(console, console->ligne_curseur, console->colonne_curseur);
                    defiler(console, true);
                }
                break;
            }

            case '\f':
                effacer_console(console);
                break;

            case '\r': {

                vider_Ligne_caracteres(ligne_actuelle);
                console->colonne_curseur = 0;
                effacer_ligne(console, console->ligne_curseur);
                break;
            }

            default:
                mise_a_jour_graphique = false;
                break;
        }
    }

    return mise_a_jour_graphique;
}

void placer_curseur(Console *console, u64 ligne, u64 colonne) {
    
    u64 etat = sauvegarder_et_desactiver_interruptions();

    if (colonne >= console->largeur_console) {
        colonne = console->largeur_console - 1;
    }

    console->ligne_curseur = ligne;
    console->colonne_curseur = colonne;

    while (console->ligne_curseur >= console->lignes->taille) {
        ajouter_Lignes_caracteres(console->lignes, creer_Ligne_caracteres(1));
    }
    
    Ligne_caracteres *ligne_actuelle = get_Lignes_caracteres(console->lignes, console->ligne_curseur);
    Caractere_colore caractere_nul = { ' ', console->couleur_texte, console->couleur_fond };

    while (ligne_actuelle->taille <= console->colonne_curseur) {
        ajouter_Ligne_caracteres(ligne_actuelle, caractere_nul);
    }

    restaurer_interruptions(etat);
}
