#include <stdarg.h>
#include "../types.h"
#include "../console.h"
#include "../interruptions/interruptions.h"
#include "affichage.h"



#define PAS_TAILLE_MAX -1

typedef enum {
    TARGET_DEBUG,
    TARGET_CONSOLE,
    TARGET_STRING,
    TARGET_CHAR
} TargetType;

typedef struct {
    TargetType type;
    void *ptr; // Pointeur opaque vers Console* ou Str*
    u64 taille;
    i64 taille_max;

} FormatTarget;

// Fonction unique pour écrire un caractère
static inline bool emit_char(FormatTarget *target, char c) {

    switch (target->type) {

        case TARGET_DEBUG:
            ajouter_caractere_debug(c);
            return false;

        case TARGET_CONSOLE:
            afficher_caractere((Console *)target->ptr, c);
            return false;

        case TARGET_STRING:
            Str_ajouter_char((Str *)target->ptr, c);
            return false;

        default:

            if (target->taille_max != PAS_TAILLE_MAX && target->taille >= target->taille_max) {
                return true;
            }
            
            ((char *)(target->ptr))[target->taille] = c;
            target->taille++;

            return false;
    }
}

u64 abs(i64 nombre) {
    if (nombre < 0) {
        return (u64)(-(nombre + 1)) + 1;
    }
    return (u64)nombre;
}



bool format_octets(FormatTarget *target, const char *s, u32 len, u64 largeur) {
    
    u32 i = 0;
    while (s[i] != '\0' && i < len) {
        
        if (emit_char(target, s[i])) {
            return true;
        }
        i++;
    }
    if (i < largeur) {

        u64 nb_espaces = largeur - i;
        
        for (u64 j = 0; j < nb_espaces; j++) {
            if (emit_char(target, ' ')) {
                return true;
            }
        }
    }

    return false;
}

bool format_texte(FormatTarget *target, const char *s, u64 largeur_minimale) {
    
    u32 i = 0;
    while (s[i] != '\0') {
        
        if (emit_char(target, s[i])) {
            return true;
        }
        i++;
    }
    if (i < largeur_minimale) {

        u64 nb_espaces = largeur_minimale - i;

        for (u64 j = 0; j < nb_espaces; j++) {
            if (emit_char(target, ' ')) {
                return true;
            }
        }
    }

    return false;
}

bool format_adresse(FormatTarget *target, void *pointeur) {
    
    u64 adresse = (u64)pointeur;
    const char *hex_chars = "0123456789ABCDEF";
    char tampon[16];
    u8 i = 0;

    if (adresse == 0) {
        return format_octets(target, "Null", 4, 18);
    }

    while (adresse > 0 && i < 16) {
        tampon[i] = hex_chars[(adresse % 16)];
        i++;
        adresse /= 16;
    }

    if (format_octets(target, "0x", 2, 2)) {
        return true;
    }

    u8 zeros_completion = 16 - i;

    for (i8 j = 0; j < zeros_completion; j++) {
        if (emit_char(target, '0')) {
            return true;
        }
    }

    for (i8 j = (i8)i - 1; j >= 0; j--) {
        if (emit_char(target, tampon[j])) {
            return true;
        }
    }

    return false;
}

bool format_non_signe_formate(FormatTarget *target, u64 nombre, u32 largeur, Padding_texte padding) {
    
    char tampon[21]; 
    u8 i = 0;

    if (nombre == 0) {
        tampon[i] = '0';
        i++;

    } else {
        while (nombre > 0 && i < 21) {
            tampon[i] = '0' + (nombre % 10);
            i++;
            nombre /= 10;
        }
    }

    u32 taille_totale = i;

    if (padding == ESPACES) {
        while (taille_totale < largeur) {
            if (emit_char(target, ' ')) {
                return true;
            }
            taille_totale++;
        }
    }
    if (padding == ZEROS) {
        while (taille_totale < largeur) {
            if (emit_char(target, '0')) {
                return true;
            }
            taille_totale++;
        }
    }
    for (i8 j = (i8)i - 1; j >= 0; j--) {
        if (emit_char(target, tampon[j])) {
            return true;
        }
    }

    return false;
}

bool format_nombre_formate(FormatTarget *target, i64 nombre, u32 largeur, Padding_texte padding) {
    
    char tampon[21]; 
    u8 i = 0;
    bool negatif = nombre < 0;

    if (nombre == 0) {
        tampon[i] = '0';
        i++;
    } else {    
        u64 valeur_absolue = abs(nombre);
        while (valeur_absolue > 0 && i < 21) {
            tampon[i] = '0' + (valeur_absolue % 10);
            i++;
            valeur_absolue /= 10;
        }
        if (negatif) {
            tampon[i] = '-';
            i++;
        }
    }

    u32 taille_totale = i;

    if (padding == ESPACES) {
        while (taille_totale < largeur) {
            if (emit_char(target, ' ')) {
                return true;
            }
            taille_totale++;
        }
    }
    if (negatif) {
        if (emit_char(target, '-')) {
            return true;
        }
        i--;
    }
    if (padding == ZEROS) {
        while (taille_totale < largeur) {
            if (emit_char(target, '0')) {
                return true;
            }
            taille_totale++;
        }
    }
    for (i8 j = (i8)i - 1; j >= 0; j--) {
        if (emit_char(target, tampon[j])) {
            return true;
        }
    }

    return false;
}

bool format_taille(FormatTarget *target, u64 taille) {

    u64 unite;
    const char *tampon_unite;

    if (taille < 1000) {
        unite = taille;
        tampon_unite = " octets";

    } else if (taille < 1000 * 1000) {
        unite = taille / 1000;
        tampon_unite = " Ko";

    } else if (taille < 1000 * 1000 * 1000) {
        unite = taille / (1000 * 1000);
        tampon_unite = " Mo";

    } else {
        unite = taille / (1000 * 1000 * 1000);
        tampon_unite = " Go";
    }

    if (format_non_signe_formate(target, unite, 0, ESPACES)) {
        return true;
    }
    
    return format_texte(target, tampon_unite, 0);
}



u64 format_core(FormatTarget *target, const char *format, va_list args) {

    bool taille_max = false;

    while (*format != '\0' && !taille_max) {

        if (*format == '%') {    
            format++;
            
            Padding_texte padding = ESPACES;
            u32 largeur = 0;

            if (*format == '0') {
                padding = ZEROS;
                format++;
            }

            while (*format >= '0' && *format <= '9') {
                largeur = largeur * 10 + (*format - '0');
                format++;
            }
            
            switch (*format) {

                case 'i':
                case 'd':
                    taille_max = format_nombre_formate(target, va_arg(args, i64), largeur, padding);
                    break;
                
                case 'u':
                    taille_max = format_non_signe_formate(target, va_arg(args, u64), largeur, padding);
                    break;
                    
                case 'm':
                    taille_max = format_taille(target, va_arg(args, u64));
                    break;

                case 's':
                    taille_max = format_texte(target, va_arg(args, char*), largeur);
                    break;

                case 'S': {
                    Str *s = va_arg(args, Str*);
                    taille_max = format_octets(target, s->elements, s->taille, largeur);
                    break;
                }

                case 'c':
                    taille_max = emit_char(target, (char)va_arg(args, i32));
                    break;

                case 'x':
                    taille_max = format_adresse(target, va_arg(args, void *));
                    break;

                // Cas particuliers liés uniquement à la Console
                case 'C':
                    if (target->type == TARGET_CONSOLE) {
                        ((Console *)target->ptr)->couleur_texte.couleur = (u32)va_arg(args, i32);
                    } else {
                        taille_max = emit_char(target, *format); // Comportement d'origine de chaine()
                    }
                    break;

                case 'F':
                    if (target->type == TARGET_CONSOLE) {
                        ((Console *)target->ptr)->couleur_fond.couleur = (u32)va_arg(args, i32);
                    } else {
                        taille_max = emit_char(target, *format); // Comportement d'origine de chaine()
                    }
                    break;

                default:
                    taille_max = emit_char(target, *format);
                    break;
            }
            
        } else {
            taille_max = emit_char(target, *format);
        }
        format++;
    }

    return target->taille;
}

void afficher(Console *console, const char *format, ...) {
    
    u64 etat = sauvegarder_et_desactiver_interruptions();

    FormatTarget target = { TARGET_CONSOLE, console, 0, PAS_TAILLE_MAX };

    va_list args;
    va_start(args, format);
    format_core(&target, format, args);
    va_end(args);

    if (curseur_visible(console)) {
        placer_caractere(console, console->ligne_curseur, console->colonne_curseur, '_');
    }

    mise_a_jour_affichage();
    restaurer_interruptions(etat);
}

void afficher_debug(const char *format, ...) {
    
    FormatTarget target = { TARGET_DEBUG, Null, 0, PAS_TAILLE_MAX };

    va_list args;
    va_start(args, format);
    format_core(&target, format, args);
    va_end(args);

}


Str *Str_formatee(const char *format, ...) {

    Str *str = Str_vide();
    if (str == Null) {
        return Null;
    }

    FormatTarget target = { TARGET_STRING, str, 0, PAS_TAILLE_MAX };

    va_list args;
    va_start(args, format);
    format_core(&target, format, args);
    va_end(args);

    return str;
}

void char_formatee(char *s, u64 taille, const char *format, ...) {
    
    // Sécurité anti-underflow et pointeur nul
    if (taille == 0 || s == Null) {
        return;
    }

    FormatTarget target = { TARGET_CHAR, s, 0, (i64)(taille - 1) };

    va_list args;
    va_start(args, format);
    format_core(&target, format, args);
    va_end(args);

    s[target.taille] = '\0';
}