#include "../types.h"
#include "../interruptions/interruptions.h"
#include "../console.h"
#include "clavier.h"
#include "io.h"



char buffer_caracteres[TAILLE_BUFFER_CARACTERES];
u8 index_ecriture = 0;
u8 index_lecture = 0;
u8 nb_elements = 0;



u8 taille_buffer() {

    return nb_elements;
}

void ajouter_caractere(char c) {

    if (nb_elements < TAILLE_BUFFER_CARACTERES) {
        buffer_caracteres[index_ecriture] = c;
        index_ecriture = (index_ecriture + 1) % TAILLE_BUFFER_CARACTERES;
        nb_elements++;
    }
}

char recuperer_caractere() {

    while (nb_elements == 0) {
        hlt();
    }

    char c = buffer_caracteres[index_lecture];
    index_lecture = (index_lecture + 1) % TAILLE_BUFFER_CARACTERES;
    nb_elements--;

    return c;
}


const char table_azerty[] = {

    /*0  */ UNDEF   ,ECHAP  ,'&'       ,é         ,'"'       ,'\''  ,'('    ,'-'    ,è      ,'_' ,
    /*10 */ ç       ,à      ,')'       ,'='       ,'\b'      ,'\t'  ,'a'    ,'z'    ,'e'    ,'r' ,
    /*20 */ 't'     ,'y'    ,'u'       ,'i'       ,'o'       ,'p'   ,'^'    ,'$'    ,'\n'   ,CTRLG,
    /*30 */ 'q'     ,'s'    ,'d'       ,'f'       ,'g'       ,'h'   ,'j'    ,'k'    ,'l'    ,'m' ,
    /*40 */ '%'     ,²      ,SHIFTG    ,'*'       ,'w'       ,'x'   ,'c'    ,'v'    ,'b'    ,'n'  ,
    /*50 */ ','     ,';'    ,':'       ,'!'       ,SHIFTD    ,'*'   ,ALT    ,' '    ,MAJ    ,F1   ,
    /*60 */ F2      ,F3     ,F4        ,F5        ,F6        ,F7    ,F8     ,F9     ,F10    ,NUMLK,
    /*70 */ UNDEF   ,'7'    ,'8'       ,'9'       ,'-'       ,'4'   ,'5'    ,'6'    ,'+'    ,'1' ,
    /*80 */ '2'     ,'3'    ,'0'       ,'.'       ,UNDEF     ,UNDEF ,UNDEF  ,F11    ,F12    ,UNDEF ,
    /*90 */ UNDEF   ,UNDEF  ,UNDEF     ,UNDEF     ,UNDEF     ,UNDEF ,UNDEF  ,UNDEF  ,UNDEF  ,UNDEF ,
    /*100*/ UNDEF   ,UNDEF  ,UNDEF     ,UNDEF     ,UNDEF     ,UNDEF ,UNDEF  ,UNDEF  ,UNDEF  ,UNDEF ,
    /*110*/ UNDEF   ,UNDEF  ,UNDEF     ,UNDEF     ,UNDEF     ,UNDEF ,UNDEF  ,UNDEF  ,UNDEF  ,UNDEF ,
    /*120*/ UNDEF   ,UNDEF  ,UNDEF     ,UNDEF     ,UNDEF     ,UNDEF ,UNDEF  ,UNDEF
};

static bool est_etendu = false;



char scancode_etendu(u8 scancode) {

    switch (scancode) {

        case 83:
            return INSERT;

        case 42:
        case 55:
            return IMPR_ECR;
        
        case 71:
            return FIN;        
        
        case 91:
            return WINDOWS;   

        case 56:
            return ALTGR;

        case 29:
            return CTRLD;

        case 75:
            return FLECHE_G;

        case 72:
            return FLECHE_H;
        
        case 77:
            return FLECHE_D;

        case 80:
            return FLECHE_B;

        default:
            return UNDEF;
    }
}

void gestion_clavier(u64 code) {

    u8 scancode = inb(0x60);

    if (scancode == 0xE0) {
        est_etendu = true;
        outb(0x20, 0x20); // EOI
        return;           // On attend le prochain octet
    }

    if (est_etendu) {

        if (!(scancode & 0x80)) {

            ajouter_caractere(scancode_etendu(scancode));
        }
        
        est_etendu = false; // On reset l'état

    } else {
        
        if (!(scancode & 0x80) && scancode < sizeof(table_azerty)) { // filtre pour enlever le relachement de touche

            ajouter_caractere(table_azerty[scancode]);
        }
    }

    outb(0x20, 0x20); // EOI
}

void afficher_touches() {

    placer_curseur(&console_informations, 2, 0);

    Ligne_caracteres *ligne = get_Lignes_caracteres(console_informations.lignes, 2);
    vider_Ligne_caracteres(ligne);

    effacer_ligne(&console_informations, 2);

    afficher(
        &console_informations, "%F%C[Buffer clavier]%F%C taille: %C%i/%i%C: %F%C[",
        COULEUR_BLEU,
        COULEUR_BLANC,
        COULEUR_BLANC,
        COULEUR_GRIS_F,
        COULEUR_BLEU_C,
        (i64)nb_elements,
        (i64)TAILLE_BUFFER_CARACTERES,
        COULEUR_GRIS_F,
        COULEUR_BLEU,
        COULEUR_BLANC
    );

    u8 j = index_lecture;
    for (u8 i = 0; i < nb_elements; i++) {
        char caractere = caractere_affichable(buffer_caracteres[j]) ? buffer_caracteres[j] : '_';
        afficher(&console_informations, "%c", caractere);
        j = (j + 1) % TAILLE_BUFFER_CARACTERES;
    }

    for (u8 i = 0; i < (TAILLE_BUFFER_CARACTERES - nb_elements); i++) {
        afficher(&console_informations, " ", caractere);
    }

    afficher(&console_informations, "]%F%C",COULEUR_BLANC ,COULEUR_GRIS_F);

}