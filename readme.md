# OS UEFI

Un système d'exploitation minimaliste développé en **C**, conçu pour démarrer nativement via **UEFI**. Ce projet a pour objectif de comprendre le fonctionnement bas niveau d'un système informatique, le développement de noyau (*kernel development*) et l'interaction directe avec le matériel moderne.

---

## Fonctionnalités

* **Bootloader UEFI :** Chargement et exécution du binaire initial via l'interface UEFI.
* **Environnement Freestanding :** Compilation en C sans dépendance à la bibliothèque standard (`libc`).
* **Gestion du Noyau :** Initialisation des structures de données système de base et gestion du processeur.
* **Compatibilité Émulateur :** Prêt à être exécuté et débogué sous QEMU avec le firmware OVMF.

---

## 📁 Structure du Projet

```text
os-uefi/
├── Makefile              # Automatisation de la compilation et génération des images de boot
├── linker.ld             # Script de liaison pour le positionnement mémoire du noyau
├── EFI_cle.sh            # Script utilitaire pour préparer une clé USB UEFI bootable
├── src/                  # Code source complet du système d'exploitation
│   ├── kernel.c / .h     # Point d'entrée principal du noyau
│   ├── uefi/             # Initialisation et interfaces avec le firmware UEFI
│   ├── memoire/          # Gestion de la mémoire physique et virtuelle (pagination)
│   ├── interruptions/    # Gestion des interruptions matérielles et logicielles (IDT)
│   ├── processus/        # Gestion des tâches, des registres et du multitâche
│   ├── io/               # Gestion des entrées/sorties (clavier et affichage framebuffer)
│   ├── console.c / .h    # Console texte interactive et gestion de l'affichage textuel
│   ├── commandes.c / .h  # Interpréteur de commandes (Shell intégré au noyau)
│   ├── timer.c / .h      # Gestion de l'horloge système et du minuteur
│   ├── string.c / .h     # Fonctions utilitaires sur les chaînes (environnement freestanding)
│   ├── font.c / .h       # Polices de caractères pour l'affichage graphique de la console
│   ├── types.h           # Définitions des types de données de base
│   └── vecteur.h         # Structures de données vectorielles
├── obj/                  # Fichiers objets temporaires et dépendances de compilation (.o, .d)
├── sortie/               # Fichiers binaires finaux pour le démarrage réel (EFI/BOOT/BOOTX64.EFI, kernel.bin)
└── sortie_qemu/          # Environnement de sortie et variables NVRAM pour les tests sous l'émulateur QEMU
```

---

## Prérequis

Pour compiler et exécuter ce projet sur une machine Linux (ou WSL), assure-toi de disposer des éléments suivants :

* **GCC / Clang** (avec support de la cible `x86_64-w64-mingw32` pour l'UEFI ou une chaîne de compilation croisée)
* **GNU Make**
* **QEMU** (émulateur x86_64)
* **OVMF** (firmware UEFI libre pour QEMU)

---

## Compilation et Exécution

### 1. Cloner le dépôt
```bash
git clone https://github.com/Paul-000000/C_OS_UEFI.git
cd C_OS_UEFI
```

### 2. Compiler le projet
```bash
make
```

### 3. Exécuter sous QEMU
```bash
make run
```

---

## Objectifs

1. **Bootstrapping :** Comprendre le passage de relais du firmware UEFI vers le code du noyau.
2. **Mémoire & Pagination :** Mettre en place la table des pages et gérer la mémoire physique/virtuelle.
3. **Interruptions & Pilotes :** Intercepter les interruptions matérielles et écrire des pilotes bas niveau (affichage vidéo, clavier).
