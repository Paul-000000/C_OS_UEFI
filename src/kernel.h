#ifndef KERNEL_H
#define KERNEL_H



#define ADRESSE_KERNEL 0x100000

#include "types.h"



void info();



typedef struct __attribute__((packed)) {
    void* adresse_buffer_image;
    u64 taille_buffer_image;
    u32 resolution_horizontale;
    u32 resolution_verticale;
    u32 resolution_horizontale_reele;

} Parametres_video;

typedef struct __attribute__((packed)) {
    void* adresse_map_memoire;
    u64 taille_map_memoire;
    u64 taille_entree_memoire;

} Parametres_memoire;

typedef struct __attribute__((packed)) {
    u64 taille_kernel;
    u64 pages_allouees;

} Parametres_kernel;

typedef struct __attribute__((packed)) {
    Parametres_video video;
    Parametres_memoire memoire;
    Parametres_kernel kernel;

} Informations_Boot;

extern Informations_Boot *informations_boot;



#endif