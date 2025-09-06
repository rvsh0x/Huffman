#ifndef _UTIL_H_
#define _UTIL_H_
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include "noeud.h"
#include "types.h"

#define MAX_FICHIERS 100

/* fonction qui retourne le nombre de lettres dans l'alphabet (donc le nombre de noeuds non NULL dans alphabet) */
int compter_lettres_alphabet(noeud *alphabet[]);

int nb_car_total(noeud *alphabet[]);

void supprimer_premier_caractere(char *chaine);

char *conversion_binaire(char c);

void afficher_arbre(arbre a, int niveau);

int compter_elements_dossier(char *chemin);

void lecture_dossier(char *chemin_dossier, char **liste_fichiers, int *indice);

void mkdir_p(char *chemin);

#endif /*_UTIL_H_ */
