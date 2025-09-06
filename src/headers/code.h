#ifndef _CODE_H_
#define _CODE_H_
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "noeud.h"
#include "types.h"

/* retourne 1 si le noeud est une feuille et 0 sinon */
int est_feuille(noeud n);

/* affiche le code binaire */
char *affichage_code(int nbr_bits, int codage);

/* fonction récursive pour créer le code d'un noeud */
void creer_code(noeud *element, int code, int profondeur, noeud *alphabet[]);

#endif /*_CODE_H_ */
