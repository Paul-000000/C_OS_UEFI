#ifndef IO_H
#define IO_H



#include "../console.h"



void afficher(Console *console,const char *format, ...);

Str *Str_formatee(const char *format, ...);

void char_formatee(char *s, u64 taille, const char *format, ...);

void afficher_debug(const char *format, ...);



#endif