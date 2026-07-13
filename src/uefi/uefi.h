#ifndef UEFI_H
#define UEFI_H

#include "../types.h"
#include "../kernel.h"

#define EFIAPI __attribute__((ms_abi))

typedef u16 CHAR16;
typedef u64 EFI_STATUS;
typedef u64 EFI_PHYSICAL_ADDRESS;
typedef u64 EFI_VIRTUAL_ADDRESS;



// TEMPS

typedef struct __attribute__((packed)) {
    u16 Year;
    u8  Month;
    u8  Day;
    u8  Hour;
    u8  Minute;
    u8  Second;
    u8  Pad1;
    u32 Nanosecond;
    i16 TimeZone;
    u8  Daylight;
    u8  Pad2;
} EFI_TIME;



// MEMOIRE

typedef enum {
    AllocateAnyPages,
    AllocateMaxAddress,
    AllocateAddress,
    MaxAllocateType
} EFI_ALLOCATE_TYPE;

typedef enum {
   EfiReservedMemoryType,
   EfiLoaderCode,
   EfiLoaderData,
   EfiBootServicesCode,
   EfiBootServicesData,
   EfiRuntimeServicesCode,
   EfiRuntimeServicesData,
   EfiConventionalMemory,
   EfiUnusableMemory,
   EfiACPIReclaimMemory,
   EfiACPIMemoryNVS,
   EfiMemoryMappedIO,
   EfiMemoryMappedIOPortSpace,
   EfiPalCode,
   EfiPersistentMemory,
   EfiUnacceptedMemoryType,
   EfiMaxMemoryType
} EFI_MEMORY_TYPE;

typedef struct {
    u32 Type;           // Type de la mémoire (RAM, MMIO, ACPI, etc.)
    u32 Pad;            // Bourrage pour alignement
    void *PhysicalStart;  // Adresse physique de début
    void *VirtualStart;   // Adresse virtuelle (souvent égale à la physique en mode boot)
    u64 NumberOfPages;  // Nombre de pages de 4 Ko occupées par ce bloc
    u64 Attribute;      // Propriétés (Cache, Write-Back, etc.)
} EFI_MEMORY_DESCRIPTOR;



// AFFICHAGE TEXTE UEFI

struct __attribute__((packed)) Fonctions_affichage_UEFI;
typedef i64 (*Effacer_texte_UEFI)(struct Fonctions_affichage_UEFI *This);
typedef i64 (*Afficher_texte_UEFI)(struct Fonctions_affichage_UEFI *This, CHAR16 *String);

typedef struct __attribute__((packed)) Fonctions_affichage_UEFI {
    void *Reset;
    Afficher_texte_UEFI OutputString;
    void *TestString;
    void *QueryMode;
    void *SetMode;
    void *SetAttribute;
    Effacer_texte_UEFI ClearScreen;
} Fonctions_affichage_UEFI;



// IMAGE LOADED PROTOCOL

#define EFI_LOADED_IMAGE_PROTOCOL_GUID \
    {0x5B1B31A1, 0x9562, 0x11D2, {0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}}

typedef struct {
    u32 Revision;
    void *ParentHandle;
    struct EFI_SYSTEM_TABLE *SystemTable;
    
    // C'est ce handle qui nous intéresse ! Il représente le périphérique physique (clé USB)
    void *DeviceHandle; 
    
    void *FilePath;
    void *Reserved;
    u32 LoadOptionsSize;
    void *LoadOptions;
    void *ImageBase;
    u64 ImageSize;
    // ... il y a d'autres champs, mais on n'en a pas besoin ici
} EFI_LOADED_IMAGE_PROTOCOL;



// STRUCTURES GRAPHIQUES (GOP)

#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID {0x9042a9de, 0x23dc, 0x4a38, {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a}}

typedef struct {
    u32 RedMask;
    u32 GreenMask;
    u32 BlueMask;
    u32 ReservedMask;
} EFI_PIXEL_BITMASK;

typedef struct __attribute__((packed)) {
    u32 Version;
    u32 HorizontalResolution;
    u32 VerticalResolution;
    u32 PixelFormat;
    EFI_PIXEL_BITMASK PixelInformation; // Fait bien 16 octets maintenant
    u32 PixelsPerScanLine;              // Se retrouve au bon offset mémoire !
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef struct __attribute__((packed)){
    u32 MaxMode;
    u32 Mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
    u64 SizeOfInfo;
    u64 FrameBufferBase;
    u64 FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

typedef struct __attribute__((packed)){
    void *Padding[2];
    void *Blt;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *Mode;
} EFI_GRAPHICS_OUTPUT_PROTOCOL;



// BOOT SERVICES (Gestion de la mémoire et recherche de protocoles)

typedef struct __attribute__((packed)) {
    u32 Data1;
    u16 Data2;
    u16 Data3;
    u8 Data4[8];
} EFI_GUID;

typedef struct __attribute__((packed)){
    u64 Signature;      // 0x5453595320494645 ("EFI SYSTEM TABLE")
    u32 Revision;
    u32 HeaderSize;
    u32 CRC32;
    u32 Reserved;
} EFI_TABLE_HEADER;

typedef struct __attribute__((packed)){
    EFI_TABLE_HEADER Hdr;

    // Services TPL
    void (EFIAPI *RaiseTPL)(u64 NewTpl);
    void (EFIAPI *RestoreTPL)(u64 OldTpl);

    // Services Mémoire
    EFI_STATUS (EFIAPI *AllocatePages)(EFI_ALLOCATE_TYPE Type, EFI_MEMORY_TYPE MemoryType, u64 Pages, EFI_PHYSICAL_ADDRESS *Memory);
    EFI_STATUS (EFIAPI *FreePages)(EFI_PHYSICAL_ADDRESS Memory, u64 Pages);
    EFI_STATUS (EFIAPI *GetMemoryMap)(u64 *MemoryMapSize, void *MemoryMap, u64 *MapKey, u64 *DescriptorSize, u32 *DescriptorVersion);
    EFI_STATUS (EFIAPI *AllocatePool)(EFI_MEMORY_TYPE PoolType, u64 Size, void **Buffer);
    EFI_STATUS (EFIAPI *FreePool)(void *Buffer);

    // Services Événements
    void *CreateEvent;
    void *SetTimer;
    void *WaitForEvent;
    void *SignalEvent;
    void *CloseEvent;
    void *CheckEvent;

    // Services Protocoles
    void *InstallProtocolInterface;
    void *ReinstallProtocolInterface;
    void *UninstallProtocolInterface;
    i64 (EFIAPI *HandleProtocol)(void *Handle, EFI_GUID *Protocol, void **Interface);
    void *Reserved;
    void *RegisterProtocolNotify;
    void *LocateHandle;
    void *LocateDevicePath;
    void *InstallConfigurationTable;

    // Services Images
    void *LoadImage;
    void *StartImage;
    EFI_STATUS (EFIAPI *Exit)(void *ImageHandle, EFI_STATUS ExitStatus, u64 ExitDataSize, CHAR16 *ExitData);
    void *UnloadImage;

    // Services Divers
    EFI_STATUS (EFIAPI *ExitBootServices)(void *ImageHandle, u64 MapKey);
    void *GetNextMonotonicCount;
    EFI_STATUS (EFIAPI *Stall)(u64 Microseconds);
    void *SetWatchdogTimer;
    void *ConnectController;
    void *DisconnectController;
    void *OpenProtocol;
    void *CloseProtocol;
    void *OpenProtocolInformation;
    void *ProtocolsPerHandle;
    void *LocateHandleBuffer;

    // Recherche de protocole (index 38)
    EFI_STATUS (EFIAPI *LocateProtocol)(EFI_GUID *Protocol, void *Registration, void **Interface);
    void *InstallMultipleProtocolInterfaces;
    void *UninstallMultipleProtocolInterfaces;
    void *CalculateCrc32;
    void *CopyMem;
    void *SetMem;
    void *CreateEventEx;
} EFI_BOOT_SERVICES;



// SYSTEME DE FICHIER FAT32 UEFI

#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID {0x964e5b22, 0x6459, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}}

#define EFI_FILE_INFO_ID { 0x9576e92, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b } }

#define EFI_FILE_MODE_READ 0x0000000000000001

typedef struct __attribute__((packed)) {
    u64 Size;              // Taille de la structure elle-même
    u64 FileSize;          // Taille du fichier en octets
    u64 PhysicalSize;      // Taille physique sur le disque
    EFI_TIME CreateTime;
    EFI_TIME LastAccessTime;
    EFI_TIME ModificationTime;
    u64 Attribute;
    CHAR16 FileName[];     // Tableau dynamique
} EFI_FILE_INFO;

struct __attribute__((packed)) EFI_FILE_PROTOCOL;
typedef EFI_STATUS (*EFI_FILE_OPEN)(struct EFI_FILE_PROTOCOL *This, struct EFI_FILE_PROTOCOL **NewHandle, CHAR16 *FileName, u64 OpenMode, u64 Attributes);
typedef EFI_STATUS (*EFI_FILE_CLOSE)(struct EFI_FILE_PROTOCOL *This);
typedef EFI_STATUS (*EFI_FILE_READ)(struct EFI_FILE_PROTOCOL *This, u64 *BufferSize, void *Buffer);

typedef struct __attribute__((packed)) _EFI_FILE_PROTOCOL EFI_FILE_PROTOCOL;

struct __attribute__((packed)) _EFI_FILE_PROTOCOL {
    u64 Revision;
    EFI_STATUS (EFIAPI *Open)(EFI_FILE_PROTOCOL *This, EFI_FILE_PROTOCOL **NewHandle, CHAR16 *FileName, u64 OpenMode, u64 Attributes);
    EFI_STATUS (EFIAPI *Close)(EFI_FILE_PROTOCOL *This);
    EFI_STATUS (EFIAPI *Delete)(EFI_FILE_PROTOCOL *This);
    EFI_STATUS (EFIAPI *Read)(EFI_FILE_PROTOCOL *This, u64 *BufferSize, void *Buffer);
    EFI_STATUS (EFIAPI *Write)(EFI_FILE_PROTOCOL *This, u64 *BufferSize, void *Buffer);
    EFI_STATUS (EFIAPI *GetPosition)(EFI_FILE_PROTOCOL *This, u64 *Position);
    EFI_STATUS (EFIAPI *SetPosition)(EFI_FILE_PROTOCOL *This, u64 Position);
    EFI_STATUS (EFIAPI *GetInfo)(EFI_FILE_PROTOCOL *This, EFI_GUID *InformationType, u64 *BufferSize, void *Buffer);
    EFI_STATUS (EFIAPI *SetInfo)(EFI_FILE_PROTOCOL *This, EFI_GUID *InformationType, u64 BufferSize, void *Buffer);
    EFI_STATUS (EFIAPI *Flush)(EFI_FILE_PROTOCOL *This);
};

struct __attribute__((packed)) EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;
typedef EFI_STATUS (*EFI_VOLUME_OPEN)(struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This, EFI_FILE_PROTOCOL **Root);

typedef struct __attribute__((packed)) EFI_SIMPLE_FILE_SYSTEM_PROTOCOL {
    u64 Revision;
    EFI_VOLUME_OPEN OpenVolume;
} EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;



// TABLE SYSTÈME UEFI

typedef struct __attribute__((packed)) {
    EFI_TABLE_HEADER Hdr;
    CHAR16 *FirmwareVendor;        
    u32 FirmwareRevision;     
    u32 pad;                 
    void *ConsoleInHandle;
    void *ConIn;
    void *ConsoleOutHandle;
    Fonctions_affichage_UEFI *ConOut; 
    void *StandardErrorHandle;
    void *StdErr;
    void *RuntimeServices;
    EFI_BOOT_SERVICES *BootServices; // Ajouté pour pouvoir appeler les fonctions
    u64 NumberOfTableEntries;
    void *ConfigurationTable;
} EFI_SYSTEM_TABLE;



typedef void __attribute__((sysv_abi)) (*KernelStart)(Informations_Boot*);

#endif