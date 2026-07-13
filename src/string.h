#ifndef STRING_H
#define STRING_H

#include "types.h"
#include "vecteur.h"



DEFINIR_VECTEUR_TYPE(char, Str)



Str *Str_vide();

Str *Str_creer(char *str);

Str *Str_dupliquer(Str s);

void Str_liberer(Str *s);

bool Str_egaux(Str *s1, Str *s2);

bool Str_ajouter_char(Str *s, char c);

bool Str_modifier_char(Str *s, u64 index, char c);

bool Str_supprimer(Str *s, u64 index);

bool Str_concaterner(Str *s1, Str *s2);

void Str_vider(Str *s);



u64 strlen(const char* str);

char *strncpy(char *dest, const char *src, u64 n);

i32 strcmp(const char* s1, const char* s2);

#endif