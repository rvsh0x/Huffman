#include <util.h>

/*une fonction qui retourne le nombre de lettres dans un alphabet*/
int compter_lettres_alphabet(noeud *alphabet[])
{
    int i, n = 0;
    for (i = 0; i < 256; i++)
    {
        if (alphabet[i] != NULL)
        {
            n++;
        }
    }
    return n;
}

/*une fonction qui retourne le nombre de caracteres total dans un alphabet*/
int nb_car_total(noeud *alphabet[])
{
    int i, n = 0;
    for (i = 0; i < 256; i++)
    {
        if (alphabet[i] != NULL)
        {
            n += alphabet[i]->occurence;
        }
    }
    return n;
}
/*une fonction qui supprime le premier caractere dans une chaine de caracteres*/
void supprimer_premier_caractere(char *chaine)
{
    int i;
    for (i = 0; chaine[i] != '\0'; i++)
    {
        chaine[i] = chaine[i + 1];
    }
}

/*une fonction de conversion binaire*/
char *conversion_binaire(char c)
{
    int i;
    char *binaire = (char *)malloc(9 * sizeof(char));
    for (i = 7; i >= 0; i--)
    {
        binaire[7 - i] = (c & (1 << i)) ? '1' : '0';
    }
    binaire[8] = '\0';
    return binaire;
}

/* une fonction pour afficher un arbre*/
void afficher_arbre(arbre a, int niveau)
{
    int i;
    if (a == NULL)
    {
        return;
    }

    for (i = 0; i < niveau; i++)
    {
        printf("  ");
    }

    printf("Caractere: %c, Occurence: %d, Codage: %d, Nbr_bits: %d\n", a->caractere, a->occurence, a->codage, a->nbr_bits);

    afficher_arbre(a->f_gauche, niveau + 1);
    afficher_arbre(a->f_droit, niveau + 1);
}

int compter_elements_dossier(char *chemin)
{
    DIR *dossier;
    struct dirent *element;
    int compteur = 0;

    dossier = opendir(chemin);
    if (dossier == NULL)
    {
        printf("Erreur lors de l'ouverture du dossier\n");
        return -1;
    }

    while ((element = readdir(dossier)) != NULL)
    {
        compteur++;
    }

    closedir(dossier);

    return compteur - 2; /* -2 pour enlever ".." et "."du compteur de fichiers */
}

void lecture_dossier(char *chemin_dossier, char **liste_fichiers, int *indice)
{
    DIR *dir;
    struct dirent *ent;
    struct stat dir_stat;
    char chemin_complet[1024];
    /* ajoute un '/' à la fin du nom s'il n'y en a pas */
    size_t len = strlen(chemin_dossier);
    if (chemin_dossier[len - 1] != '/')
    {
        chemin_dossier[len] = '/';
        chemin_dossier[len + 1] = '\0';
    }

    stat(chemin_dossier, &dir_stat); /* obtenir les infos sur le chemin */

    if ((dir = opendir(chemin_dossier)) != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            sprintf(chemin_complet, "%s%s", chemin_dossier, ent->d_name);
            if (stat(chemin_complet, &dir_stat) == 0 && S_ISREG(dir_stat.st_mode)) /* si elle est un fichier */
            {
                {
                    strcpy(liste_fichiers[*indice], chemin_complet);
                    (*indice)++;
                }
            }
            else if (S_ISDIR(dir_stat.st_mode) && strcmp(ent->d_name, "..") != 0 && strcmp(ent->d_name, ".") != 0)
            { /* si elle est un dossier et il ne s agit pas de dossiers speciaux */
                lecture_dossier(chemin_complet, liste_fichiers, indice);
            }
        }
        closedir(dir);
    }
}

void mkdir_p(char *chemin)
{
    char tmp[1024];
    char *pos = NULL;
    size_t len;

    strncpy(tmp, chemin, sizeof(tmp));
    tmp[sizeof(tmp) - 1] = '\0';

    /* écrit chemin dans tmp */
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
    {
        /* si on finit par un dossier, on termina la chaine */
        tmp[len - 1] = '\0';
    }
    /* parcourt le chemin caractère par caractère */
    for (pos = tmp + 1; *pos != '\0'; pos++)
    {
        if (*pos == '/')
        {
            *pos = '\0';
            /* créé le dossier */
            mkdir(tmp, 0777); /* EN C c est juste ... mais en C++ NONN elle prend le chemin et les permissions */
            *pos = '/';
        }
    }
    mkdir(tmp, 0777);
}
