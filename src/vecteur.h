#ifndef VECTEUR_H
#define VECTEUR_H

#include "types.h"
#include "memoire/memoire.h"



/*
Type_vecteur* creer_Type_vecteur(u64 capacite_initiale)

bool ajouter_Type_vecteur(Type_vecteur *vecteur, Type_element element)

Type_element get_Type_vecteur(Type_vecteur *vecteur, u64 index)

bool supprimer_Type_vecteur(Type_vecteur *vecteur, u64 index)

void vider_Type_vecteur(Type_vecteur *vecteur)

void liberer_Type_vecteur(Type_vecteur *vecteur)
*/

#define DEFINIR_VECTEUR_TYPE(Type_element, Type_vecteur) \
    typedef struct { \
        Type_element *elements; \
        u64 taille; \
        u64 capacite; \
    } Type_vecteur; \
    \
    __attribute__((unused)) static inline Type_vecteur *creer_##Type_vecteur(u64 capacite_initiale) { \
        \
        if (capacite_initiale == 0) return Null; \
        Type_vecteur *vecteur = (Type_vecteur*)malloc(sizeof(Type_vecteur)); \
        if (vecteur == Null) return Null; \
        \
        vecteur->elements = (Type_element*)malloc(sizeof(Type_element) * capacite_initiale); \
        if (vecteur->elements == Null) { \
            free(vecteur); \
            return Null; \
        } \
        vecteur->taille = 0; \
        vecteur->capacite = capacite_initiale; \
        return vecteur; \
    } \
    \
    __attribute__((unused)) static inline bool ajouter_##Type_vecteur(Type_vecteur *vecteur, Type_element element) { \
        \
        if (vecteur->taille >= vecteur->capacite) { \
            u64 nouvelle_capacite = (vecteur->capacite == 0) ? 4 : vecteur->capacite * 2; \
            Type_element *nouveaux_elements = (Type_element*)malloc(sizeof(Type_element) * nouvelle_capacite); \
            if (nouveaux_elements == Null) { \
                return false; \
            } \
            \
            memmove(nouveaux_elements, vecteur->elements, sizeof(Type_element) * vecteur->taille); \
            free(vecteur->elements); \
            vecteur->elements = nouveaux_elements; \
            vecteur->capacite = nouvelle_capacite; \
        } \
        \
        vecteur->elements[vecteur->taille] = element; \
        vecteur->taille++; \
        return true; \
    } \
    \
    __attribute__((unused)) static inline Type_element get_##Type_vecteur(Type_vecteur *vecteur, u64 index) { \
        if (index >= vecteur->taille) { \
            /* Retourne une structure vide/zero si hors limite */ \
            Type_element vide; \
            memset(&vide, 0, sizeof(Type_element)); \
            return vide; \
        } \
        return vecteur->elements[index]; \
    } \
    \
    __attribute__((unused)) static inline bool supprimer_##Type_vecteur(Type_vecteur *vecteur, u64 index) { \
        if (index >= vecteur->taille) return false; \
        \
        memmove(&vecteur->elements[index], \
                &vecteur->elements[index + 1], \
                (vecteur->taille - index - 1) * sizeof(Type_element)); \
        \
        vecteur->taille--; \
        return true; \
    } \
    \
    __attribute__((unused)) static inline void vider_##Type_vecteur(Type_vecteur *vecteur) { \
        vecteur->taille = 0; \
    } \
    \
    __attribute__((unused)) static inline void liberer_##Type_vecteur(Type_vecteur *vecteur) { \
        if (vecteur->elements != Null) free(vecteur->elements); \
        free(vecteur); \
    }



DEFINIR_VECTEUR_TYPE(u64, Vec_u64)
DEFINIR_VECTEUR_TYPE(bool, Vec_bool)



#endif