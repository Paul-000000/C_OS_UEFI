#include "../types.h"
#include "memoire.h"
#include "../kernel.h"
#include "../interruptions/interruptions.h"
#include "../io/io.h"
#include "memoire_physique.h"
#include "memoire_virtuelle.h"
#include "../io/affichage.h"



Gestionnaire_Memoire gestionnaire_memoire;
u64 *table_plm4_noyau;



// gestion du tas
void fusionner_blocs_libres();

bool init_tas(Parametres_memoire *parametres, u64* pml4_actuel, u64 nb_pages) {

    table_plm4_noyau = pml4_actuel;

    bool memoire_physique_initialisee = init_memoire_physique(&informations_boot->memoire);
    if (!memoire_physique_initialisee) {
        return false;
    }

    gestionnaire_memoire.octets_allouees_tas = 0;
    gestionnaire_memoire.pages_allouees_tas = nb_pages;

    desactiver_protection_ecriture();

    bool res = allouer_pages_contigues(pml4_actuel, nb_pages, ADRESSE_VIRTUELLE_TAS, PAGE_PRESENT | PAGE_RW);
    if (!res) {
        return false;
    }
    
    activer_protection_ecriture();
    
    u64 cr3;
    __asm__ __volatile__("mov %%cr3, %0" : "=r"(cr3));
    __asm__ __volatile__("mov %0, %%cr3" :: "r"(cr3));

    u64 taille_tas_octets = nb_pages * TAILLE_PAGE;
    Bloc_Memoire *premier = (Bloc_Memoire*)ADRESSE_VIRTUELLE_TAS;
    
    premier->taille = taille_tas_octets - sizeof(Bloc_Memoire);
    premier->est_libre = true;
    premier->suivant = Null;

    gestionnaire_memoire.premier_bloc = premier;

    return true;
}

bool agrandir_tas(u64 nb_pages) {

    if (nb_pages == 0) return true;

    // 1. Calculer l'adresse virtuelle du début de la nouvelle zone
    // Le tas commence à ADRESSE_VIRTUELLE_TAS
    u64 adresse_nouvelle_zone = ADRESSE_VIRTUELLE_TAS + gestionnaire_memoire.pages_allouees_tas * TAILLE_PAGE;

    // 2. Mapper les nouvelles pages dans la table de page actuelle (PML4)
    // On utilise table_plm4_noyau car le tas est partagé
    desactiver_protection_ecriture();
    bool res = allouer_pages_contigues(table_plm4_noyau, nb_pages, adresse_nouvelle_zone, PAGE_PRESENT | PAGE_RW);
    activer_protection_ecriture();

    if (!res) {
        return false; // Échec de l'allocation physique ou du mapping
    }

    // AJOUT : Recharger CR3 pour forcer la mise à jour du cache TLB
    u64 cr3;
    __asm__ __volatile__("mov %%cr3, %0" : "=r"(cr3));
    __asm__ __volatile__("mov %0, %%cr3" :: "r"(cr3));

    // 3. Créer un nouveau bloc mémoire libre pour cette zone
    Bloc_Memoire *nouveau_bloc = (Bloc_Memoire*)adresse_nouvelle_zone;
    
    nouveau_bloc->taille = (nb_pages * TAILLE_PAGE) - sizeof(Bloc_Memoire);
    nouveau_bloc->est_libre = true;
    nouveau_bloc->suivant = Null;

    // 4. Lier ce nouveau bloc à la fin de la liste chaînée existante
    if (gestionnaire_memoire.premier_bloc == Null) {
        gestionnaire_memoire.premier_bloc = nouveau_bloc;
    } else {
        Bloc_Memoire *courant = gestionnaire_memoire.premier_bloc;
        while (courant->suivant != Null) {
            courant = courant->suivant;
        }
        courant->suivant = nouveau_bloc;
    }

    fusionner_blocs_libres();

    // 5. Mettre à jour le gestionnaire de mémoire
    gestionnaire_memoire.pages_allouees_tas += nb_pages;

    return true;
}

void *malloc(u64 taille) {
    u64 etat = sauvegarder_et_desactiver_interruptions();

    taille = (taille + 7) & ~7; // Alignement sur 8 octets

    // On utilise une boucle infinie plutôt qu'une récursion
    while (true) {
        Bloc_Memoire *courant = gestionnaire_memoire.premier_bloc;

        // 1. Tentative de trouver un bloc existant
        while (courant != Null) {
            if (courant->est_libre && courant->taille >= taille) {
                
                if (courant->taille >= (taille + sizeof(Bloc_Memoire) + 8)) {
                    Bloc_Memoire *nouveau = (Bloc_Memoire*)((u8*)courant + sizeof(Bloc_Memoire) + taille);
                    nouveau->taille = courant->taille - taille - sizeof(Bloc_Memoire);
                    nouveau->est_libre = true;
                    nouveau->suivant = courant->suivant;

                    courant->taille = taille;
                    courant->suivant = nouveau;
                    gestionnaire_memoire.octets_allouees_tas += taille;
                } else {
                    gestionnaire_memoire.octets_allouees_tas += courant->taille;
                }

                courant->est_libre = false;
                restaurer_interruptions(etat); // Libération propre
                return (void*)((u8*)courant + sizeof(Bloc_Memoire));
            }
            courant = courant->suivant;
        }

        // 2. Si on arrive ici, aucun bloc n'est assez grand. Il faut agrandir le tas.
        // Calcul du strict minimum de pages nécessaires pour ce bloc (taille utile + en-tête)
        u64 taille_totale_requise = taille + sizeof(Bloc_Memoire);
        u64 pages_necessaires = (taille_totale_requise + TAILLE_PAGE - 1) / TAILLE_PAGE;

        // Stratégie d'allocation : On demande au moins 16 pages (64 Ko) pour éviter de fragmenter 
        // les tables de pages, sauf si la demande initiale est plus grande.
        u64 pages_a_allouer = (pages_necessaires < 16) ? 16 : pages_necessaires;

        // Sécurité : Si le système n'a plus assez de pages pour notre stratégie de 16 pages, 
        // on rebascule sur le strict minimum nécessaire.
        if (gestionnaire_memoire.pages_libres < pages_a_allouer) {
            if (gestionnaire_memoire.pages_libres >= pages_necessaires) {
                pages_a_allouer = pages_necessaires;
            } else {
                // Plus du tout assez de mémoire physique dans tout l'OS !
                restaurer_interruptions(etat); // /!\ TRÈS IMPORTANT : on restaure avant de partir
                return Null; 
            }
        }

        // 3. On tente l'agrandissement
        if (!agrandir_tas(pages_a_allouer)) {
            // Échec du mapping virtuel ou de l'allocation
            restaurer_interruptions(etat); // /!\ TRÈS IMPORTANT
            return Null;
        }

        // L'agrandissement a réussi ! La boucle 'while(true)' va recommencer le parcours 
        // et trouvera à coup sûr le nouveau bloc qui vient d'être lié et fusionné.
    }
}

void free(void *ptr) {
    
    if (ptr == Null) return;

    u64 etat = sauvegarder_et_desactiver_interruptions();

    Bloc_Memoire *bloc = (Bloc_Memoire*)((u8*)ptr - sizeof(Bloc_Memoire));
    bloc->est_libre = true;
    gestionnaire_memoire.octets_allouees_tas -= bloc->taille;

    fusionner_blocs_libres();

    restaurer_interruptions(etat);
}

void fusionner_blocs_libres() {
    
    u64 etat = sauvegarder_et_desactiver_interruptions();

    Bloc_Memoire *courant = gestionnaire_memoire.premier_bloc;

    while (courant != Null) {
        if (courant->est_libre && courant->suivant != Null && courant->suivant->est_libre) {
            courant->taille += sizeof(Bloc_Memoire) + courant->suivant->taille;
            courant->suivant = courant->suivant->suivant;
            continue; 
        }
        courant = courant->suivant;
    }

    restaurer_interruptions(etat);
}



// gestion des piles
// Si chaque processus a son propre PML4, PLUS BESOIN de multiplier par le PID !
bool configurer_pile_processus_isole(u64 *pml4_du_processus, u64 nb_pages_pile) {
    
    // Le haut de la pile est TOUJOURS le même pour tout le monde
    u64 adresse_haut_pile = ADRESSE_VIRTUELLE_PILES; 

    // On mappe les pages utiles de la pile
    for (u64 i = 0; i < nb_pages_pile; i++) {
        u64 addr_virtuelle_page = adresse_haut_pile - ((i + 1) * TAILLE_PAGE);
        void *page_physique = allouer_page();
        lier_adresse_virtuelle(pml4_du_processus, addr_virtuelle_page, (u64)page_physique, PAGE_PRESENT | PAGE_RW | PAGE_USER);
    }
    
    // La page de garde est TOUJOURS à la même adresse virtuelle pour tout le monde
    u64 addr_virtuelle_garde = adresse_haut_pile - ((nb_pages_pile + 1) * TAILLE_PAGE);
    lier_adresse_virtuelle(pml4_du_processus, addr_virtuelle_garde, 0, 0); // Non présente
    
    return true;
}

u64 *recuperer_table_noyau() {

    table_plm4_noyau = obtenir_table_actuelle();

    return table_plm4_noyau;
}

u64* dupliquer_pml4_noyau() {

    // 1. Allouer une page vierge pour le nouveau PML4 du processus
    u64* nouveau_pml4 = creer_table_adresse_virtuelle();
    if (nouveau_pml4 == Null) return Null;

    // 3. Copier les entrées du noyau dans le nouveau PML4
    // Si tu es en Identity Mapping complet, tu peux copier les 512 entrées.
    // Si tu as séparé le noyau dans la moitié supérieure, copie uniquement les entrées correspondantes.
    for (u32 i = 0; i < 512; i++) {
        nouveau_pml4[i] = table_plm4_noyau[i];
    }

    return nouveau_pml4;
}

u64* nouvelle_pile(u64 nb_pages) {

    u64 *pml4 = dupliquer_pml4_noyau();
    
    if (pml4 == Null) {
        return Null;
    }

    // 2. Configurer la pile dans CE pml4 spécifique à l'adresse fixe
    desactiver_protection_ecriture();
    bool pile_ok = configurer_pile_processus_isole(pml4, nb_pages);
    activer_protection_ecriture();

    if (!pile_ok) {
        return Null;
    }

    gestionnaire_memoire.pages_allouees_piles += nb_pages;
    return pml4;
}

void liberer_pile(u64 *pml4_processus, u64 nb_pages) {
    
    if (pml4_processus == Null) return;

    desactiver_protection_ecriture();

    for (u64 i = 0; i < nb_pages; i++) {
        // Calcule l'adresse virtuelle de la page de la pile (en descendant depuis le sommet)
        u64 addr_virtuelle_page = (u64)ADRESSE_VIRTUELLE_PILES - ((i + 1) * TAILLE_PAGE);
        
        // Retrouve l'adresse physique réelle associée à cette page virtuelle
        u64 adresse_physique = obtenir_adresse_physique(pml4_processus, addr_virtuelle_page);
        
        if (adresse_physique != 0) {
            // Libère la page physique dans le bitmap
            liberer_page((void *)adresse_physique);
            
            // Nettoie la table des pages virtuelle en la démappant (flags à 0)
            lier_adresse_virtuelle(pml4_processus, addr_virtuelle_page, 0, 0);
        }
    }

    // Pour la page de garde : on ne désalloue RIEN physiquement car elle n'a pas de page physique.
    // On se contente d'effacer son entrée virtuelle par sécurité.
    u64 addr_virtuelle_garde = (u64)ADRESSE_VIRTUELLE_PILES - ((nb_pages + 1) * TAILLE_PAGE);
    lier_adresse_virtuelle(pml4_processus, addr_virtuelle_garde, 0, 0);

    activer_protection_ecriture();

    gestionnaire_memoire.pages_allouees_piles -= nb_pages;
}




void afficher_memoire() {

    placer_curseur(&console_informations, 3, 0);

    Ligne_caracteres *ligne = get_Lignes_caracteres(console_informations.lignes, 3);
    vider_Ligne_caracteres(ligne);
    
    effacer_ligne(&console_informations, 3);

    afficher(&console_informations, "%F%C[Memoire]%F%C %C pages libres %u (%m) / %u (%m)%C   Tas : %C%m (%u octets) / %u pages (%m)%C   Piles : %C%u pages (%m)%C\n",
        COULEUR_BLEU,
        COULEUR_BLANC,
        COULEUR_BLANC,
        COULEUR_GRIS_F,

        COULEUR_BLEU_C,
        gestionnaire_memoire.pages_libres,
        gestionnaire_memoire.pages_libres * TAILLE_PAGE,
        gestionnaire_memoire.total_pages,
        gestionnaire_memoire.total_pages * TAILLE_PAGE,
        COULEUR_GRIS_F,
        
        COULEUR_BLEU_C,
        gestionnaire_memoire.octets_allouees_tas,
        gestionnaire_memoire.octets_allouees_tas,
        gestionnaire_memoire.pages_allouees_tas,
        gestionnaire_memoire.pages_allouees_tas * TAILLE_PAGE,
        COULEUR_GRIS_F,

        COULEUR_BLEU_C,
        gestionnaire_memoire.pages_allouees_piles,
        gestionnaire_memoire.pages_allouees_piles * TAILLE_PAGE,
        COULEUR_GRIS_F
    );
}

void afficher_tas() {
    
    u64 nb_blocs = 0;
    u64 nb_allocations = 0;


    Bloc_Memoire *courant = gestionnaire_memoire.premier_bloc;
    while (courant != Null) { // 1er passage
    
        if (!courant->est_libre) {
            nb_allocations++;
        }
    
        nb_blocs++;
        courant = courant->suivant;
    }


    Vec_u64 *adresses = creer_Vec_u64(nb_blocs * 2);
    Vec_u64 *tailles = creer_Vec_u64(nb_blocs * 2);
    Vec_bool *etats = creer_Vec_bool(nb_blocs * 2);


    courant = gestionnaire_memoire.premier_bloc;
    while (courant != Null) {
        
        ajouter_Vec_u64(adresses, (u64)courant);
        ajouter_Vec_u64(tailles, courant->taille);
        ajouter_Vec_bool(etats, courant->est_libre);

        courant = courant->suivant;
    }


    afficher(&console_principale, "%Ctaille allouee %m (%u octets) sur %u pages (%m)\n",
        COULEUR_BLEU_C,
        gestionnaire_memoire.octets_allouees_tas,
        gestionnaire_memoire.octets_allouees_tas,
        gestionnaire_memoire.pages_allouees_tas,
        gestionnaire_memoire.pages_allouees_tas * TAILLE_PAGE
    );

    afficher(&console_principale, "%i/%i%C blocs libres :\n", nb_blocs - nb_allocations, nb_blocs, COULEUR_BLANC);
    
    afficher(&console_principale, "%C%F%3s%C%F | %C%F%18s%C%F | %C%F%12s%C%F\n",
        COULEUR_GRIS_F, COULEUR_BLEU_C, "Num", 
        COULEUR_BLANC, COULEUR_GRIS_F, COULEUR_GRIS_F, COULEUR_BLEU_C, "Adresse",
        COULEUR_BLANC, COULEUR_GRIS_F, COULEUR_GRIS_F, COULEUR_BLEU_C, "Taille",
        COULEUR_BLANC, COULEUR_GRIS_F, COULEUR_GRIS_F
    );


    for (u64 i = 0; i < adresses->taille; i++) {
        if (get_Vec_bool(etats, i)) {

            afficher(&console_principale, "%C%3u%C | %C%x%C | %C%m%C\n",
                COULEUR_BLEU_C, i, COULEUR_BLANC,
                COULEUR_BLEU_C, get_Vec_u64(adresses, i), COULEUR_BLANC,
                COULEUR_BLEU_C, get_Vec_u64(tailles, i), COULEUR_BLANC
            );
        }
    }


    afficher(&console_principale, "\n%C%i/%i%C blocs alloues :\n", COULEUR_BLEU_C, nb_allocations, nb_blocs, COULEUR_BLANC);


    for (u64 i = 0; i < adresses->taille; i++) {
        if (!get_Vec_bool(etats, i)) {

            afficher(&console_principale, "%C%3u%C | %C%x%C | %C%m%C\n",
                COULEUR_BLEU_C, i, COULEUR_BLANC,
                COULEUR_BLEU_C, get_Vec_u64(adresses, i), COULEUR_BLANC,
                COULEUR_BLEU_C, get_Vec_u64(tailles, i), COULEUR_BLANC
            );
        }
    }
}

void afficher_map_memoire(Parametres_memoire *parametres_memoire) {

    u64 pages_totales = 0;

    for (u64 i = 0; i < parametres_memoire->taille_map_memoire; i += parametres_memoire->taille_entree_memoire) {
        EFI_MEMORY_DESCRIPTOR *descripteur = (EFI_MEMORY_DESCRIPTOR *)((u8*)parametres_memoire->adresse_map_memoire + i);
        pages_totales += descripteur->NumberOfPages;
    }

    afficher(&console_principale, "Carte memoire :\n");
    afficher(&console_principale, "adresse : %C%x%C\n", COULEUR_BLEU_C, parametres_memoire->adresse_map_memoire, COULEUR_BLANC);
    afficher(&console_principale, "taille : %C%m (%u entrees de %m)%C\n",
        COULEUR_BLEU_C,
        parametres_memoire->taille_map_memoire,
        parametres_memoire->taille_map_memoire / parametres_memoire->taille_entree_memoire,
        parametres_memoire->taille_entree_memoire,
        COULEUR_BLANC
    );

    afficher(&console_principale, "taille totale %C%m (%u octets, %u pages)%C\n",
        COULEUR_BLEU_C,
        pages_totales * TAILLE_PAGE,
        pages_totales * TAILLE_PAGE,
        pages_totales,
        COULEUR_BLANC
    );

    afficher(&console_principale, "%C%F%3s%C%F | %C%F%18s%C%F | %C%F%18s%C%F | %C%F%4s%C%F | %C%F%14s%C%F | %C%F%21s%C%F\n",
        COULEUR_GRIS_F, COULEUR_BLEU_C, "Num",                  COULEUR_BLANC, COULEUR_GRIS_F,
        COULEUR_GRIS_F, COULEUR_BLEU_C, "Adresse physique",     COULEUR_BLANC, COULEUR_GRIS_F,
        COULEUR_GRIS_F, COULEUR_BLEU_C, "Adresse virtuelle",    COULEUR_BLANC, COULEUR_GRIS_F,
        COULEUR_GRIS_F, COULEUR_BLEU_C, "Type",                 COULEUR_BLANC, COULEUR_GRIS_F,
        COULEUR_GRIS_F, COULEUR_BLEU_C, "Taille (pages)",       COULEUR_BLANC,COULEUR_GRIS_F,
        COULEUR_GRIS_F, COULEUR_BLEU_C, "Attributs",            COULEUR_BLANC, COULEUR_GRIS_F
    );

    u64 j = 0;
    for (u64 i = 0; i < parametres_memoire->taille_map_memoire; i += parametres_memoire->taille_entree_memoire) {
        
        EFI_MEMORY_DESCRIPTOR *descripteur = (EFI_MEMORY_DESCRIPTOR *)((u8*)parametres_memoire->adresse_map_memoire + i);
        
        afficher(&console_principale, "%C%3u%C | %C%x%C | %C%x%C | %C%4u%C | %C%14u%C | %C%21u%C\n",
            COULEUR_BLEU_C,
            j,
            COULEUR_BLANC,
            COULEUR_BLEU_C,
            descripteur->PhysicalStart,
            COULEUR_BLANC,
            COULEUR_BLEU_C,
            descripteur->VirtualStart,
            COULEUR_BLANC,
            COULEUR_BLEU_C,
            descripteur->Type,
            COULEUR_BLANC,
            COULEUR_BLEU_C,
            descripteur->NumberOfPages,
            COULEUR_BLANC,
            COULEUR_BLEU_C,
            descripteur->Attribute,
            COULEUR_BLANC
        );
        j++;
    }
}



void *memmove(void *dest, const void *src, u64 n) {

    u8 *d = (u8 *)dest;
    const u8 *s = (const u8 *)src;

    if (d < s) {
        // Cas 1: Destination avant la source, copie vers l'avant
        for (u64 i = 0; i < n; i++) {
            d[i] = s[i];
        }
    } else {
        // Cas 2: Destination après la source (ou égale), copie vers l'arrière
        // On part de la fin pour éviter d'écraser la source avant de la lire
        for (u64 i = n; i > 0; i--) {
            d[i - 1] = s[i - 1];
        }
    }
    return dest;
}

// Copie de mémoire
void* memcpy(void* dest, const void* src, u64 n) {
    i8* d = (i8*)dest;
    const i8* s = (const i8*)src;
    for (u64 i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

// Initialisation de mémoire
void* memset(void* s, i32 c, u64 n) {

    volatile u8* p = (volatile u8*)s;
    for (u64 i = 0; i < n; i++) {
        p[i] = (u8)c;
    }
    return s;
}

