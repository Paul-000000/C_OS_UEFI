#ifndef INTERRUPTIONS_H
#define INTERRUPTIONS_H



#include "../types.h"



// Structure du TSS en 64 bits (doit être "packed")
typedef struct __attribute__((packed)) {
    u32 reserved0;
    u64 rsp0;      // Pile utilisée lors des passages de Ring 3 (User) à Ring 0 (Kernel)
    u64 rsp1;
    u64 rsp2;
    u64 reserved1;
    u64 ist1;      // <-- Pile de secours 1 (IST 1)
    u64 ist2;      // <-- Pile de secours 2 (IST 2)
    u64 ist3;
    u64 ist4;
    u64 ist5;
    u64 ist6;
    u64 ist7;
    u64 reserved2;
    u16 reserved3;
    u16 iopb_offset;
} TSS;

// Structure d'une entrée TSS dans la GDT (Spécifique au 64 bits) - 16 octets
typedef struct __attribute__((packed)) {
    u16 limit_low;
    u16 base_low;
    u8  base_middle;
    u8  access;
    u8  granularity;
    u8  base_high;
    u32 base_upper;
    u32 reserved;
} TSS_Descriptor;

// Structure d'une entrée classique de la GDT (Code/Data) - 8 octets
typedef struct __attribute__((packed)) {
    u16 limit_low;
    u16 base_low;
    u8  base_middle;
    u8  access;
    u8  granularity;
    u8  base_high;
} GDT_Entry;

typedef struct __attribute__((packed)) {
    u16 limit;
    u64 base;
} GDT_Pointer;

typedef struct __attribute__((packed)) {
    GDT_Entry null;        // 0x00
    GDT_Entry code;        // 0x08
    GDT_Entry data;        // 0x10
    TSS_Descriptor tss;    // 0x18
} GDT_Table;

typedef struct __attribute__((packed)) {
    u16 offset_low;
    u16 selector;
    u8  ist;
    u8  type_attributes;
    u16 offset_mid;
    u32 offset_high;
    u32 reserved;
} IDT_Entry;

typedef struct __attribute__((packed)) {
    u16 limit;
    u64 base;
} IDT_Pointer;

typedef void (*Handler_Fonction)(u64);



extern GDT_Table gdt;



static inline void outb(u16 port, u8 val) {
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline u8 inb(u16 port) {
    u8 val;
    __asm__ __volatile__("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void hlt() {

    __asm__ __volatile__("hlt");
}

static inline void configurer_interruptions(bool activer) {

    if (activer) {
        __asm__ __volatile__("sti" : : : "memory");
    } else {
        __asm__ __volatile__("cli" : : : "memory");
    }

}

static inline u64 sauvegarder_et_desactiver_interruptions() {

    u64 rflags;
    
    // pushfq place le registre de drapeaux (RFLAGS) sur la pile
    // pop %0 récupère cette valeur dans la variable 'rflags'
    // cli coupe les interruptions
    __asm__ __volatile__(
        "pushfq\n\t"
        "pop %0\n\t"
        "cli"
        : "=r"(rflags)  // Sortie : la variable rflags
        : 
        : "memory"      // Indique au compilateur de ne pas réordonner le code autour
    );
    
    return rflags;
}

static inline void restaurer_interruptions(u64 rflags_precedents) {
    // Le bit 9 du registre RFLAGS est l'IF (Interrupt Flag).
    // S'il était à 1, cela signifie que les interruptions étaient activées.
    if (rflags_precedents & (1 << 9)) {
        __asm__ __volatile__(
            "sti" 
            : 
            : 
            : "memory"
        );
    }
}

__attribute__((noreturn)) void attente_infinie();

__attribute__((noreturn)) void erreur_fatale(i64 code, const char *message);

void init_interruptions();

void initialiser_entree_interuption(u8 num_entree, u64 fonction, u8 attributes, u8 ist);



#endif