#ifndef _DECOMPRESSION_H_
#define _DECOMPRESSION_H_
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "types.h"
#include "util.h"
#include "code.h"

/* lecture de l'en-tête pour reconnaître l'alphabet du fichier */
void rec_alph_fich(FILE *fic, noeud *alphabet[], char **nom_fichier);

void recreation_huffman(noeud *alphabet[], noeud *huffman[]);

/* fonction qui renvoie 1 si le code existe dans l'arbre et 0 sinon */
int code_existe(noeud *arbre_huffman, char *code);

void decompression(FILE *fic_comp, FILE *fic_decom, noeud *arbre_huffman, noeud *alphabet[]);

#endif /*_DECOMPRESSION_H_ */
