/**
 * @file file_selector.h
 * @brief Sélecteur de fichiers intégré pour le projet Huffman
 */

#ifndef FILE_SELECTOR_H
#define FILE_SELECTOR_H

#include "graphique.h"

/* Types de sélection - défini dans graphique.h */

/* Options de sélection */
typedef struct {
    FS_Mode mode;                   /* Mode de sélection */
    int multiselect;                /* Sélection multiple */
    int show_hidden;                /* Afficher les fichiers cachés */
    const char* start_dir;          /* Répertoire initial */
    const char* title;              /* Titre de la fenêtre */
    const char* button_text;        /* Texte du bouton de validation */
} FS_Options;

/* Résultat de sélection */
typedef struct {
    char** paths;           /* Chemins sélectionnés */
    size_t count;           /* Nombre de fichiers */
    int was_cancelled;      /* Annulé par l'utilisateur */
    char* error;            /* Message d'erreur */
} FS_Result;

/* État du sélecteur */
typedef struct {
    int is_open;            /* Sélecteur ouvert */
    FS_Options options;     /* Options de configuration */
    char current_path[512]; /* Chemin actuel */
    char** file_list;       /* Liste des fichiers */
    size_t file_count;      /* Nombre de fichiers */
    int selected_index;     /* Index sélectionné */
    int scroll_offset;      /* Décalage de scroll */
    char search_text[256];  /* Texte de recherche */
    int search_active;      /* Recherche active */
    char** selected_paths;  /* Chemins sélectionnés */
    size_t selected_count;  /* Nombre de sélections */
    char* error_message;    /* Message d'erreur */
} FileSelector;

/* ============================================================================
 * FONCTIONS PRINCIPALES
 * ============================================================================ */

/**
 * @brief Initialise le sélecteur de fichiers
 * @return true si succès, false sinon
 */
int fs_init(void);

/**
 * @brief Ferme le sélecteur de fichiers
 */
void fs_shutdown(void);

/**
 * @brief Ouvre un dialogue de sélection
 * @param ctx Contexte graphique
 * @param options Options de configuration
 * @return Résultat de la sélection
 */
FS_Result fs_open_dialog(GraphiqueContext* ctx, const FS_Options* options);

/**
 * @brief Libère un résultat de sélection
 * @param result Résultat à libérer
 */
void fs_free_result(FS_Result* result);

/**
 * @brief Dessine le sélecteur de fichiers simple
 * @param ctx Contexte graphique
 * @param current_dir Répertoire actuel
 * @param file_list Liste des fichiers
 * @param file_count Nombre de fichiers
 * @param selected_index Index sélectionné
 * @param scroll_offset Décalage de scroll
 * @param options Options de configuration
 */
void fs_draw_simple(GraphiqueContext* ctx, const char* current_dir, char file_list[][256], 
                   int file_count, int selected_index, int scroll_offset, const FS_Options* options);

/* ============================================================================
 * FONCTIONS UTILITAIRES
 * ============================================================================ */

/**
 * @brief Vérifie si un fichier est sélectionnable
 * @param filename Nom du fichier
 * @param mode Mode de sélection
 * @return true si sélectionnable, false sinon
 */
int fs_is_selectable(const char* filename, FS_Mode mode);

/**
 * @brief Vérifie si un chemin est un répertoire
 * @param path Chemin à vérifier
 * @return true si c'est un répertoire, false sinon
 */
int fs_is_directory(const char* path);

/**
 * @brief Rafraîchit la liste des fichiers
 * @param directory Répertoire à lister
 * @param file_list Liste de fichiers (sortie)
 * @param file_count Nombre de fichiers (sortie)
 */
void fs_refresh_file_list(const char* directory, char file_list[][256], int* file_count);

/* ============================================================================
 * FONCTIONS DE RENDU
 * ============================================================================ */

/**
 * @brief Dessine le sélecteur de fichiers
 * @param ctx Contexte graphique
 * @param selector État du sélecteur
 */
void fs_draw(GraphiqueContext* ctx, FileSelector* selector);

/**
 * @brief Gère les événements du sélecteur
 * @param ctx Contexte graphique
 * @param selector État du sélecteur
 * @param event Événement SDL2
 * @return true si l'événement a été traité
 */
int fs_handle_event(GraphiqueContext* ctx, FileSelector* selector, SDL_Event* event);

/* ============================================================================
 * FONCTIONS UTILITAIRES
 * ============================================================================ */


/**
 * @brief Navigue vers un répertoire
 * @param selector État du sélecteur
 * @param path Chemin du répertoire
 */
void fs_navigate_to(FileSelector* selector, const char* path);

/**
 * @brief Navigue vers le répertoire parent
 * @param selector État du sélecteur
 */
void fs_navigate_up(FileSelector* selector);

/**
 * @brief Navigue vers le répertoire home
 * @param selector État du sélecteur
 */
void fs_navigate_home(FileSelector* selector);

/**
 * @brief Navigue vers le bureau
 * @param selector État du sélecteur
 */
void fs_navigate_desktop(FileSelector* selector);

/**
 * @brief Navigue vers les documents
 * @param selector État du sélecteur
 */
void fs_navigate_documents(FileSelector* selector);

/**
 * @brief Filtre les fichiers selon le type
 * @param selector État du sélecteur
 * @param mode Mode de sélection
 */
void fs_filter_files(FileSelector* selector, FS_Mode mode);

/**
 * @brief Vérifie si un fichier est sélectionnable
 * @param filename Nom du fichier
 * @param mode Mode de sélection
 * @return true si sélectionnable, false sinon
 */
int fs_is_selectable(const char* filename, FS_Mode mode);

/**
 * @brief Obtient l'icône d'un fichier
 * @param filename Nom du fichier
 * @param is_dir true si c'est un répertoire
 * @return Icône à afficher
 */
const char* fs_get_file_icon(const char* filename, int is_dir);

/**
 * @brief Obtient la couleur d'un fichier
 * @param filename Nom du fichier
 * @param is_dir true si c'est un répertoire
 * @param is_selected true si sélectionné
 * @return Couleur SDL
 */
SDL_Color fs_get_file_color(const char* filename, int is_dir, int is_selected);

#endif /* FILE_SELECTOR_H */
