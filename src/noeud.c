#include "noeud.h"

/* une fonction qui retourne un booleen pour savoir si l'arbre est vide ou non */
int est_vide(arbre a)
{
  if (a == NULL)
  {
    return 1;
  }
  else
    return 0;
}

/* une fonction pour creer l'arbre vide */
arbre creer_arbre_vide(void)
{
  return NULL;
}

/* Question 4 */
noeud *creer_st_noeud(char c, int i)
{
  arbre a = creer_arbre_vide();

  a = (arbre)malloc(sizeof(noeud));
  if (a == NULL)
  {
    printf("Erreur d'allocation de mémoire\n");
    return NULL;
  }
  a->occurence = i;
  a->codage = 0;
  a->nbr_bits = 0;
  a->caractere = c;
  a->f_gauche = NULL;
  a->f_droit = NULL;
  return a;
}

/* Question 5 */
noeud *creer_feuille(int *tab, int index)
{
  arbre a = creer_arbre_vide();

  a = (arbre)malloc(sizeof(noeud));
  if (a == NULL)
  {
    printf("Erreur d'allocation de mémoire\n");
    return NULL;
  }
  a->occurence = *tab;
  a->codage = 0;
  a->nbr_bits = 0;
  a->caractere = index;
  a->f_gauche = NULL;
  a->f_droit = NULL;
  return a;
}

void deux_plus_petits(int tab[], int taille, int *p1, int *p2)
{
  int i;
  if (taille < 2)
  {
    printf("Erreur : taille du tableau insuffisante\n");
    return;
  }
  if (tab[0] < tab[1])
  {
    *p1 = 0;
    *p2 = 1;
  }
  else
  {
    *p1 = 1;
    *p2 = 0;
  }

  for (i = 2; i < taille; i++)
  {
    if (tab[i] < tab[*p1])
    {
      *p2 = *p1;
      *p1 = i;
    }
    else if (tab[i] < tab[*p2] && i != *p1)
    {
      *p2 = i;
    }
  }
}

void creer_noeud(noeud *tab[], int taille)
{
  int p1, p2, i;
  int tab_occurrences[256];
  noeud *n;
  /* Vérifie si le tableau est non nul et si la taille est positive */
  for (i = 0; i < taille; i++)
  {
    tab_occurrences[i] = tab[i]->occurence;
  }
  
  deux_plus_petits(tab_occurrences, taille, &p1, &p2);
  /* création du nouveau noeud uniquement si l'occurence est != 0*/
  if (tab[p1]->occurence != 0)
  {
    if (tab[p2]->occurence != 0)
    {
      n = creer_st_noeud(tab[p1]->occurence + tab[p2]->occurence, tab[p1]->occurence + tab[p2]->occurence);
      n->f_gauche = tab[p2];
      n->f_droit = tab[p1];
      tab[p2]->codage = 0;
      tab[p1]->codage = 1;
      tab[p1] = n;
    }
    /* suppression du noeud p2 dans le tableau => si occurence = 0 ou comme on a créé un nouveau noeud */
    for (i = p2; i < taille - 1; i++)
    {
      tab[i] = tab[i + 1];
    }
  }
  else
  {
    /* suppression du noeud p1 dans le tableau si occurence = 0 */
    for (i = p1; i < taille - 1; i++)
    {
      tab[i] = tab[i + 1];
    }
  }
}

/* Fonction pour libérer récursivement un arbre Huffman */
void liberer_arbre(noeud *racine)
{
  if (racine == NULL) {
    return;
  }
  
  /* Libérer récursivement les sous-arbres */
  liberer_arbre(racine->f_gauche);
  liberer_arbre(racine->f_droit);
  
  /* Libérer le nœud courant */
  free(racine);
}
