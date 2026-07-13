#include "memoire_virtuelle.h"
#include "memoire_physique.h"
#include "../io/io.h"



void desactiver_protection_ecriture() {
    u64 cr0;
    __asm__ __volatile__("mov %%cr0, %0" : "=r"(cr0));
    __asm__ __volatile__("mov %0, %%cr0" :: "r"(cr0 & ~(1ULL << 16)));
}

void activer_protection_ecriture() {
    u64 cr0;
    __asm__ __volatile__("mov %%cr0, %0" : "=r"(cr0));
    __asm__ __volatile__("mov %0, %%cr0" :: "r"(cr0 | (1ULL << 16)));
}

u64* creer_table_adresse_virtuelle() {

    u64* pml4 = (u64*)allouer_page();
    return pml4;
}

u64 *obtenir_table_actuelle() {

    u64 cr3;
    __asm__ __volatile__("mov %%cr3, %0" : "=r"(cr3));
    // On masque les flags de CR3 pour ne garder que l'adresse physique
    return (u64*)(cr3 & MASQUE_ADRESSE); 
}

u64 obtenir_adresse_physique(u64 *pml4, u64 adresse_virtuelle) {
    
    u64 index_pml4 = PML4_INDEX(adresse_virtuelle);
    u64 entree_pml4 = pml4[index_pml4];
    if (!(entree_pml4 & PAGE_PRESENT)) return 0;

    u64 *pdpt = (u64*)(entree_pml4 & MASQUE_ADRESSE);
    u64 index_pdpt = PDPT_INDEX(adresse_virtuelle);
    u64 entree_pdpt = pdpt[index_pdpt];
    if (!(entree_pdpt & PAGE_PRESENT)) return 0;

    u64 *pd = (u64*)(entree_pdpt & MASQUE_ADRESSE);
    u64 index_pd = PD_INDEX(adresse_virtuelle);
    u64 entree_pd = pd[index_pd];
    if (!(entree_pd & PAGE_PRESENT)) return 0;

    u64 *pt = (u64*)(entree_pd & MASQUE_ADRESSE);
    u64 index_pt = PT_INDEX(adresse_virtuelle);
    u64 entree_pt = pt[index_pt];
    if (!(entree_pt & PAGE_PRESENT)) return 0;

    return (entree_pt & MASQUE_ADRESSE);
}

void lier_adresse_virtuelle(u64 *pml4, u64 adresse_virtuelle, u64 adresse_physique, u64 flags) {
    
    // 1. Niveau PML4
    u64 index_pml4 = PML4_INDEX(adresse_virtuelle);
    u64 entree_pml4 = pml4[index_pml4];
    u64 *pdpt;

    if (!(entree_pml4 & PAGE_PRESENT)) {
        // La table PDPT n'existe pas, on l'alloue
        
        pdpt = (u64*)allouer_page();
        // On lie le PML4 à la nouvelle PDPT avec les flags basiques
        pml4[index_pml4] = (u64)pdpt | PAGE_PRESENT | PAGE_RW | PAGE_USER;
        
    } else {
        // La table existe, on récupère son adresse physique (en retirant les flags)
        pdpt = (u64*)(entree_pml4 & MASQUE_ADRESSE);
    }

    // 2. Niveau PDPT
    u64 index_pdpt = PDPT_INDEX(adresse_virtuelle);
    u64 entree_pdpt = pdpt[index_pdpt];
    u64 *pd;

    if (!(entree_pdpt & PAGE_PRESENT)) {
        pd = (u64*)allouer_page();
        pdpt[index_pdpt] = (u64)pd | PAGE_PRESENT | PAGE_RW | PAGE_USER;
    } else {
        pd = (u64*)(entree_pdpt & MASQUE_ADRESSE);
    }
    
    // 3. Niveau PD
    u64 index_pd = PD_INDEX(adresse_virtuelle);
    u64 entree_pd = pd[index_pd];
    u64 *pt;

    if (!(entree_pd & PAGE_PRESENT)) {
        pt = (u64*)allouer_page();
        pd[index_pd] = (u64)pt | PAGE_PRESENT | PAGE_RW | PAGE_USER;
    } else {
        pt = (u64*)(entree_pd & MASQUE_ADRESSE);
    }

    // 4. Niveau PT (Page Table - le dernier niveau)
    u64 index_pt = PT_INDEX(adresse_virtuelle);
    
    // On inscrit l'adresse physique finale cible avec les flags demandés par l'utilisateur (lecture seule, R/W, user...)
    pt[index_pt] = (adresse_physique & MASQUE_ADRESSE) | flags;
}

bool allouer_pages_contigues(u64* pml4_actuel, u64 nb_pages, u64 adresse_virtuelle, u64 flags) {

    if (nb_pages == 0) return true;

    for (u64 offset = 0; offset < nb_pages; offset++) {

        void* page_physique = allouer_page();
        if (page_physique == Null) {
            return false;
        }

        lier_adresse_virtuelle(pml4_actuel, adresse_virtuelle + offset * TAILLE_PAGE, (u64)page_physique, flags);
    }

    return true;
}