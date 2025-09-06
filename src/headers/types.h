#ifndef _TYPES_H_
#define _TYPES_H_

typedef struct noeud
{
  char caractere;
  int occurence;
  int codage;
  int nbr_bits;
  struct noeud *f_gauche;
  struct noeud *f_droit;
} noeud;
typedef noeud *arbre;

#endif /* _TYPES_H_ */
