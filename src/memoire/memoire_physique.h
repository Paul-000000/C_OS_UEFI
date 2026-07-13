#ifndef MEMOIRE_PHYSIQUE_H
#define MEMOIRE_PHYSIQUE_H



#include "../types.h"
#include "../uefi/uefi.h" // Pour EFI_MEMORY_DESCRIPTOR

#define TAILLE_PAGE 4096



// Macros pour manipuler les bits
#define SET_BIT(index)   (bitmap[(index) / 8] |= (1 << ((index) % 8)))
#define CLEAR_BIT(index) (bitmap[(index) / 8] &= ~(1 << ((index) % 8)))
#define TEST_BIT(index)  (bitmap[(index) / 8] & (1 << ((index) % 8)))



bool init_memoire_physique(Parametres_memoire *parametres);

void *allouer_page();

void liberer_page(void* adresse_physique);



#endif