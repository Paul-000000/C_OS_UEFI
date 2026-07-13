#include "uefi.h"
#include "../types.h"
#include "../font.h"
#include <stdarg.h>



#define TAILLE_BUFFER_MAP_MEMOIRE 32768
#define TAILLE_BUFFER_INFORMATIONS_FICHIER 256
#define TAILLE_PAGE 4096
#define DELAI_ERREUR 10
#define CODE_RETOUR_ERREUR 0x8000000000000001



typedef union Couleur {
    
    struct __attribute__((packed)) {
        u8 b,g,r,a;
    };

    u32 couleur;

} Couleur;

Informations_Boot infos_boot;
u8 map_memoire[TAILLE_BUFFER_MAP_MEMOIRE]; // Un tampon statique en mémoire pour stocker temporairement la carte mémoire (4 Ko suffisent largement)

u16 ligne_curseur = 0;
u16 colonne_curseur = 0;

const Couleur couleur_fond_console = { .b = 25, .g = 25, .r = 25 };
const Couleur couleur_texte_console = { .b = 0, .g = 0, .r = 255 };



void afficher(const char *format, ...);

void afficher_erreur_UEFI(EFI_SYSTEM_TABLE *table, void *image_handle, char *texte);

void colorer_ecran(Couleur c);



bool recuperer_informations_graphiques(EFI_SYSTEM_TABLE *table) {

    EFI_GUID id_protocole_graphique = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *protocole_graphique = 0;
    
    EFI_STATUS status = table->BootServices->LocateProtocol(&id_protocole_graphique, 0, (void**)&protocole_graphique);
    
    if (status != 0 || protocole_graphique == 0) {
        
        return false;
    }
        
    infos_boot.video.adresse_buffer_image          = (void*)protocole_graphique->Mode->FrameBufferBase;
    infos_boot.video.taille_buffer_image           = protocole_graphique->Mode->FrameBufferSize;
    infos_boot.video.resolution_horizontale        = protocole_graphique->Mode->Info->HorizontalResolution;
    infos_boot.video.resolution_verticale          = protocole_graphique->Mode->Info->VerticalResolution;

    u32 resolution_horizontale_reele = protocole_graphique->Mode->Info->PixelsPerScanLine;

    infos_boot.video.resolution_horizontale_reele  = resolution_horizontale_reele == 0 ? infos_boot.video.resolution_horizontale : resolution_horizontale_reele;
    
    return true;
}

bool recuperer_informations_memoire(EFI_SYSTEM_TABLE *table, u64 *mapkey) {

    u64 mapSize = sizeof(map_memoire);
    u64 descriptorSize = 0;
    u32 descriptorVersion = 0;

    EFI_STATUS status = table->BootServices->GetMemoryMap(
        &mapSize, 
        map_memoire, 
        mapkey, 
        &descriptorSize, 
        &descriptorVersion
    );

    if (status != 0) {
        return false;
    }

    infos_boot.memoire.adresse_map_memoire   = (void*)map_memoire;
    infos_boot.memoire.taille_map_memoire    = mapSize;
    infos_boot.memoire.taille_entree_memoire = descriptorSize;

    return true;
}

void charger_kernel(EFI_SYSTEM_TABLE *table, void *image_handle) {

    EFI_STATUS status;
    
    // Récupérer les informations sur l'image en cours
    EFI_GUID loaded_image_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_LOADED_IMAGE_PROTOCOL *loaded_image = 0;

    status = table->BootServices->HandleProtocol(image_handle, &loaded_image_guid, (void**)&loaded_image);
    if (status != 0 || loaded_image == 0) {
        afficher_erreur_UEFI(table, image_handle, "Impossible de trouver LoadedImage.");
    }

    // Ouvrir le système de fichiers UNIQUEMENT sur le périphérique de boot
    EFI_GUID fs_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs = 0;

    status = table->BootServices->HandleProtocol(loaded_image->DeviceHandle, &fs_guid, (void**)&fs);
    if (status != 0 || fs == 0) {
        afficher_erreur_UEFI(table, image_handle, "Impossible d'ouvrir le systeme de fichiers du boot.");
    }

    // Ouvrir le volume racine
    EFI_FILE_PROTOCOL *racine = 0;
    status = fs->OpenVolume(fs, &racine);
    if (status != 0 || racine == 0) {
        afficher_erreur_UEFI(table, image_handle, "Impossible d'ouvrir le volume racine.");
    }

    // Ouvrir le fichier dans le sous-dossier EFI\BOOT
    EFI_FILE_PROTOCOL *fichier_kernel = 0;
    status = racine->Open(racine, &fichier_kernel, L"EFI\\BOOT\\kernel.bin", EFI_FILE_MODE_READ, 0);
    if (status != 0 || fichier_kernel == 0) {
        afficher_erreur_UEFI(table, image_handle, "Fichier kernel.bin introuvable dans EFI\\BOOT.");
    }

    // Récupérer la taille du fichier
    static u8 info_buffer[TAILLE_BUFFER_INFORMATIONS_FICHIER];
    u64 info_size = sizeof(info_buffer);

    EFI_GUID guid = EFI_FILE_INFO_ID;
    status = fichier_kernel->GetInfo(fichier_kernel, &guid, &info_size, (void *)info_buffer);

    if (status != 0) {
        afficher_erreur_UEFI(table, image_handle, "Impossible de lire les infos du fichier (GetInfo a echoue)");
    }

    EFI_FILE_INFO *file_info = (EFI_FILE_INFO*)info_buffer;
    
    infos_boot.kernel.taille_kernel = file_info->FileSize;
    infos_boot.kernel.pages_allouees = (infos_boot.kernel.taille_kernel / TAILLE_PAGE) + 1;

    // Allouer la place nécessaire
    EFI_PHYSICAL_ADDRESS adresse_kernel = ADRESSE_KERNEL;
    status = table->BootServices->AllocatePages(AllocateAddress, EfiLoaderData, infos_boot.kernel.pages_allouees, &adresse_kernel);

    if (status != 0 || adresse_kernel != ADRESSE_KERNEL) {
        afficher_erreur_UEFI(table, image_handle, "Impossible d'allouer l'espace du kernel");
    }

    // Lire le fichier dans l'espace alloué
    u64 taille_kernel = infos_boot.kernel.taille_kernel;
    status = fichier_kernel->Read(fichier_kernel, &taille_kernel, (void*)ADRESSE_KERNEL);
    if (status != 0) {
        fichier_kernel->Close(fichier_kernel);
        afficher_erreur_UEFI(table, image_handle, "Erreur lors de la lecture du fichier kernel.bin.");
    }

    // Nettoyer et fermer les descripteurs de fichiers
    fichier_kernel->Close(fichier_kernel);
    racine->Close(racine);
}



i64 efi_main(void *image_handle, EFI_SYSTEM_TABLE *table) {
    
    u64 map_key;
    EFI_STATUS status;

    if (!recuperer_informations_graphiques(table)) {
        
        afficher_erreur_UEFI(table, image_handle, "problème de protocole graphique");
        return CODE_RETOUR_ERREUR;
    }

    colorer_ecran(couleur_fond_console);

    charger_kernel(table, image_handle);
    
    
    //afficher("Kernel charge en : %x\n", ADRESSE_KERNEL);


    if (!recuperer_informations_memoire(table, &map_key)) {
        
        afficher_erreur_UEFI(table, image_handle, "problème de map memoire");
        return CODE_RETOUR_ERREUR;
    }


    //afficher("Map memoire : %x\n", map_key);


    // quitter l'uefi et sauter dans le kernel
    status = table->BootServices->ExitBootServices(image_handle, map_key);

    if (status != 0) {
        afficher_erreur_UEFI(table, image_handle, "Echec ExitBootServices");
        return CODE_RETOUR_ERREUR;
    }


    //afficher("Saut vers le noyau\n");


    KernelStart demarrer_noyau = (KernelStart)ADRESSE_KERNEL;
    demarrer_noyau(&infos_boot);


    return 0;
}



bool caractere_affichable(char c) {

    return (c >= 32 && c <= 126);
}

bool caractere_controle(char c) {

    return (c < 32 || c == 127);
}

void caractere(u32 x, u32 y, char c, Couleur couleur, Couleur couleur_fond) {

    for (u8 i = 0; i < hauteur_police; i++) {
        for (u8 j = 0; j < largeur_police; j++) {
            
            Couleur couleur_texte_choisie = (police_8x16[(u8)c][i] & (1 << (largeur_police - j - 1))) ? couleur : couleur_fond ; 

            ((u32 *)infos_boot.video.adresse_buffer_image)[(y + i) * infos_boot.video.resolution_horizontale_reele + (x + j)] = couleur_texte_choisie.couleur;

        }
    }
    
}

void afficher_caractere(char c) {

    if (caractere_affichable(c)) {
        
        caractere(colonne_curseur * largeur_police, ligne_curseur * hauteur_police, c, couleur_texte_console, couleur_fond_console);
        colonne_curseur++;
    }

    if (caractere_controle(c) && c == '\n') {
        
        ligne_curseur++;
        colonne_curseur = 0;
    }
}

void colorer_ecran(Couleur c) {

    u32 *framebuffer = (u32 *)infos_boot.video.adresse_buffer_image;
    u64 total_pixels = infos_boot.video.resolution_horizontale_reele * infos_boot.video.resolution_verticale;

    for (u64 i = 0; i < total_pixels; i++) {
        framebuffer[i] = c.couleur;
    }

    ligne_curseur = 0;
    colonne_curseur = 0;
}

u64 abs(i64 nombre) {

	if (nombre < 0) {
        return (u64)(-(nombre + 1)) + 1;
	}

	return (u64)nombre;
}

void afficher_texte(const char *s) {
    
    u32 i = 0;

    while (s[i] != '\0') {
        
        afficher_caractere(s[i]);
        i++;
    }
}

void afficher_adresse(void *pointeur) {

	u64 adresse = (u64)pointeur;

	const char *hex_chars = "0123456789ABCDEF";
	char tampon[16];
	u8 i = 0;

	if (adresse == 0) {
		afficher("Null");
		return;
	}

	while (adresse > 0 && i < 16) {
		tampon[i] = hex_chars[(adresse % 16)];
		i++;
		adresse /= 16;
    }

	afficher("0x");

	u8 zeros_completion = 16 - i;

	for (i8 j = 0; j < zeros_completion; j++) {
		afficher_caractere('0');
	}

	for (i8 j = (i8)i-1; j >= 0; j--) {
		afficher_caractere(tampon[j]);
	}
}

void afficher_nombre_formate(i64 nombre) {

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

    if (negatif) {
        afficher_caractere('-');
        i--;
    }

    for (i8 j = (i8)i - 1; j >= 0; j--) {
        afficher_caractere(tampon[j]);
    }
}

void afficher(const char *format, ...) {

    va_list args;
    va_start(args, format);

    while (*format != '\0') {
        
        if (*format == '%') {    
            format++;
            
            switch (*format) {
                case 'i':
                case 'd':
                    afficher_nombre_formate(va_arg(args, i64));
                    break;

                case 's':
                    afficher_texte(va_arg(args, char*));
                    break;

                case 'c':
                    afficher_caractere((char)va_arg(args, i32));
                    break;

                case 'x':
                    afficher_adresse(va_arg(args, void *));
                    break;

                default:
                    afficher_caractere(*format);
                    break;
            }
            
        } else {
            afficher_caractere(*format);
        }
        
        format++; 
    }

    va_end(args);
}

void afficher_erreur_UEFI(EFI_SYSTEM_TABLE *table, void *image_handle, char *texte) {
    
    colorer_ecran(couleur_fond_console);
    afficher("Erreur fatale UEFI : %s\nArret dans %i secondes",texte, DELAI_ERREUR);

    table->BootServices->Stall(DELAI_ERREUR * 1000000);

    table->BootServices->Exit(image_handle, CODE_RETOUR_ERREUR, 0, (void*)0);
}

