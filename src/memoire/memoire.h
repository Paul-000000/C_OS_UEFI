#ifndef MEMOIRE_H
#define MEMOIRE_H

#include "../types.h"
#include "../kernel.h"



#define ADRESSE_VIRTUELLE_PILES 0x0000100000000000ULL
#define ADRESSE_VIRTUELLE_TAS   0x0000010000000000ULL
#define TAILLE_PAGE 4096



typedef struct Bloc_Memoire {
    u64 taille;
    bool est_libre;
    struct Bloc_Memoire *suivant;

} Bloc_Memoire;

typedef struct Gestionnaire_Memoire {

    u64 total_pages; // meme les pages deja alllouees par l'uefi
    u64 pages_libres;

    u64 pages_allouees_tas;
    u64 octets_allouees_tas;
    u64 pages_allouees_piles;

    Bloc_Memoire *premier_bloc;

} Gestionnaire_Memoire;



extern Gestionnaire_Memoire gestionnaire_memoire;



bool init_tas(Parametres_memoire *parametres, u64* pml4_actuel, u64 nb_pages);

bool allouer_pages_contigues(u64* pml4_actuel, u64 nb_pages, u64 adresse_virtuelle, u64 flags);

void *malloc(u64 taille);

void free(void *ptr);



u64* nouvelle_pile(u64 nb_pages);

u64 *recuperer_table_noyau();

void liberer_pile(u64 *pml4_processus, u64 nb_pages);



void afficher_memoire();

void afficher_tas();

void afficher_map_memoire(Parametres_memoire *parametres_memoire);



void *memmove(void *dest, const void *src, u64 n);

void* memcpy(void* dest, const void* src, u64 n);

void* memset(void* s, i32 c, u64 n);

#endif