#include "memoire_physique.h"
#include "../string.h"
#include "../types.h"
#include "memoire.h"



u8 *bitmap = Null;
u64 dernier_index_alloue = 0; // Optimisation pour ne pas toujours chercher depuis 0



bool init_memoire_physique(Parametres_memoire *parametres) {

    gestionnaire_memoire.total_pages = 0;
    gestionnaire_memoire.pages_libres = 0;
    gestionnaire_memoire.pages_allouees_tas = 0;
    gestionnaire_memoire.octets_allouees_tas = 0;
    gestionnaire_memoire.pages_allouees_piles = 0;
    gestionnaire_memoire.premier_bloc = Null;

    u64 plus_haute_adresse = 0;

    // 1. Trouver la plus haute adresse pour connaître le nombre total de pages
    for (u64 i = 0; i < parametres->taille_map_memoire; i += parametres->taille_entree_memoire) {
        
        EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)((u8*)parametres->adresse_map_memoire + i);
        u64 fin_region = ((u64)(desc->PhysicalStart)) + (desc->NumberOfPages * TAILLE_PAGE);

        if (fin_region > plus_haute_adresse) {
            plus_haute_adresse = fin_region;
        }
    }

    if (plus_haute_adresse == 0) {
        return false;
    }

    gestionnaire_memoire.total_pages = plus_haute_adresse / TAILLE_PAGE;
    u64 taille_bitmap = gestionnaire_memoire.total_pages / 8;
    if (taille_bitmap % 8 != 0) taille_bitmap++; // Arrondir au supérieur

    // 2. Trouver un bloc EfiConventionalMemory assez grand pour stocker notre bitmap
    for (u64 i = 0; i < parametres->taille_map_memoire; i += parametres->taille_entree_memoire) {
        EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)((u8*)parametres->adresse_map_memoire + i);
        
        if ((u64)desc->PhysicalStart != 0 && desc->Type == EfiConventionalMemory && (desc->NumberOfPages * TAILLE_PAGE) >= taille_bitmap) {
            bitmap = (u8*)desc->PhysicalStart;
            break;
        }
    }

    if (bitmap == Null) {
        return false;
    }

    // 3. Initialiser tout à 1 (occupé par défaut par sécurité)
    memset(bitmap, 0xFF, taille_bitmap);

    // 4. Parcourir la carte mémoire et mettre à 0 (libre) les zones EfiConventionalMemory
    for (u64 i = 0; i < parametres->taille_map_memoire; i += parametres->taille_entree_memoire) {

        EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)((u8*)parametres->adresse_map_memoire + i);
        
        if (desc->Type == EfiConventionalMemory) {
            
            u64 index_depart = (u64)(desc->PhysicalStart) / TAILLE_PAGE;
            
            for (u64 j = 0; j < desc->NumberOfPages; j++) {
                CLEAR_BIT(index_depart + j);
                gestionnaire_memoire.pages_libres++;
            }
        }
    }

    // 5. Ne pas oublier de marquer les pages utilisées par le bitmap lui-même comme occupées !
    u64 index_bitmap = (u64)bitmap / TAILLE_PAGE;
    u64 pages_pour_bitmap = (taille_bitmap + TAILLE_PAGE - 1) / TAILLE_PAGE;
    for (u64 i = 0; i < pages_pour_bitmap; i++) {
        SET_BIT(index_bitmap + i);
        gestionnaire_memoire.pages_libres--;
    }

    // page 0 
    if (!TEST_BIT(0)) {
        SET_BIT(0);
        gestionnaire_memoire.pages_libres--;
    }

    return true;
}

void *allouer_page() {

    // Recherche de la première page libre
    for (u64 i = dernier_index_alloue; i < gestionnaire_memoire.total_pages; i++) {
        
        if (!TEST_BIT(i)) {
            SET_BIT(i);
            gestionnaire_memoire.pages_libres--;
            dernier_index_alloue = i;
            
            void* adresse_physique = (void*)(i * TAILLE_PAGE);
            // Toujours renvoyer une page propre
            memset(adresse_physique, 0, TAILLE_PAGE); 
            return adresse_physique;
        }
    }
    
    // Si on arrive ici, il faut reboucler depuis le début (0 à dernier_index_alloue)
    // (Simplifié pour l'exemple, à rajouter pour être robuste)
    return Null; 
}

void liberer_page(void* adresse_physique) {

    u64 index = (u64)adresse_physique / TAILLE_PAGE;
    if (TEST_BIT(index)) {

        CLEAR_BIT(index);
        gestionnaire_memoire.pages_libres++;
        if (index < dernier_index_alloue) {
            dernier_index_alloue = index; // Optimisation du prochain scan
        }
    }
}