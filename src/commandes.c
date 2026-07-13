#include "console.h"
#include "interruptions/interruptions.h"
#include "processus/processus.h"
#include "io/clavier.h"
#include "string.h"
#include "types.h"
#include "kernel.h"
#include "commandes.h"
#include "vecteur.h"
#include "memoire/memoire.h"
#include "timer.h"
#include "io/io.h"



DEFINIR_VECTEUR_TYPE(Commande, Vec_Commandes)

Vec_Commandes *commandes;



void help();
void ps();
void kill();
void time();
void proc();
void clear();
void map();
void erreur_debordement_memoire();
void erreur_division_par_zero();
void erreur_recursion();



void ajouter_commande(Str *nom, void (*fonction)()) {

    Commande commande = {nom, fonction};

    ajouter_Vec_Commandes(commandes, commande);
}

void interpreteur_commandes() {

    commandes = creer_Vec_Commandes(9);

    ajouter_commande(Str_creer("help"), help);
    ajouter_commande(Str_creer("ps"), ps);
    ajouter_commande(Str_creer("kill"), kill);
    ajouter_commande(Str_creer("time"), time);
    ajouter_commande(Str_creer("proc"), proc);
    ajouter_commande(Str_creer("info"), info);
    ajouter_commande(Str_creer("clear"), clear);
    ajouter_commande(Str_creer("tas"), afficher_tas);
    ajouter_commande(Str_creer("map"), map);
    ajouter_commande(Str_creer("err_mem"), erreur_debordement_memoire);
    ajouter_commande(Str_creer("err_div_0"), erreur_division_par_zero);
    ajouter_commande(Str_creer("err_rec"), erreur_recursion);

    Str *nom_commande = Str_vide();
    help();

    while (true) {
        
        bool en_saisie = true; // Flag pour contrôler la boucle de saisie

        while (en_saisie) { 
            
            char touche = recuperer_caractere();

            switch (touche) {
                case '\n':
                    en_saisie = false; // On indique qu'on a fini de taper
                    break;

                case '\b':
                    if (nom_commande->taille > 0) {
                        Str_supprimer(nom_commande, nom_commande->taille - 1);
                        afficher(&console_principale, "\b");
                    }
                    break;

                case FLECHE_H:
                    defiler(&console_principale, false);
                    break;

                case FLECHE_B:
                    defiler(&console_principale, true);
                    break;

                default:
                    Str_ajouter_char(nom_commande, touche);
                    afficher(&console_principale, "%C%c%C", COULEUR_BLEU, touche, COULEUR_BLANC);
                    break;      
            }
        }

        // L'exécution de la commande se fait bien ICI, à chaque tour du shell
        afficher(&console_principale, "\n");
        bool commande_trouvee = false;

        for (u16 j = 0; j < commandes->taille; j++) {
            if (Str_egaux(commandes->elements[j].nom, nom_commande)) {
                i64 pid = cree_processus(commandes->elements[j].fonction, commandes->elements[j].nom, 1);
                
                if (pid == -1) {
                    afficher(&console_principale, "%CErreur lors de la création du processus%C\n", COULEUR_ROUGE, COULEUR_BLANC);
                }

                commande_trouvee = true;
                break;
            }
        }

        if (!commande_trouvee) {
            afficher(&console_principale, "%CCommande inconnue: \"%S\"%C\n", COULEUR_ROUGE, nom_commande, COULEUR_BLANC);
        }

        Str_vider(nom_commande);
    }
}


void clear() {

    effacer_console(&console_principale);
    afficher(&console_principale, "console effacee\n");
    afficher_console(&console_principale);
}

void help() {

    afficher(&console_principale, "%i Commandes disponibles :\n", commandes->taille);

    for (u16 i = 0; i < commandes->taille; i++) {
        afficher(&console_principale, "- %C%S%C\n", COULEUR_BLEU_C, commandes->elements[i].nom, COULEUR_BLANC);
    }

    afficher(&console_principale, "%C", COULEUR_BLANC);
}

void ps() {

    afficher(&console_principale, "%d Processus en cours d'execution :\n", nb_processus);
    afficher(&console_principale, "%C%F%3s%C%F | %C%F%25s%C%F | %C%F%10s%C%F | %C%F%s%C%F\n",
        COULEUR_GRIS_F,
        COULEUR_BLEU_C,
        "PID", 
        COULEUR_BLANC,
        COULEUR_GRIS_F,
        COULEUR_GRIS_F,
        COULEUR_BLEU_C,
        "Nom",
        COULEUR_BLANC,
        COULEUR_GRIS_F,
        COULEUR_GRIS_F,
        COULEUR_BLEU_C,
        "Etat",
        COULEUR_BLANC,
        COULEUR_GRIS_F,
        COULEUR_GRIS_F,
        COULEUR_BLEU_C,
        "Taille pile en pages",
        COULEUR_BLANC,
        COULEUR_GRIS_F
    );

    char nom_temp[10];

    for (u64 i = 0; i < processus->taille; i++) {

        Processus *p = get_Vec_Processus(processus, i);
        if (p == Null) {
            continue;
        }

        nom_etat_processus(p->etat, nom_temp);

        afficher(&console_principale, "%C%3i%C | %C%25S%C | %C%10s%C | %C%u%C\n",
            COULEUR_BLEU_C,
            i,
            COULEUR_BLANC,
            COULEUR_BLEU_C,
            p->nom,
            COULEUR_BLANC,
            COULEUR_BLEU_C,
            nom_temp,
            COULEUR_BLANC,
            COULEUR_BLEU_C,
            p->nb_pages_piles,
            COULEUR_BLANC
        );
    }
}

void kill() {

    u8 nb_processus = processus->taille;

    afficher(&console_principale, "%i processus tues\n", nb_processus - 1);

    for (u64 i = 1; i < processus->taille; i++) {
        
        afficher(&console_principale, "%C%S (%i)%C\n",
            COULEUR_BLEU_C,
            nom_processus(i),
            i,
            COULEUR_BLANC
        );

        if (i != pid_actif) { // on évite le processus kill lui-même
            tuer_processus(i);
        }
    }

    cree_processus(interpreteur_commandes, Str_creer("interpreteur de commandes"), 1);

    afficher(&console_principale, "\n");
}

void time() {

    u64 secondes = secondes_ecoulees();

    afficher(&console_principale, "temps ecoule : %C%02i:%02i:%02i%C\n",
        COULEUR_BLEU_C,
        secondes / 3600,
        secondes % 3600 / 60,
        secondes % 60,
        COULEUR_BLANC
    );
}

void proc() {

    while(true) {
        
        afficher(&console_principale, "\n[temps: %C%i%C] processus: %C%S (pid: %i)%C\n",
            COULEUR_BLEU_C,
            secondes_ecoulees(),
            COULEUR_BLANC,
            COULEUR_BLEU_C,
            nom_processus(pid_actif),
            pid_actif,
            COULEUR_BLANC
        );

        endormir_processus(pid_actif, 5);
    }
}

void map() {

    afficher_map_memoire(&(informations_boot->memoire));
}



void erreur_debordement_memoire() {

    u8 buffer[4];
    volatile u8 index = 4;
    buffer[index] = 0;
}

void erreur_division_par_zero() {

    volatile u8 denominateur = 0;
    volatile u8 resultat;
    resultat = 42 / denominateur; 

    (void)resultat; 
}

void erreur_recursion() {

    volatile bool vrai = true;

    if (vrai) {
        erreur_recursion();
    }
}