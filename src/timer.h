#ifndef TIMER_H
#define TIMER_H



#include "types.h"
#include "interruptions/interruptions.h"

#define FREQUENCE_INTERRUPTION 60



extern volatile u64 ticks_interruption;



u64 secondes_ecoulees();

void init_timer(u32 frequence_hz);

void gestion_timer(u64 code);

void afficher_temps();



#endif