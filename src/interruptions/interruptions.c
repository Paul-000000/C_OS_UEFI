#include "interruptions.h"
#include "../memoire/memoire.h"
#include "../types.h"
#include "../string.h"
#include "../console.h"
#include "../io/clavier.h"
#include "../timer.h"
#include "../processus/processus.h"
#include "../io/io.h"
#include "../kernel.h"



#define SEGMENT_CODE_KERNEL 0x08 
#define ATTRIBUT_PRESENT_RING_0 0x8E
#define COMMANDE_PIC_MASTER 0x20
#define DONNEES_PIC_MASTER 0x21
#define COMMANDE_PIC_SLAVE 0xA0
#define DONNEES_PIC_SLAVE 0xA1



u8 pile_secours_pf[4096] __attribute__((aligned(16)));
TSS tss_noyau;
GDT_Table gdt;
GDT_Pointer gdtr;
IDT_Pointer idtr;
IDT_Entry idt[256];
extern u64 table_adresses_isr[256];
Handler_Fonction table_redirection[256];

u64 __stack_chk_guard = 0xDEADC0DEDEADC0DE;



void init_gdt_et_tss() {
    
    // A. Configuration du TSS
    memset(&tss_noyau, 0, sizeof(TSS));
    tss_noyau.ist1 = (u64)pile_secours_pf + 4096; // La pile IST grandit vers le bas
    tss_noyau.iopb_offset = sizeof(TSS);

    // B. Configuration de la GDT
    memset(&gdt, 0, sizeof(gdt));

    // Code Kernel (Index 1 -> Selecteur 0x08)
    gdt.code.limit_low = 0xFFFF;
    gdt.code.access = 0x9A;      // Présent, Ring 0, Executable, Lisible
    gdt.code.granularity = 0xAF; // Flag 64-bit (Long Mode) + limite haute (0xF)

    // Data Kernel (Index 2 -> Selecteur 0x10)
    gdt.data.limit_low = 0xFFFF;
    gdt.data.access = 0x92;      // Présent, Ring 0, Inscriptible
    gdt.data.granularity = 0xCF; // 32-bit (Par défaut pour les données) + limite haute (0xF)

    // Descripteur TSS (Index 3 -> Selecteur 0x18)
    u64 tss_base = (u64)&tss_noyau;
    gdt.tss.limit_low = sizeof(TSS) - 1;

    gdt.tss.base_low = tss_base & 0xFFFF;
    gdt.tss.base_middle = (tss_base >> 16) & 0xFF;
    gdt.tss.base_high = (tss_base >> 24) & 0xFF;
    gdt.tss.base_upper = (tss_base >> 32) & 0xFFFFFFFF;
    gdt.tss.access = 0x89;       // Présent, Ring 0, Type: 64-bit TSS (Available)
    gdt.tss.granularity = 0x00;

    // C. Chargement matériel de la GDT
    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base = (u64)&gdt;

    __asm__ __volatile__(
        "lgdt %0\n\t"
        // On recharge le registre CS (Code Segment) avec un 'Far Return' (lretq)
        "push $0x08\n\t"             
        "lea 1f(%%rip), %%rax\n\t"
        "push %%rax\n\t"
        "lretq\n\t"                  
        "1:\n\t"
        // On recharge les registres de données
        "mov $0x10, %%ax\n\t"        
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        "mov %%ax, %%ss\n\t"
        : : "m"(gdtr) : "rax", "memory"
    );

    // D. Chargement du Task Register (TR) avec le sélecteur du TSS (0x18)
    __asm__ __volatile__("ltr %%ax" : : "a"((u16)0x18));
}

__attribute__((noreturn)) void attente_infinie() {

    while (true) {
        hlt();
    }
}

__attribute__((noreturn)) void erreur_fatale(i64 code, const char *message) {

    configurer_interruptions(false);

    init_affichage(&informations_boot->video, false);
    colorer_ecran(COULEUR_NOIR);
    
    afficher_debug("Erreur fatale: %s\nCode d'interruption: %i\nProcessus %u : %S\n", 
        message,
        code,
        pid_actif,
        nom_processus(pid_actif)
    );

    attente_infinie();
}

void erreur(u64 code, const char *message) {

    if (pid_actif == 0) {
        erreur_fatale(code, message);
    }

    afficher(&console_principale, "%CErreur: %s\nCode d'interruption: %i\nArret du processus %u : %S%C\n",
        COULEUR_ROUGE,
        message,
        code,
        pid_actif,
        nom_processus(pid_actif),
        COULEUR_BLANC
    );

    tuer_processus(pid_actif);

}



void __stack_chk_fail(void) { // en cas de débordement memoire

    erreur(-1,"Debordement memoire");
}

void gestionnaire_division_par_zero(u64 code) {

    erreur(code, "Division par 0");
}

void gestionnaire_page_fault(u64 code) {

    erreur(code, "Page fault");
}

void gestionnaire_double_page_fault(u64 code) {

    erreur_fatale(code, "Double page fault");
}

void gestionnaire_instruction_invalide(u64 code) {

    erreur_fatale(code, "Instruction invalide");
}

void gestionnaire_adresse_memoire_invalide(u64 code) {

    erreur_fatale(code, "Adresse memoire invalide (General Protection Fault)");
}

void gestionnaire_erreur_inconnue(u64 code) {

    erreur_fatale(code, "Erreur inconnue");
}

void gestionnaire_irq7_fantome(u64 code) {
    (void)code; // On ignore simplement l'interruption
}


void initialiser_entree_interuption(u8 num_entree, u64 adresse, u8 attributes, u8 ist) {
    
    IDT_Entry *descriptor = &idt[num_entree];

    descriptor->offset_low  = (u16)(adresse & 0xFFFF);
    descriptor->offset_mid  = (u16)((adresse >> 16) & 0xFFFF);
    descriptor->offset_high = (u32)((adresse >> 32) & 0xFFFFFFFF);

    descriptor->selector    = SEGMENT_CODE_KERNEL; // /!\ ATTENTION: UEFI utilise généralement 0x38 pour le Segment de Code Kernel GDT, à vérifier si ça crash
    descriptor->ist         = ist;
    descriptor->type_attributes = attributes;
    descriptor->reserved    = 0;
}

void init_pic() {

    //(2 puces) PIC 8259 (Programmable Interrupt Controller)
    // interruptions matérielles 0 à 7
    // Master PIC

    outb(COMMANDE_PIC_MASTER, 0x11); // ICW1: Initialisation + besoin de ICW4
    outb(DONNEES_PIC_MASTER, 0x20); // ICW2: déccalage des codes d'interruptions de 32 (0x20)
    outb(DONNEES_PIC_MASTER, 0x04); // ICW3: Cascade (Master)
    outb(DONNEES_PIC_MASTER, 0x01); // ICW4: Mode 8086

    // Slave PIC
    outb(COMMANDE_PIC_SLAVE, 0x11);
    outb(DONNEES_PIC_SLAVE, 0x28); // ICW2: déccalage des codes d'interruptions de 40 (0x28)
    outb(DONNEES_PIC_SLAVE, 0x02);
    outb(DONNEES_PIC_SLAVE, 0x01); // ICW4: Mode 8086

    // Masques (activer toutes les IRQs)
    outb(DONNEES_PIC_MASTER, 0xFC);
    outb(DONNEES_PIC_SLAVE, 0xFF);
}

void lier_gestionnaire(u8 code_interruption, Handler_Fonction fonction) {

    table_redirection[code_interruption] = fonction;

}

void init_interruptions() {
    
    configurer_interruptions(false);
    init_gdt_et_tss();

    idtr.limit = (sizeof(IDT_Entry) * 256) - 1;
    idtr.base  = (u64)&idt;

    for (u16 i = 0; i < 256; i++) {
        initialiser_entree_interuption(i, table_adresses_isr[i], ATTRIBUT_PRESENT_RING_0, 0);
        lier_gestionnaire(i, gestionnaire_erreur_inconnue);
    }


    // Assignation de la pile IST 1 pour les fautes critiques
    initialiser_entree_interuption(8, table_adresses_isr[8], ATTRIBUT_PRESENT_RING_0, 1);  // Double Fault
    initialiser_entree_interuption(14, table_adresses_isr[14], ATTRIBUT_PRESENT_RING_0, 1); // Page Fault

    init_timer(FREQUENCE_INTERRUPTION);

    init_pic();

    lier_gestionnaire(0, gestionnaire_division_par_zero);
    lier_gestionnaire(32, gestion_timer);
    lier_gestionnaire(33, gestion_clavier);
    lier_gestionnaire(39, gestionnaire_irq7_fantome);
    lier_gestionnaire(14, gestionnaire_page_fault); 
    lier_gestionnaire(6, gestionnaire_instruction_invalide);
    lier_gestionnaire(8, gestionnaire_double_page_fault); 
    lier_gestionnaire(13, gestionnaire_adresse_memoire_invalide);

    __asm__ __volatile__("lidt %0" : : "m"(idtr)); 
    configurer_interruptions(true);
}
