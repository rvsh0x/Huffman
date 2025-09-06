#include "decompression.h"

/* decompresser l'entete*/
void rec_alph_fich(FILE *fic, noeud *alphabet[], char **nom_fichier)
{
    noeud *element;
    int n = 0, nb_lu = 0, i = 0;
    char c, c_courant, c_lu = 0;
    c_courant='0';
    /* nombre de feuilles */
    while ((c = fgetc(fic)) && c != '\n')
    {
        n = n * 10 + (c - '0');
    }
    while (nb_lu < n)
    {
        c = fgetc(fic);
        switch (i)
        {
        case 0:
            /* lecture du caractère */
            if (c_lu && c == ' ')
            {
                i++;
            }
            else
            {
                element = creer_st_noeud(c, 0);
                c_courant = c;
                c_lu = 1;
                alphabet[(unsigned char)c_courant] = element;
            }
            break;
        case 1:
            /* lecture du nombre d'occurences */
            if (c == ' ')
            {
                i++;
            }
            else
            {
                alphabet[(unsigned char)c_courant]->occurence = alphabet[(unsigned char)c_courant]->occurence * 10 + (c - '0');
            }
            break;
        case 2:
            /* lecture du codage */
            if (c == ' ')
            {
                i++;
            }
            else
            {
                alphabet[(unsigned char)c_courant]->codage = alphabet[(unsigned char)c_courant]->codage * 10 + (c - '0');
            }
            break;
        case 3:
            /* lecture du nombre de bits */
            if (c == '\n')
            {
                i = 0;
                nb_lu++;
                c_lu = 0;
            }
            else
            {
                alphabet[(unsigned char)c_courant]->nbr_bits = alphabet[(unsigned char)c_courant]->nbr_bits * 10 + (c - '0');
            }
            break;
        }
    }
    fgetc(fic); /* \n */
    if (fgets(*nom_fichier, 500, fic) == NULL)
    {
        printf("erreur de lecture du fichier compressé\n");
        exit(EXIT_FAILURE);
    };
    (*nom_fichier)[strcspn(*nom_fichier, "\n")] = '\0'; /* retire le \n du nom du fichier */
    /* enlève le nom du dossier */
    /*    nom = strchr(*nom_fichier, '/');
        if (nom != NULL)
        {
            strcpy(*nom_fichier, nom + 1);
       } */
}

void recreation_huffman(noeud *alphabet[], noeud *huffman[])
{
    int i, taille = 256;
    /* initialisation de l'arbre de huffman */
    for (i = 0; i < 256; i++)
    {
        if (alphabet[i] != NULL)
        {
            huffman[i] = creer_st_noeud(i, alphabet[i]->occurence);
        }
        else
        {
            huffman[i] = creer_st_noeud(i, 0);
        }
    }
    while (taille > 1)
    {
        creer_noeud(huffman, taille);
        taille--;
    }
}

/* Pour savoir si le code existe dans l'arbre huffman ou non*/
int code_existe(noeud *arbre_huffman, char *code)
{
    if (arbre_huffman == NULL)
    {
        return 0;
    }
    if (arbre_huffman->f_droit == NULL && arbre_huffman->f_gauche == NULL)
    {
        return *code == '\0';
    }

    if (*code == '0')
    {
        return code_existe(arbre_huffman->f_gauche, code + 1);
    }
    else if (*code == '1')
    {
        return code_existe(arbre_huffman->f_droit, code + 1);
    }
    return 0;
}

char retourne_caractere(noeud *alphabet[], char *code)
{
    int i;
    for (i = 0; i < 256; i++)
    {
        if (alphabet[i] != NULL)
        {
            if (strcmp(affichage_code(alphabet[i]->nbr_bits, alphabet[i]->codage), code) == 0)
            {
                return alphabet[i]->caractere;
            }
        }
    }
    return '\0';
}

/* decompresser le fichier */
void decompression(FILE *fic_comp, FILE *fic_decom, noeud *arbre_huffman, noeud *alphabet[])
{
    char c = fgetc(fic_comp), next_c = fgetc(fic_comp);
    int compteur_bit, nb_car = 0;
    char *code, *binaire;
    code = (char *)malloc(256 * sizeof(char));
    if (code == NULL)
    {
        printf("erreur d'allocation mémoire\n");
        exit(EXIT_FAILURE);
    }
    code[0] = '\0';
    while (!feof(fic_comp) && nb_car < nb_car_total(alphabet))
    {
        compteur_bit = 0;
        binaire = conversion_binaire(c);
        while (compteur_bit < 8)
        {
            strncat(code, binaire, 1);
            supprimer_premier_caractere(binaire);
            if (code_existe(arbre_huffman, code))
            {
                fputc(retourne_caractere(alphabet, code), fic_decom);
                nb_car++;
                /* permet de ne pas écrire les 0 ajoutés */
                if (next_c == EOF && retourne_caractere(alphabet, code) == '\n')
                {
                    break;
                }
                code[0] = '\0';
            }
            compteur_bit++;
        }
        c = next_c;
        next_c = fgetc(fic_comp);
    }
    compteur_bit = 0;
    binaire = conversion_binaire(c);
    while (compteur_bit < 8)
    {
        strncat(code, binaire, 1);
        supprimer_premier_caractere(binaire);
        if (code_existe(arbre_huffman, code))
        {
            if (nb_car != nb_car_total(alphabet))
            {
                fputc(retourne_caractere(alphabet, code), fic_decom);
                nb_car++;
            }
            if (next_c == EOF && retourne_caractere(alphabet, code) == '\n')
            {
                compteur_bit = 7;
            }
            else
            {
                code[0] = '\0';
            }
        }
        compteur_bit++;
    }
}
