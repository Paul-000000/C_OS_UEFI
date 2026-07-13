#!/bin/bash

USB_PATH="/media/paul/8B0E-4D91"
SOURCE_EFI="./sortie/EFI"

if [ -d "$USB_PATH" ]; then

	echo "Clé détectée"
	rm -rf "$USB_PATH/EFI"
	if [ -d "$SOURCE_EFI" ]; then
	
		cp -r "$SOURCE_EFI" "$USB_PATH/"
		sync
		echo "Copie terminée."
		
		gio mount -u "$USB_PATH"
		
		if [ $? -eq 0 ]; then
		    echo "Clé éjectée avec succès"
		else
		    echo "Erreur lors de l'éjection. La clé est peut-être toujours utilisée"
		fi
	    else
		echo "Erreur : Le dossier source $SOURCE_EFI n'existe pas."
	    fi
else
    echo "Erreur : La clé USB n'est pas montée à $USB_PATH"
fi
