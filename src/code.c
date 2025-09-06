#include "code.h"

/* Une fonction qui retourne un booleen pour savoir si un noeud est une feuille ou non */
int est_feuille(noeud n)
{
    if (est_vide(n.f_droit) && est_vide(n.f_gauche))
    {
        return 1;
    }
    return 0;
}


/* Compter le nombre de chiffre d'un entier */
int compte_chiffre(int n)
{
    int nb = 0;
    if (n == 0)
        return 1;
    while (n != 0)
    {
        n /= 10;
        nb++;
    }
    return nb;
}

/* C'est pour l'affichage */
char *affichage_code(int nbr_bits, int codage)
{
    char *code = (char *)malloc((nbr_bits + 1) * sizeof(char));
    char *temp = (char *)malloc(256 * sizeof(char));
    int i, n;
    n = compte_chiffre(codage);
    for (i = 0; i < nbr_bits - n; i++)
    {
        code[i] = '0';
    }
    code[nbr_bits - n] = '\0';

    sprintf(temp, "%d", codage);
    strcat(code, temp);
    free(temp);
    return code;
}

void creer_code(noeud *element, int code, int profondeur, noeud *alphabet[])
{
    if (est_feuille(*element))
    {
        element->nbr_bits = profondeur;
        element->codage = code;
        alphabet[(unsigned char)element->caractere] = element;
        affichage_code(element->nbr_bits, element->codage);
    }
    else
    {
        if (profondeur == 0)
        {
            /* appel récursif à gauche en injectant un 0 dans le codage */
            creer_code(element->f_gauche, 0, profondeur + 1, alphabet);
            /* appel récursif à droite en injectant un 1 dans le codage */
            creer_code(element->f_droit, 1, profondeur + 1, alphabet);
        }
        else
        {
            /* appel récursif à gauche en injectant un 0 dans le codage */
            creer_code(element->f_gauche, code * 10, profondeur + 1, alphabet);
            /* appel récursif à droite en injectant un 1 dans le codage */
            creer_code(element->f_droit, code * 10 + 1, profondeur + 1, alphabet);
        }
    }
}
