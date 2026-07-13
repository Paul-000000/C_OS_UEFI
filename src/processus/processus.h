#ifndef __PROCESSUS_H__
#define __PROCESSUS_H__



#include "../types.h"
#include "../vecteur.h"
#include "../string.h"



typedef enum etat_processus {
    ELU,
    ACTIVABLE,
    ENDORMI,
    MORT,
    A_LIBERER
} Etat_processus;

typedef struct processus {

    Str *nom;
    Etat_processus etat;
    u64 date_reveil;
    u64 registres[9]; // [0]=rbx, [1]=rbp, [2]=r12, [3]=r13, [4]=r14, [5]=r15, [6]=rdi, [7]=rip, [8]=rsp
    u64* table_pml4;
    u64 *pile;
    u64 nb_pages_piles;

} Processus;

DEFINIR_VECTEUR_TYPE(Processus *, Vec_Processus)



extern Vec_Processus *processus;
extern i64 pid_actif;
extern u64 nb_processus;



bool init_processus();

int64_t cree_processus(void code(), Str *nom, u64 nb_pages);

bool ordonnance(bool sauvegarde);

extern void context_switch(u64 *anciens_registres, u64 *nouveaux_registres, u64 nouveau_cr3);

u64 taille_pile(i64 pid);

Str *nom_processus(i64 pid);

bool tuer_processus(i64 pid);

bool reinitialiser_processus();

bool endormir_processus(i64 pid, u64 secondes);

bool fin_processus();

bool lanceur_processus(void proc());

void nom_etat_processus(Etat_processus etat, char nom[10]);

void afficher_processus();



#endif