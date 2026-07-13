#include "types.h"
#include "kernel.h"
#include "io/affichage.h"
#include "console.h"
#include "interruptions/interruptions.h"
#include "processus/processus.h"
#include "commandes.h"
#include "memoire/memoire.h"
#include "memoire/memoire_physique.h"
#include "memoire/memoire_virtuelle.h"
#include "string.h"
#include "io/io.h"



/*

À IMPLÉMENTER :

- USB
- systeme de fichiers Fat32
- clavier et souris
- fenetres

- optimisation liste doublement chainée tas (O(1))
- aléatoire

*/



Informations_Boot *informations_boot;



void kernel_main(Informations_Boot *infos) {
    
    informations_boot = infos;

    bool affichage_initialise = init_affichage(&informations_boot->video, false);
    if (!affichage_initialise) {
        attente_infinie();
    }

    bool memoire_initialisee = init_tas(&informations_boot->memoire, obtenir_table_actuelle(), 4096);
    if (!memoire_initialisee) {
        erreur_fatale(-1, "Impossible d'initialiser le tas physique");
    }

    affichage_initialise = init_affichage(&informations_boot->video, true);
    if (!affichage_initialise) {
        erreur_fatale(-1, "Impossible d'initialiser l'affichage avec buffer");
    }

    init_consoles(COULEUR_GRIS_F, COULEUR_BLANC, 1);

    init_interruptions();

    init_processus();

    cree_processus(interpreteur_commandes, Str_creer("interpreteur de commandes"), 1);

    
    
    while (true) {
        configurer_interruptions(true);
        hlt(); 
        configurer_interruptions(false);
    }
}

void info() {

    afficher(&console_principale, "%CGraphiques:%C\n", COULEUR_BLEU_C, COULEUR_BLANC);
    afficher(&console_principale, "adresse buffer : %C%x%C\n", COULEUR_BLEU_C, informations_boot->video.adresse_buffer_image, COULEUR_BLANC);
    afficher(&console_principale, "taille : %C%i (%i / %i x %i)%C\n",
        COULEUR_BLEU_C,
        informations_boot->video.taille_buffer_image,
        informations_boot->video.resolution_horizontale,
        informations_boot->video.resolution_horizontale_reele,
        informations_boot->video.resolution_verticale,
        COULEUR_BLANC
    );

    afficher(&console_principale, "%CMemoire:%C\n", COULEUR_BLEU_C, COULEUR_BLANC);
    afficher(&console_principale, "adresse : %C%x%C\n", COULEUR_BLEU_C, ADRESSE_KERNEL, COULEUR_BLANC);
    afficher(&console_principale, "taille : %C%m (%i pages allouées)%C\n", COULEUR_BLEU_C, informations_boot->kernel.taille_kernel, informations_boot->kernel.pages_allouees, COULEUR_BLANC);
}
