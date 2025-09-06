#ifndef _NOEUD_H_
#define _NOEUD_H_
#include <stdlib.h>
#include <stdio.h>
#include "types.h"

int est_vide(arbre a);
arbre creer_arbre_vide(void);

/* Question 4 : créer une structure noeud  */
noeud *creer_st_noeud(char c, int i);
noeud *creer_feuille(int *tab, int index);

/* dans *p1 et *p2 contiennent les indices des deux plus petits nombrs != 0 du tableau, valent -1 si rien n'est trouvé */
void deux_plus_petits(int tableau[], int taille, int *p1, int *p2);

/* Question 7 : creer_noeud */
void creer_noeud(noeud *tab[], int taille);

/* Fonction pour libérer récursivement un arbre Huffman */
void liberer_arbre(noeud *racine);

#endif /*_NOEUD_H_ */
