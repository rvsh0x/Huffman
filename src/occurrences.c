#include "occurrences.h"

#define taille_max 200

/* Question 1 */
void ouvrir_fichier(FILE *fic)
{
    int bytes_lu;
    char buffer[taille_max];
    bytes_lu = fread(buffer, sizeof(char), taille_max, fic);
    printf("Contenu du fichier :\n");
    printf("%.*s", bytes_lu, buffer);
}

/* Question 2 */
void lecture_fichier(FILE *fic)
{
    char caractere;
    printf("Contenu du fichier :\n");
    while ((caractere = fgetc(fic)) != EOF)
    {
        printf("%c", caractere);
    }
}

/* Question 3 */
void occurence(FILE *fic, int tab[])
{
    int i;
    int caractere;

    /* initialiser le tableau */
    for (i = 0; i < 256; i++)
    {
        tab[i] = 0;
    }

    /* remplir le tableau */
    while ((caractere = fgetc(fic)) != EOF)
    {
        tab[caractere]++;
    }

    /*affichage du tableau*/
    /*
    for (i = 0; i < 256; i++)
    {
        if (tab[i] > 0)
        {
            printf("Le caractère '%c' apparaît %d fois.\n", i, tab[i]);
        }
    } */
}
