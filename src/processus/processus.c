#include "processus.h"
#include "../console.h"
#include "../interruptions/interruptions.h"
#include "../types.h"
#include "../string.h"
#include "../memoire/memoire.h"
#include "../memoire/memoire_virtuelle.h"
#include "../vecteur.h"
#include "../timer.h"
#include "../io/io.h"



Vec_Processus *processus;
i64 pid_actif;
u64 nb_processus;



void nettoyer_processus_morts();

bool init_processus() {

    processus = creer_Vec_Processus(4);
    if (processus == Null) {
        return false;
    }

    Processus *p = (Processus*)malloc(sizeof(Processus));
    if (p == Null) {
        return false;
    }

    p->etat = ELU;
    memset(p->registres, 0, sizeof(u64) * 9);
    p->nom = Str_creer("Kernel main");
    p->nb_pages_piles = 0;
    p->table_pml4 = recuperer_table_noyau();
    p->pile = Null;

    if (!ajouter_Vec_Processus(processus, p)) {
        free(p);
        return false;
    }

    pid_actif = 0;
    nb_processus = 1;
    return true;
}

i64 cree_processus(void code(), Str *nom, u64 nb_pages) {

    nettoyer_processus_morts();

    u64 pid = 0;
    bool slot_trouve = false;
    
    // 1. On cherche un emplacement MORT (on renomme la variable locale pour éviter le shadowing)
    for (; pid < processus->taille; pid++) {

        Processus *p_verif = get_Vec_Processus(processus, pid);
        
        if (p_verif != Null && p_verif->etat == MORT) {
        
            slot_trouve = true;
            break;
        }
    }

    Processus *p_cible = Null;

    // 2. Si aucun emplacement MORT n'a été trouvé, on doit agrandir le vecteur
    if (!slot_trouve) {

        p_cible = (Processus*)malloc(sizeof(Processus));
        if (p_cible == Null) {
            return -1;
        }

        if (!ajouter_Vec_Processus(processus, p_cible)) {
            free(p_cible);
            return -1;
        }
        pid = processus->taille - 1;

    } else {
        // Si on recycle un slot MORT, il est déjà alloué sur le tas
        p_cible = get_Vec_Processus(processus, pid);
        if (p_cible == Null) {
            return -1;
        }
    }

    // 3. Initialisation des données directement dans le bloc du tas
    p_cible->etat = ACTIVABLE;
    p_cible->nom = nom;
    memset(p_cible->registres, 0, sizeof(u64) * 9);
    p_cible->nb_pages_piles = nb_pages;

    p_cible->pile = (u64*)(ADRESSE_VIRTUELLE_PILES - (nb_pages * TAILLE_PAGE));
    p_cible->table_pml4 = nouvelle_pile(nb_pages);
    if (p_cible->table_pml4 == Null) {
        return -1;
    }

    p_cible->registres[6] = (u64)code; // RDI
    p_cible->registres[7] = (u64)lanceur_processus; // RIP
    p_cible->registres[8] = ADRESSE_VIRTUELLE_PILES - 8; // RSP aligné 16 octets
    p_cible->registres[1] = ADRESSE_VIRTUELLE_PILES;
    
    nb_processus++;
    return pid; 
}

void processus_suivant() {

    while(true) {
        
        pid_actif = (pid_actif + 1) % processus->taille;

        Processus *p = get_Vec_Processus(processus, pid_actif);
        if (p == Null) {
            continue;
        }

        Etat_processus etat = p->etat;

        if (etat == ACTIVABLE || etat == ELU) {
            break;
        }
    }
}

void reveille_processus() {

    for (u64 i = 0; i < processus->taille; i++) {

        Processus *p = get_Vec_Processus(processus, i);
        if (p == Null) {
            continue;
        }

        if (p->etat == ENDORMI && secondes_ecoulees() >= p->date_reveil) {
            p->etat = ACTIVABLE;
        }
    }
}

bool ordonnance(bool sauvegarde) {

    u64 pid_ancien_processus = pid_actif;
    Processus *ancien;

    reveille_processus();
    nettoyer_processus_morts();
    processus_suivant();

    if (sauvegarde) {

        ancien = get_Vec_Processus(processus, pid_ancien_processus);
        if (ancien == Null) {
            return false; // Échec de la récupération de l'ancien processus
        }

        if (ancien->etat == ELU) {
            ancien->etat = ACTIVABLE;
        }
    }

    Processus *nouveau = get_Vec_Processus(processus, pid_actif);
    if (nouveau == Null) {
        return false; // Échec de la récupération du nouveau processus
    }

    nouveau->etat = ELU;
    u64 *anciens_registres = sauvegarde ? &(ancien->registres[0]) : Null;

    // Mettre à jour CR3 force le processeur à utiliser le nouvel espace virtuel
    u64 adresse_physique_pml4 = (u64)nouveau->table_pml4;

    context_switch(anciens_registres, &(nouveau->registres[0]), adresse_physique_pml4);

    return true;
}

Str *nom_processus(i64 pid) {

    Processus *p = get_Vec_Processus(processus, pid);
    if (p == Null) {
        return Null; // Échec de la récupération du processus
    }

    return p->nom;
}

u64 taille_pile(i64 pid) {

    Processus *p = get_Vec_Processus(processus, pid);
    if (p == Null) {
        return 0;
    }

    return p->nb_pages_piles;
}

bool tuer_processus(i64 pid) {
    
    if (pid == 0) {
        return false; // Ne peut pas tuer le processus kernel
    }

    Processus *p = get_Vec_Processus(processus, pid);
    if (p == Null) {
        return false;
    }

    // Sécurité : si le processus est déjà mort, on ne fait rien
    if (p->etat == MORT || p->etat == A_LIBERER) {
        return true;
    }

    p->etat = A_LIBERER;
    nb_processus--;

    // CHANGER ICI : On ne bascule immédiatement que si le processus SE TUE LUI-MÊME
    if (pid == pid_actif) {
        return ordonnance(false);
    }

    return true;
}

bool reinitialiser_processus() {

    for (u64 pid = 1; pid < processus->taille; pid++) {

        Processus *p = get_Vec_Processus(processus, pid);
        if (p == Null) {
            continue;
        }

        p->etat = A_LIBERER;
    }

    nb_processus = 1;

    return ordonnance(false);
}

void nettoyer_processus_morts() {

    for (u64 i = 0; i < processus->taille; i++) {
        
        Processus *p = get_Vec_Processus(processus, i);
        
        if (p != Null && p->etat == A_LIBERER) {
            
            liberer_pile(p->table_pml4, p->nb_pages_piles);
            
            p->etat = MORT;
        }
    }
}

bool endormir_processus(i64 pid, u64 secondes) {

    if (pid == 0) {
        return false;
    }

    Processus *p = get_Vec_Processus(processus, pid);
    if (p == Null) {
        return false;
    }

    p->etat = ENDORMI;
    p->date_reveil = secondes_ecoulees() + secondes;

    return ordonnance(true); 
}

bool fin_processus() {

    if (pid_actif == 0) {
        return false;
    }

    Processus *p = get_Vec_Processus(processus, pid_actif);
    if (p == Null) {
        return false;
    }

    p->etat = A_LIBERER;
    nb_processus--;

    return ordonnance(true);
}

bool lanceur_processus(void proc()) {

    configurer_interruptions(true);
    proc();
    return fin_processus();
}

void nom_etat_processus(Etat_processus etat, char nom[10]) {

    switch (etat) {

        case ELU:
            strncpy(nom, "Elu", 10);
            break;

        case ACTIVABLE:
            strncpy(nom, "Activable", 10);
            break;

        case ENDORMI:
            strncpy(nom, "Endormi", 10);
            break;

        case MORT:
            strncpy(nom, "Mort", 10);
            break;
        
        case A_LIBERER:
            strncpy(nom, "A liberer", 10);

        default:
            strncpy(nom, "Inconnu", 10);
            break;
    }
}

void afficher_processus() {

    placer_curseur(&console_informations, 1, 0);

    Ligne_caracteres *ligne = get_Lignes_caracteres(console_informations.lignes, 1);
    vider_Ligne_caracteres(ligne);

    effacer_ligne(&console_informations, 1);

    afficher(&console_informations, "%F%C[Processus]%F%C %i%C elu: %C%25S%C (%Cpid: %i %u pages%C)",
        COULEUR_BLEU,
        COULEUR_BLANC,
        COULEUR_BLANC,
        COULEUR_GRIS_F,
        nb_processus,
        COULEUR_GRIS_F,
        COULEUR_BLEU_C,
        nom_processus(pid_actif),
        COULEUR_GRIS_F,
        COULEUR_BLEU,
        pid_actif,
        taille_pile(pid_actif),
        COULEUR_GRIS_F
    );
}

