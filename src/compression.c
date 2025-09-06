#include "compression.h"

void ecriture_code(int nbr_bits, int codage, FILE *fic, int pile[], int *l_pile)
{
    int i, j, bit, nb;
    char *code = affichage_code(nbr_bits, codage);
    /* boucle qui permet d'obtenir le codage bit par bit */
    for (i = nbr_bits - 1; i >= 0; i--)
    {
        /* récupère le premier bit du code */
        bit = code[0] - '0';
        /* met à jour le code */
        supprimer_premier_caractere(code);
        pile[*l_pile] = bit;
        (*l_pile)++;
        if (*l_pile == 8)
        {
            nb = 0;
            for (j = 0; j < 8; j++)
            {
                /* décale à gauche de 1 bit et OR avec le nombre pile[j] donc ajoute 1 si pile[j] vaut 1 */
                nb = (nb << 1) | pile[j];
            }
            fprintf(fic, "%c", nb);
            *l_pile = 0;
        }
    }
}

/* ecrire l'entete dans le fichier compresser */
void en_tete(FILE *fic, noeud *alphabet[], char *nom_fichier)
{
    int i;
    int n = compter_lettres_alphabet(alphabet);

    fprintf(fic, "%d\n", n);
    for (i = 0; i < 256; i++)
    {
        if (alphabet[i] != NULL)
        {
            fprintf(fic, "%c %d %d %d\n", alphabet[i]->caractere, alphabet[i]->occurence, alphabet[i]->codage, alphabet[i]->nbr_bits);
        }
    }
    fprintf(fic, "\n%s\n", nom_fichier);
}

void codes_fichier(FILE *fic_depart, FILE *fic_dest, noeud *alphabet[])
{
    int c, i, nb = 0;
    int pile[10];
    int l_pile = 0;
    while ((c = fgetc(fic_depart)) != EOF)
    {
        ecriture_code(alphabet[c]->nbr_bits, alphabet[c]->codage, fic_dest, pile, &l_pile);
    }
    if (l_pile > 0)
    {
        /* écriture des bits restants */
        for (i = 0; i < l_pile; i++)
        {
            /* décale à gauche de 1 bit et OR avec le nombre pile[j] donc ajoute 1 si pile[j] vaut 1 */
            nb = (nb << 1) | pile[i];
        }
        for (i = l_pile; i < 8; i++)
        {
            nb = (nb << 1); /* on complète le nombre par des 0 */
        }
        fprintf(fic_dest, "%c", nb);
    }
}
