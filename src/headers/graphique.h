#ifndef _GRAPHIQUE_H_
#define _GRAPHIQUE_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
/*#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>*/
#include </opt/homebrew/include/SDL2/SDL.h>
#include </opt/homebrew/include/SDL2/SDL_ttf.h>
#include "util.h"
#include "noeud.h"
#include "types.h"
#include "occurrences.h"
#include "code.h"
#include "compression.h"
#include "decompression.h"
/* Types pour le sélecteur de fichiers */
typedef enum {
    FS_MODE_OPEN,       /* Ouverture de fichier(s) */
    FS_MODE_SAVE,       /* Sauvegarde de fichier */
    FS_MODE_COMPRESS,   /* Sélection pour compression */
    FS_MODE_DECOMPRESS  /* Sélection pour décompression */
} FS_Mode;

#define WIDTH_BOX 400
#define HEIGHT_BOX 75
#define START_POINT_BOX 100
#define GAP_BOX 50
#define NB_MAX_FILES 100
#define MAX_FILES_PER_ROW 5
#define MAX_FILES_PER_COL 4
#define WINDOW_WIDTH 1400
#define WINDOW_HEIGHT 900

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_Font *font;
    int running;
} GraphiqueContext;

/* Fonctions d'initialisation et de fermeture */
int init_graphique(GraphiqueContext *ctx);
void quit_graphique(GraphiqueContext *ctx);

/* Fonctions de dessin de base */
void draw_text_box(GraphiqueContext *ctx, int x, int y, int w, int h, 
                  const char *text, SDL_Color fg, SDL_Color bg);
void draw_text(GraphiqueContext *ctx, int x, int y, const char *text, SDL_Color color);
void clear_window(GraphiqueContext *ctx, SDL_Color color);
void update_window(GraphiqueContext *ctx);

/* menu graphique général : choix entre compression et décompression */
void menu(GraphiqueContext *ctx);

/* retourne 1 si le fichier nom_fichier est présent dans la liste et 0 sinon */
int fichier_present(char **liste_fichiers, char *nom_fichier, int nb_fichiers);

/* renvoie un tableau de chaînes de caractères contenant la listes des noms des fichiers du dossier courant */
char **liste_fichiers(int *nb_fichiers);

/* menu de la compression : choix des fichiers à compresser */
void menu_compression(GraphiqueContext *ctx, int indice_depart, char **selected, int *nb_selected);

/* menu de la décompression : choix du fichier à décompresser */
void menu_decompression(GraphiqueContext *ctx, int indice_depart, char *selected);

/* Fonctions de compression et décompression */
void perform_compression(GraphiqueContext *ctx);
void perform_decompression(GraphiqueContext *ctx, char *archive_name);

/* Fonctions utilitaires */
void draw_button(GraphiqueContext *ctx, int x, int y, int w, int h, 
                const char *text, SDL_Color fg, SDL_Color bg, int hover);
int point_in_rect(int x, int y, int rect_x, int rect_y, int rect_w, int rect_h);
void show_compression_stats(GraphiqueContext *ctx, char *filename, int original_size, int compressed_size);
void draw_huffman_tree(GraphiqueContext *ctx, noeud *root, int x, int y, int level);

/* Nouvelles fonctions pour l'interface améliorée */
void draw_progress_bar(GraphiqueContext *ctx, int x, int y, int w, int h, int percent, const char *message);
void draw_file_browser(GraphiqueContext *ctx, char *current_dir, char **selected_files, int *nb_selected);
void draw_statistics(GraphiqueContext *ctx, long original_size, long compressed_size, const char *operation);
void navigate_directory(char *new_dir);
void reset_interface_state(void);

/* Fonctions du sélecteur de fichiers intégré */
int open_file_selector(GraphiqueContext* ctx, FS_Mode mode, const char* title, const char* button_text);
int open_compression_file_selector(GraphiqueContext* ctx);
int open_decompression_file_selector(GraphiqueContext* ctx);

#endif /* GRAPHIQUE_H_ */
