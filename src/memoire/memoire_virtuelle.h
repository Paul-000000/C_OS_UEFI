#ifndef MEMOIRE_VIRTUELLE_H
#define MEMOIRE_VIRTUELLE_H



#include "../types.h"



// Extraction des index depuis une adresse virtuelle
#define PML4_INDEX(virt) (((virt) >> 39) & 0x1FF)
#define PDPT_INDEX(virt) (((virt) >> 30) & 0x1FF)
#define PD_INDEX(virt)   (((virt) >> 21) & 0x1FF)
#define PT_INDEX(virt)   (((virt) >> 12) & 0x1FF)

// Masques et Flags des entrées de table de pages
#define PAGE_PRESENT  0x01
#define PAGE_RW       0x02 // Read/Write
#define PAGE_USER     0x04 // Accessible en mode utilisateur
#define MASQUE_ADRESSE 0x000FFFFFFFFFF000ULL // Masque pour récupérer l'adresse physique (efface les flags)



void desactiver_protection_ecriture();

void activer_protection_ecriture();

void lier_adresse_virtuelle(u64 *pml4, u64 adresse_virtuelle, u64 adresse_physique, u64 flags);

u64 obtenir_adresse_physique(u64 *pml4, u64 adresse_virtuelle);

u64* obtenir_table_actuelle();

u64* creer_table_adresse_virtuelle();

bool allouer_pages_contigues(u64* pml4_actuel, u64 nb_pages, u64 adresse_virtuelle, u64 flags);



#endif