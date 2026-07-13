#include "types.h"
#include "string.h"
#include "memoire/memoire.h"



Str *Str_vide() {

    Str *s = creer_Str(8);
    if (s == Null) {
        return Null;
    }

    s->taille = 0;
    return s;
}

Str *Str_creer(char *str) {

    u64 taille = strlen(str);

    Str *s = creer_Str(taille);
    if (s == Null) {
        return Null;
    }
    
    u64 i = 0;
    while (str[i] != '\0') {
        s->elements[i] = str[i];
        i++;
    }

    s->taille = taille;
    return s;
}

Str *Str_dupliquer(Str s) {

    Str *s2 = creer_Str(s.taille);
    if (s2 == Null) {
        return Null;
    }

    s2->taille = s.taille;
    s2->capacite = s.taille;
    memmove(s2->elements, s.elements, sizeof(char) * s.taille);

    return s2;
}

void Str_liberer(Str *s) {

    if (s == Null) return;

    liberer_Str(s);
}

bool Str_egaux(Str *s1, Str *s2) {

    if (s1 == Null || s2 == Null) {
        return false;
    }

    if (s1->taille != s2->taille) {
        return false;
    }

    u64 i = 0;

    while (i < s1->taille) {
        if (s1->elements[i] != s2->elements[i]) {
            return false;
        }
        i++;
    }

    return true;
}

bool Str_ajouter_char(Str *s, char c) {

    return ajouter_Str(s, c);
}

bool Str_modifier_char(Str *s, u64 index, char c) {

    if (index >= s->taille) {
        return false;
    }

    s->elements[index] = c;
    return true;
}

bool Str_supprimer(Str *s, u64 index) {

    return supprimer_Str(s, index);
}

bool Str_concaterner(Str *s1, Str *s2) {
    u64 taille_totale = s1->taille + s2->taille;

    if (taille_totale > s1->capacite) {
        char *nouveau_elements = (char*)malloc(sizeof(char) * taille_totale);
        if (nouveau_elements == Null) {
            return false;
        }

        // 1. On copie d'abord depuis l'ancien tableau
        memmove(nouveau_elements, s1->elements, sizeof(char) * s1->taille);
        // 2. On libère l'ancien tableau ensuite
        free(s1->elements);
        
        s1->elements = nouveau_elements;
        s1->capacite = taille_totale;
    }

    memmove(&s1->elements[s1->taille], s2->elements, sizeof(char) * s2->taille);
    s1->taille = taille_totale;

    return true;
}

void Str_vider(Str *s) {

    s->taille = 0;
}



u64 strlen(const char* str) {

    u64 len = 0;

    while (str[len] != '\0') {
        len++;
    }

    return len;
}

char *strncpy(char *dest, const char *src, u64 n) {

    u64 i;

    // Copie les caractères de src vers dest tant qu'on n'a pas atteint n
    // et qu'on n'est pas tombé sur la fin de la chaîne source (\0)
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }

    // Règle stricte de strncpy : si src est plus courte que n,
    // on remplit le reste de l'espace alloué avec des \0
    for (; i < n; i++) {
        dest[i] = '\0';
    }

    return dest;
}

i32 strcmp(const char* s1, const char* s2) {

    // On avance tant que les caractères sont identiques 
    // et qu'on n'est pas arrivé à la fin de la chaîne (le '\0')
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }

    // On retourne la différence entre les deux derniers caractères trouvés
    // On caste en unsigned char pour éviter les problèmes de signe
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}