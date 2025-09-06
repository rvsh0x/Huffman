/**
 * @file file_selector.c
 * @brief Selecteur de fichiers simple et fonctionnel
 */

#include "file_selector.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

/* Variables globales */
static int g_initialized = 0;

/* ============================================================================
 * FONCTIONS PRINCIPALES
 * ============================================================================ */

int fs_init(void) {
    if (g_initialized) {
        return 1;
    }
    
    g_initialized = 1;
    return 1;
}

void fs_shutdown(void) {
    g_initialized = 0;
}

FS_Result fs_open_dialog(GraphiqueContext* ctx, const FS_Options* options) {
    FS_Result result;
    char current_dir[512];
    char file_list[100][256];
    int file_count = 0;
    int selected_index = -1;
    int scroll_offset = 0;
    int running = 1;
    SDL_Event event;
    /* int i; */ /* Variable inutilisée */
    
    /* Initialiser le resultat */
    result.paths = NULL;
    result.count = 0;
    result.was_cancelled = 1;
    result.error = NULL;
    
    if (!ctx || !options) {
        return result;
    }
    
    /* Copier le repertoire de depart */
    strncpy(current_dir, options->start_dir ? options->start_dir : ".", sizeof(current_dir) - 1);
    current_dir[sizeof(current_dir) - 1] = '\0';
    
    /* Charger la liste des fichiers */
    fs_refresh_file_list(current_dir, file_list, &file_count);
    
    /* Boucle principale */
    while (running) {
        /* Gerer les evenements */
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        running = 0;
                        break;
                    case SDLK_RETURN:
                    case SDLK_KP_ENTER:
                        if (selected_index >= 0 && selected_index < file_count) {
                            char full_path[512];
                            
                            /* Verifier si c'est un repertoire pour navigation */
                            if (strcmp(file_list[selected_index], "..") == 0) {
                                /* Navigation vers le repertoire parent */
                                char* last_slash = strrchr(current_dir, '/');
                                if (last_slash && last_slash != current_dir) {
                                    *last_slash = '\0';
                                } else {
                                    strcpy(current_dir, "/");
                                }
                                fs_refresh_file_list(current_dir, file_list, &file_count);
                                selected_index = 0;
                                scroll_offset = 0;
                            } else {
                                /* Verifier si c'est un repertoire avec le chemin complet */
                                if (strcmp(current_dir, "/") == 0) {
                                    sprintf(full_path, "/%s", file_list[selected_index]);
                                } else {
                                    sprintf(full_path, "%s/%s", current_dir, file_list[selected_index]);
                                }
                                
                                if (fs_is_directory(full_path)) {
                                    /* Navigation vers le repertoire selectionne */
                                    strcpy(current_dir, full_path);
                                    fs_refresh_file_list(current_dir, file_list, &file_count);
                                    selected_index = 0;
                                    scroll_offset = 0;
                                } else if (fs_is_selectable(full_path, options->mode)) {
                                    /* Sélection d'un fichier */
                                    result.paths = malloc(sizeof(char*));
                                    if (result.paths) {
                                        result.paths[0] = malloc(strlen(full_path) + 1);
                                        if (result.paths[0]) {
                                            strcpy(result.paths[0], full_path);
                                            result.count = 1;
                                            result.was_cancelled = 0;
                                            running = 0;
                                        }
                                    }
                                }
                            }
                        }
                        break;
                    case SDLK_UP:
                        if (selected_index > 0) {
                            selected_index--;
                            if (selected_index < scroll_offset) {
                                scroll_offset = selected_index;
                            }
                        }
                        break;
                    case SDLK_DOWN:
                        if (selected_index < file_count - 1) {
                            selected_index++;
                            if (selected_index >= scroll_offset + 20) {
                                scroll_offset = selected_index - 19;
                            }
                        }
                        break;
                }
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mouse_x = event.button.x;
                int mouse_y = event.button.y;
                
                /* Zone de la liste des fichiers */
                if (mouse_x >= 50 && mouse_x <= 750 && mouse_y >= 140 && mouse_y <= 480) {
                    int click_index = scroll_offset + (mouse_y - 140) / 17;
                    if (click_index >= 0 && click_index < file_count) {
                        selected_index = click_index;
                        
                        /* Vérifier si c'est un répertoire pour navigation */
                        char full_path[512];
                        if (strcmp(file_list[selected_index], "..") == 0) {
                            /* Navigation vers le répertoire parent */
                            char* last_slash = strrchr(current_dir, '/');
                            if (last_slash && last_slash != current_dir) {
                                *last_slash = '\0';
                            } else {
                                strcpy(current_dir, "/");
                            }
                            fs_refresh_file_list(current_dir, file_list, &file_count);
                            selected_index = 0;
                            scroll_offset = 0;
                        } else {
                            /* Vérifier si c'est un répertoire avec le chemin complet */
                            if (strcmp(current_dir, "/") == 0) {
                                sprintf(full_path, "/%s", file_list[selected_index]);
                            } else {
                                sprintf(full_path, "%s/%s", current_dir, file_list[selected_index]);
                            }
                            
                            if (fs_is_directory(full_path)) {
                                /* Navigation vers le répertoire sélectionné */
                                strcpy(current_dir, full_path);
                                fs_refresh_file_list(current_dir, file_list, &file_count);
                                selected_index = 0;
                                scroll_offset = 0;
                            } else if (fs_is_selectable(full_path, options->mode)) {
                                /* Sélection d'un fichier */
                                result.paths = malloc(sizeof(char*));
                                if (result.paths) {
                                    result.paths[0] = malloc(strlen(full_path) + 1);
                                    if (result.paths[0]) {
                                        strcpy(result.paths[0], full_path);
                                        result.count = 1;
                                        result.was_cancelled = 0;
                                        running = 0;
                                    }
                                }
                            }
                        }
                    }
                }
                
                /* Bouton Annuler */
                if (mouse_x >= 50 && mouse_x <= 150 && mouse_y >= 520 && mouse_y <= 560) {
                    running = 0;
                }
                
                /* Bouton OK */
                if (mouse_x >= 650 && mouse_x <= 750 && mouse_y >= 520 && mouse_y <= 560) {
                    if (selected_index >= 0 && selected_index < file_count) {
                        if (fs_is_selectable(file_list[selected_index], options->mode)) {
                            result.paths = malloc(sizeof(char*));
                            if (result.paths) {
                                result.paths[0] = malloc(strlen(file_list[selected_index]) + 1);
                                if (result.paths[0]) {
                                    strcpy(result.paths[0], file_list[selected_index]);
                                    result.count = 1;
                                    result.was_cancelled = 0;
                                    running = 0;
                                }
                            }
                        }
                    }
                }
            }
        }
        
        /* Rendu */
        fs_draw_simple(ctx, current_dir, file_list, file_count, selected_index, scroll_offset, options);
    }
    
    return result;
}

void fs_draw_simple(GraphiqueContext* ctx, const char* current_dir, char file_list[][256], 
                   int file_count, int selected_index, int scroll_offset, const FS_Options* options) {
    SDL_Color bg_color = {25, 25, 35, 255};
    SDL_Color title_color = {255, 255, 255, 255};
    SDL_Color text_color = {220, 220, 220, 255};
    SDL_Color selected_bg = {70, 130, 200, 255};
    SDL_Color selected_text = {255, 255, 255, 255};
    SDL_Color button_color = {60, 60, 80, 255};
    SDL_Color button_text = {255, 255, 255, 255};
    SDL_Color border_color = {100, 100, 120, 255};
    int i;
    
    if (!ctx || !ctx->renderer) return;
    
    /* Fond avec dégradé */
    SDL_SetRenderDrawColor(ctx->renderer, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
    SDL_RenderClear(ctx->renderer);
    
    /* Bordure de la fenêtre */
    SDL_Rect window_border = {20, 20, 760, 560};
    SDL_SetRenderDrawColor(ctx->renderer, border_color.r, border_color.g, border_color.b, border_color.a);
    SDL_RenderDrawRect(ctx->renderer, &window_border);
    
    /* Titre avec fond */
    SDL_Rect title_bg = {30, 30, 740, 50};
    SDL_SetRenderDrawColor(ctx->renderer, 40, 40, 60, 255);
    SDL_RenderFillRect(ctx->renderer, &title_bg);
    
    draw_text(ctx, 50, 45, options->title ? options->title : "Selectionner fichiers", title_color);
    
    /* Répertoire actuel avec fond */
    SDL_Rect dir_bg = {30, 90, 740, 30};
    SDL_SetRenderDrawColor(ctx->renderer, 35, 35, 50, 255);
    SDL_RenderFillRect(ctx->renderer, &dir_bg);
    
    draw_text(ctx, 50, 100, "Repertoire:", text_color);
    draw_text(ctx, 150, 100, current_dir, title_color);
    
    /* Zone de la liste des fichiers */
    SDL_Rect list_bg = {30, 130, 740, 350};
    SDL_SetRenderDrawColor(ctx->renderer, 30, 30, 45, 255);
    SDL_RenderFillRect(ctx->renderer, &list_bg);
    
    /* Bordure de la liste */
    SDL_SetRenderDrawColor(ctx->renderer, border_color.r, border_color.g, border_color.b, border_color.a);
    SDL_RenderDrawRect(ctx->renderer, &list_bg);
    
    /* Liste des fichiers */
    for (i = scroll_offset; i < file_count && i < scroll_offset + 20; i++) {
        int y = 140 + (i - scroll_offset) * 17;
        /* SDL_Color current_text_color = (i == selected_index) ? selected_text : text_color; */ /* Variable inutilisée */
        
        /* Fond de sélection */
        if (i == selected_index) {
            SDL_Rect selection_rect = {35, y - 1, 730, 17};
            SDL_SetRenderDrawColor(ctx->renderer, selected_bg.r, selected_bg.g, selected_bg.b, selected_bg.a);
            SDL_RenderFillRect(ctx->renderer, &selection_rect);
        }
        
        /* Icône et nom du fichier avec distinction des types et couleurs */
        char display_name[300];
        char full_path[512];
        SDL_Color file_color = text_color; /* Couleur par défaut */
        
        /* Construire le chemin complet pour vérifier le type */
        if (strcmp(current_dir, "/") == 0) {
            sprintf(full_path, "/%s", file_list[i]);
        } else {
            sprintf(full_path, "%s/%s", current_dir, file_list[i]);
        }
        
        if (fs_is_directory(full_path)) {
            sprintf(display_name, "[DIR] %s", file_list[i]);
            /* Couleur bleue pour les dossiers */
            file_color.r = 100;
            file_color.g = 150;
            file_color.b = 255;
        } else {
            const char* ext = strrchr(file_list[i], '.');
            if (ext) {
                if (strcmp(ext, ".txt") == 0) {
                    sprintf(display_name, "[TXT] %s", file_list[i]);
                    /* Couleur verte pour les fichiers .txt */
                    file_color.r = 100;
                    file_color.g = 255;
                    file_color.b = 100;
                } else if (strcmp(ext, ".c") == 0 || strcmp(ext, ".h") == 0) {
                    sprintf(display_name, "[C] %s", file_list[i]);
                    /* Couleur cyan pour les fichiers .c et .h */
                    file_color.r = 100;
                    file_color.g = 255;
                    file_color.b = 255;
                } else if (strcmp(ext, ".huff") == 0) {
                    sprintf(display_name, "[HUFF] %s", file_list[i]);
                    /* Couleur orange pour les fichiers .huff */
                    file_color.r = 255;
                    file_color.g = 165;
                    file_color.b = 0;
                } else {
                    sprintf(display_name, "[FILE] %s", file_list[i]);
                    /* Couleur grise pour les autres fichiers */
                    file_color.r = 150;
                    file_color.g = 150;
                    file_color.b = 150;
                }
            } else {
                sprintf(display_name, "[FILE] %s", file_list[i]);
                /* Couleur grise pour les fichiers sans extension */
                file_color.r = 150;
                file_color.g = 150;
                file_color.b = 150;
            }
        }
        
        /* Appliquer la couleur de sélection si nécessaire */
        if (i == selected_index) {
            file_color = selected_text;
        }
        
        draw_text(ctx, 50, y, display_name, file_color);
    }
    
    /* Zone des boutons */
    SDL_Rect button_bg = {30, 490, 740, 60};
    SDL_SetRenderDrawColor(ctx->renderer, 40, 40, 60, 255);
    SDL_RenderFillRect(ctx->renderer, &button_bg);
    
    /* Boutons modernes */
    draw_button(ctx, 100, 500, 120, 40, "Annuler", button_text, button_color, 0);
    draw_button(ctx, 600, 500, 120, 40, "OK", button_text, button_color, 0);
    
    /* Instructions et légende des couleurs */
    draw_text(ctx, 50, 550, "Utilisez les fleches pour naviguer, Entree pour selectionner", text_color);
    
    /* Légende des couleurs */
    SDL_Color legend_color = {180, 180, 200, 255};
    draw_text(ctx, 50, 570, "Legende:", legend_color);
    
    if (options->mode == FS_MODE_COMPRESS) {
        /* Légende pour la compression */
        SDL_Color blue_color = {100, 150, 255, 255};
        SDL_Color green_color = {100, 255, 100, 255};
        SDL_Color cyan_color = {100, 255, 255, 255};
        SDL_Color gray_color = {150, 150, 150, 255};
        
        draw_text(ctx, 120, 570, "[DIR] Dossiers", blue_color);
        draw_text(ctx, 250, 570, "[TXT] .txt", green_color);
        draw_text(ctx, 320, 570, "[C] .c/.h", cyan_color);
        draw_text(ctx, 400, 570, "[FILE] Autres", gray_color);
    } else if (options->mode == FS_MODE_DECOMPRESS) {
        /* Légende pour la décompression */
        SDL_Color orange_color = {255, 165, 0, 255};
        SDL_Color blue_color = {100, 150, 255, 255};
        SDL_Color green_color = {100, 255, 150, 255};
        SDL_Color gray_color = {150, 150, 150, 255};
        
        draw_text(ctx, 120, 570, "[HUFF] .huff", orange_color);
        draw_text(ctx, 200, 570, "[ZIP] .zip/.rar", blue_color);
        draw_text(ctx, 300, 570, "[FILE] .tar/.gz", green_color);
        draw_text(ctx, 400, 570, "Autres", gray_color);
    }
    
    /* Mettre à jour l'affichage */
    SDL_RenderPresent(ctx->renderer);
}

void fs_refresh_file_list(const char* directory, char file_list[][256], int* file_count) {
    DIR* dir;
    struct dirent* entry;
    int count = 0;
    
    *file_count = 0;
    
    dir = opendir(directory);
    if (!dir) {
        return;
    }
    
    /* Ajouter ".." pour remonter */
    if (strcmp(directory, "/") != 0) {
        strcpy(file_list[count], "..");
        count++;
    }
    
    /* Lire les fichiers */
    while ((entry = readdir(dir)) != NULL && count < 100) {
        if (entry->d_name[0] != '.') {
            strncpy(file_list[count], entry->d_name, 255);
            file_list[count][255] = '\0';
            count++;
        }
    }
    
    closedir(dir);
    *file_count = count;
}

int fs_is_selectable(const char* filename, FS_Mode mode) {
    const char* ext;
    
    /* Vérifier d'abord si c'est un répertoire */
    if (fs_is_directory(filename)) {
        if (mode == FS_MODE_COMPRESS) {
            return 1; /* Permettre la sélection des dossiers pour compression */
        }
        return 0; /* Pas de sélection de répertoires pour décompression */
    }
    
    ext = strrchr(filename, '.');
    if (!ext) {
        return 0;
    }
    
    if (mode == FS_MODE_COMPRESS) {
        /* Fichiers compressibles */
        return (strcmp(ext, ".txt") == 0 || strcmp(ext, ".c") == 0 || strcmp(ext, ".h") == 0);
    } else if (mode == FS_MODE_DECOMPRESS) {
        /* Fichiers décompressibles */
        return (strcmp(ext, ".huff") == 0 || strcmp(ext, ".zip") == 0 || 
                strcmp(ext, ".rar") == 0 || strcmp(ext, ".7z") == 0 ||
                strcmp(ext, ".tar") == 0 || strcmp(ext, ".gz") == 0);
    }
    
    return 0;
}

int fs_is_directory(const char* path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    return 0;
}

void fs_free_result(FS_Result* result) {
    int i;
    
    if (!result) return;
    
    if (result->paths) {
        for (i = 0; i < (int)result->count; i++) {
            if (result->paths[i]) {
                free(result->paths[i]);
            }
        }
        free(result->paths);
        result->paths = NULL;
    }
    
    result->count = 0;
    result->was_cancelled = 1;
    
    if (result->error) {
        free(result->error);
        result->error = NULL;
    }
}

/* Fonctions vides pour compatibilité */
int fs_handle_event(GraphiqueContext* ctx, FileSelector* selector, SDL_Event* event) {
    (void)ctx;
    (void)selector;
    (void)event;
    return 0;
}

void fs_draw(GraphiqueContext* ctx, FileSelector* selector) {
    (void)ctx;
    (void)selector;
}

SDL_Color fs_get_file_color(const char* filename, int is_dir, int is_selected) {
    (void)is_selected;
    if (is_dir) {
        return (SDL_Color){100, 150, 255, 255}; /* Bleu pour les dossiers */
    }
    
    const char* ext = strrchr(filename, '.');
    if (ext) {
        if (strcmp(ext, ".huff") == 0) {
            return (SDL_Color){255, 165, 0, 255}; /* Orange pour .huff */
        } else if (strcmp(ext, ".zip") == 0 || strcmp(ext, ".rar") == 0) {
            return (SDL_Color){100, 150, 255, 255}; /* Bleu pour .zip/.rar */
        } else if (strcmp(ext, ".tar") == 0 || strcmp(ext, ".gz") == 0) {
            return (SDL_Color){100, 255, 150, 255}; /* Vert pour .tar/.gz */
        }
    }
    
    return (SDL_Color){150, 150, 150, 255}; /* Gris pour les autres */
}

const char* fs_get_file_icon(const char* filename, int is_dir) {
    if (is_dir) {
        return "[DIR]";
    }
    
    const char* ext = strrchr(filename, '.');
    if (ext) {
        if (strcmp(ext, ".huff") == 0) {
            return "[HUFF]";
        } else if (strcmp(ext, ".zip") == 0 || strcmp(ext, ".rar") == 0) {
            return "[ZIP]";
        } else if (strcmp(ext, ".tar") == 0 || strcmp(ext, ".gz") == 0) {
            return "[FILE]";
        }
    }
    
    return "[FILE]";
}

void fs_navigate_up(FileSelector* selector) {
    (void)selector;
}

void fs_navigate_home(FileSelector* selector) {
    (void)selector;
}

void fs_navigate_to(FileSelector* selector, const char* path) {
    (void)selector;
    (void)path;
}

void fs_navigate_desktop(FileSelector* selector) {
    (void)selector;
}

void fs_navigate_documents(FileSelector* selector) {
    (void)selector;
}
