#ifndef _COMPRESSION_H_
#define _COMPRESSION_H_
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "types.h"
#include "util.h"
#include "code.h"

/* permet d'écrire le code dans un fichier avec un système de pile */
void ecriture_code(int nbr_bits, int codage, FILE *fic, int pile[], int *l_pile);

/* Fonction qui permet d'écrire l'en-tête dans le fichier fic ouvert en mode écriture <<binaire>>

1ère ligne => nb de feuilles
2ème ligne => les différents noeuds sous la forme : v o c n
- v valeur du caractère
- o nb d'occurences
- c codage
- n nb de bits
3ème ligne => nom d'origine du fichier
 */
void en_tete(FILE *fic, noeud *alphabet[], char *nom_fichier);

/* fonction écrit dans fic_dest le contenu codé de fic_depart */
void codes_fichier(FILE *fic_depart, FILE *fic_dest, noeud *alphabet[]);

#endif /*_COMPRESSION_H_ */
