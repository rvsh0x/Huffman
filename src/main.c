#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include </opt/homebrew/include/SDL2/SDL.h>
#include </opt/homebrew/include/SDL2/SDL_ttf.h>
#include "types.h"
#include "occurrences.h"
#include "noeud.h"
#include "code.h"
#include "compression.h"
#include "decompression.h"
#include "graphique.h"

void usage(char *s)
{
    printf("Programme de compression et de decompression de fichiers textes (version v5)\n\n");
    printf("Usage %s : [option] [nom_archive] [fichiers ou dossier]\n", s);
    printf("Options :\n\t-c : compression de [fichiers ou dossier] vers une archive nom_archive\n\t-d : decompression de nom_archive vers le dossier ou les fichiers d'origine\n\t\tsi [dossier_cible] est fourni, decompression dans ce dossier sinon dans le dossier courant\n\t-h  : affiche ce menu d'aide\n\t-g : affiche le programme en versions graphique\n");
}

int main(int argc, char *argv[])
{
    /*declarations des variables*/
    int t[256], i, opt, dossier_decompression = 0, nb_fichiers = 0, fic, indice = 0, est_dossier;
    FILE *fichier_depart = NULL, *fichier_dest = NULL;
    noeud *arbre_huffman[256];
    noeud *alphabet[256];
    char **liste_fichiers = NULL;
    char *nom_fich_archive, *result = NULL, *nom_dossier_decompression = NULL, *nom_dossier, *dernier_slash;
    char chemin_complet[1024], chemin_dossier[1023];
    int taille = 256;
    struct stat dir_stat, st = {0};

    liste_fichiers = (char **)malloc(MAX_FICHIERS * sizeof(char *));
    if (liste_fichiers == NULL)
    {
        printf("Erreur d'allocation memoire\n");
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < 100; i++)
    {
        liste_fichiers[i] = (char *)malloc(100 * sizeof(char));
        if (liste_fichiers[i] == NULL)
        {
            printf("Erreur d'allocation memoire\n");
            exit(EXIT_FAILURE);
        }
    }

    nom_fich_archive = (char *)malloc(100 * sizeof(char));
    nom_dossier = (char *)malloc(20 * sizeof(char));

    if (argc < 2)
    {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    while ((opt = getopt(argc, argv, "hgc:d:")) != -1)
    {
        switch (opt)
        {
        case 'h':
            usage(argv[0]);
            break;
        case 'g':
            {
                GraphiqueContext ctx;
                memset(&ctx, 0, sizeof(GraphiqueContext));
                if (!init_graphique(&ctx)) {
                    fprintf(stderr, "Failed to initialize graphics\n");
                    exit(EXIT_FAILURE);
                }
                menu(&ctx);
                quit_graphique(&ctx);
            }
            break;
        case 'c':
            if (argc < 4)
            {
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            nb_fichiers = argc - 3;
            if (nb_fichiers > 100)
            {
                printf("Erreur : trop de fichiers\n");
                exit(EXIT_FAILURE);
            }
            sprintf(nom_fich_archive, "%s", optarg);
            /* mettre une boucle pour tous les fichiers */
            for (i = optind; i < argc; i++)
            {
                stat(argv[i], &dir_stat);
                if (S_ISDIR(dir_stat.st_mode))
                {
                    /* on est dans un répertoire */
                    if ((nb_fichiers = compter_elements_dossier(argv[i])) > MAX_FICHIERS)
                    {
                        printf("Erreur : trop de fichiers dans le dossier\n");
                        exit(EXIT_FAILURE);
                    }
                    sprintf(nom_dossier, "%s", argv[i]);

                    lecture_dossier(nom_dossier, liste_fichiers, &indice);
                }
                else
                {
                    sprintf(liste_fichiers[indice], "%s", argv[i]);
                    indice++;
                }
            }

            /* compresser tous les fichiers */

            nb_fichiers = indice;

            /* ouverture du fichier de destination */
            fichier_dest = fopen(nom_fich_archive, "w+");
            if (fichier_dest == NULL)
            {
                printf("erreur de l'ouverture du fichier_dest\n");
                exit(EXIT_FAILURE);
            }
            for (fic = 0; fic < nb_fichiers; fic++)
            {
                *arbre_huffman = NULL;
                *alphabet = NULL;
                taille = 256;
                /*ouverture du fichier_depart*/
                fichier_depart = fopen(liste_fichiers[fic], "r");
                if (fichier_depart == NULL)
                {
                    printf("Impossible d'ouvrir le fichier_depart %s pour lecture \n", liste_fichiers[fic]);
                    exit(EXIT_FAILURE);
                }
                /*fin ouverture du fichier_depart*/

                occurence(fichier_depart, t);

                /*fermeture du fichier_depart*/
                if (fclose(fichier_depart) != 0)
                {
                    printf("Erreur lors de la fermeture de fichier_depart\n");
                    exit(EXIT_FAILURE);
                }
                /*fin fermeture du fichier_depart*/

                for (i = 0; i < 256; i++)
                {
                    arbre_huffman[i] = creer_st_noeud(i, t[i]);
                }

                /* création de l'arbre de Huffman */
                while (taille > 1)
                {
                    creer_noeud(arbre_huffman, taille);
                    taille--;
                }

                /* initialisation de l'alphabet */
                for (i = 0; i < 256; i++)
                {
                    alphabet[i] = NULL;
                }

                /* affectation du codage */
                creer_code(arbre_huffman[0], 0, 0, alphabet);
                /* écriture de l'en-tête */

                en_tete(fichier_dest, alphabet, liste_fichiers[fic]);

                /*ouverture du fichier_depart*/
                fichier_depart = fopen(liste_fichiers[fic], "r");
                if (fichier_depart == NULL)
                {
                    printf("Impossible d'ouvrir le fichier_depart pour codes %s \n", liste_fichiers[fic]);
                    exit(EXIT_FAILURE);
                }
                /*fin ouverture du fichier_depart*/

                codes_fichier(fichier_depart, fichier_dest, alphabet);
                fputs("\n\n\n", fichier_dest);

                if (fclose(fichier_depart) != 0)
                {
                    printf("Erreur lors de la fermeture de fichier_depart\n");
                    exit(EXIT_FAILURE);
                }
            }
            printf("L'archive est disponible dans le fichier %s\n", nom_fich_archive);
            if (fclose(fichier_dest) != 0)
            {
                printf("Erreur lors de la fermeture de fichier_dest\n");
                exit(EXIT_FAILURE);
            }
            break;
        case 'd':
            /* décompression */
            sprintf(nom_fich_archive, "%s", optarg);

            if (argc > 3)
            {
                nom_dossier_decompression = (char *)malloc(100 * sizeof(char));
                sprintf(nom_dossier_decompression, "%s", argv[optind]);
                printf("Dossier de destination des fichiers décompressés : %s\n", nom_dossier_decompression);
                dossier_decompression = 1;
            }

            fichier_depart = fopen(nom_fich_archive, "r");
            if (fichier_depart == NULL)
            {
                printf("Impossible d'ouvrir le fichier_depart \n");
                exit(EXIT_FAILURE);
            }

            while (!feof(fichier_depart))
            {
                est_dossier = 0;
                for (i = 0; i < 256; i++)
                {
                    alphabet[i] = NULL;
                }
                result = (char *)malloc(500 * sizeof(char));
                if (result == NULL)
                {
                    printf("Erreur d'allocation de mémoire\n");
                    exit(EXIT_FAILURE);
                }

                rec_alph_fich(fichier_depart, alphabet, &result);

                /* vérifier si le nom de fichier comporte un dossier */
                dernier_slash = strrchr(result, '/');
                if (dernier_slash != NULL)
                {
                    est_dossier = 1;
                    indice = dernier_slash - result;
                    strncpy(nom_dossier, result, indice);
                    nom_dossier[indice] = '\0'; /* contient le nom du dossier */
                    result = dernier_slash + 1; /* contient le nom du fichier */
                }

                recreation_huffman(alphabet, arbre_huffman);

                fichier_dest = fopen(result, "w");

                decompression(fichier_depart, fichier_dest, arbre_huffman[0], alphabet);

                if (fscanf(fichier_depart, "\n\n\n") != 0)
                {
                    printf("Erreur dans le fichier compressé\n");
                    exit(EXIT_FAILURE);
                }

                printf("Le fichier %s a été décompressé.\n", result);

                if (est_dossier)
                {
                    if (dossier_decompression)
                    {
                        if (nom_dossier_decompression[strlen(nom_dossier_decompression) - 2] == '/')
                        {
                            nom_dossier_decompression[strlen(nom_dossier_decompression) - 2] = '\0';
                        }
                        if (stat(nom_dossier_decompression, &st) == -1)
                        {
                            /* si le dossier n'existe pas on le créé */
                            mkdir(nom_dossier_decompression, 0777);
                        }
                        sprintf(chemin_dossier, "%s/%s", nom_dossier_decompression, nom_dossier);
                        if (stat(chemin_dossier, &st) == -1)
                        {
                            mkdir_p(chemin_dossier);
                        }
                        sprintf(chemin_complet, "%s/%s", chemin_dossier, result);
                        rename(result, chemin_complet);
                    }
                    else
                    {
                        if (stat(nom_dossier, &st) == -1)
                        {
                            /* si le dossier n'existe pas */
                            mkdir_p(nom_dossier);
                            /* création du dossier */
                        }
                        sprintf(chemin_complet, "%s/%s", nom_dossier, result);
                        rename(result, chemin_complet);
                    }
                }

                if (fclose(fichier_dest) != 0)
                {
                    printf("Erreur lors de la fermeture de fichier_dest\n");
                    exit(EXIT_FAILURE);
                }
            }
            if (fclose(fichier_depart) != 0)
            {
                printf("Erreur lors de la fermeture de fichier_depart\n");
                exit(EXIT_FAILURE);
            }
            break;
        case '?':
            usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    exit(EXIT_SUCCESS);
}
