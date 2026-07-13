#include "types.h"
#include "timer.h"
#include "console.h"
#include "processus/processus.h"
#include "io/clavier.h"
#include "io/affichage.h"
#include "io/io.h"



#define PIT_PORT_CHANNEL0 0x40
#define PIT_PORT_COMMAND  0x43



volatile u64 ticks_interruption = 0;



u64 secondes_ecoulees() {

    return ticks_interruption / FREQUENCE_INTERRUPTION;
}

void init_timer(u32 frequence_hz) {
    
    u32 diviseur = 1193182 / frequence_hz; // La formule standard

    outb(PIT_PORT_COMMAND, 0x36);
    outb(PIT_PORT_CHANNEL0, (u8)(diviseur & 0xFF));
    outb(PIT_PORT_CHANNEL0, (u8)((diviseur >> 8) & 0xFF));
}

void afficher_temps() {

    placer_curseur(&console_informations, 0, 0);

    Ligne_caracteres *ligne = get_Lignes_caracteres(console_informations.lignes, 0);
    vider_Ligne_caracteres(ligne);

    effacer_ligne(&console_informations, 0);

    u64 secondes = secondes_ecoulees();

    afficher(&console_informations, "%F%C[Temps]%F%C %02i:%02i:%02i",
        COULEUR_BLEU,
        COULEUR_BLANC,
        COULEUR_BLANC,
        COULEUR_GRIS_F,
        secondes / 3600,
        secondes % 3600 / 60,
        secondes % 60
    );
}

void gestion_timer(u64 code) {

    ticks_interruption++;

    if (ticks_interruption % FREQUENCE_INTERRUPTION == 0) {
        afficher_temps();
    }

    afficher_processus();
    afficher_touches();
    afficher_memoire();
    afficher_affichage();

    mise_a_jour_affichage();

    outb(0x20, 0x20); // Envoyer fin de traitement de l'interruption

    // if (ticks_interruption == 2) {
    //     erreur_fatale(-1, "Erreur de programmation du timer");
    // }

    ordonnance(true);
}


