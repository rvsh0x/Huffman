#include "graphique.h"
#include "file_selector.h"
#include <unistd.h>
#include <math.h>

/* Variables globales pour l'interface */
static char archive_name[256] = "";
static char output_filename[256] = ""; /* Nom du fichier de sortie pour la decompression */
static char selected_files[NB_MAX_FILES][256];
static int nb_selected_files = 0;
static char current_directory[512] = ".";
static int current_mode = 0; /* 0: menu principal, 1: compression, 2: decompression */
static int file_browser_open = 0; /* 0: ferme, 1: ouvert */
static int text_input_active = 0; /* 0: inactif, 1: actif */
static int text_input_mode = 0; /* 0: nom archive, 1: nom sortie decompression */
static int show_warning = 0; /* 0: pas d'avertissement, 1: afficher avertissement */
static char warning_message[256] = "";

/* Variables pour les animations */
static int animation_frame = 0;
static int button_animation = 0;
/* static int fade_alpha = 255; */ /* Variable inutilisée */
static int progress_percent = 0;
static char progress_message[256] = "";
static int show_progress = 0;
static long total_original_size = 0;
static long total_compressed_size = 0;
static int file_browser_scroll = 0;

/* ============================================================================
 * FONCTIONS DU SELECTEUR DE FICHIERS
 * ============================================================================ */

/**
 * @brief Ouvre le selecteur de fichiers
 * @param ctx Contexte graphique
 * @param mode Mode de selection
 * @param title Titre de la fenetre
 * @param button_text Texte du bouton de validation
 * @return true si des fichiers ont été sélectionnés, false sinon
 */
int open_file_selector(GraphiqueContext* ctx, FS_Mode mode, const char* title, const char* button_text) {
    FS_Options options;
    
    if (!ctx) return 0;
    
    /* Configuration du sélecteur */
    options.mode = mode;
    options.multiselect = (mode == FS_MODE_COMPRESS) ? 1 : 0; /* Sélection multiple pour compression */
    options.show_hidden = 0;
    options.start_dir = current_directory;
    options.title = title;
    options.button_text = button_text;
    
    /* Ouvrir le dialogue */
    FS_Result result;
    result = fs_open_dialog(ctx, &options);
    
    if (!result.was_cancelled && result.count > 0) {
        /* Copier les fichiers sélectionnés */
        size_t i;
        nb_selected_files = 0;
        for (i = 0; i < result.count && i < NB_MAX_FILES; i++) {
            strncpy(selected_files[nb_selected_files], result.paths[i], sizeof(selected_files[0]) - 1);
            selected_files[nb_selected_files][sizeof(selected_files[0]) - 1] = '\0';
            nb_selected_files++;
        }
        
        /* Mettre à jour le répertoire actuel */
        if (result.count > 0) {
            char* last_slash = strrchr(result.paths[0], '/');
            if (last_slash) {
                strncpy(current_directory, result.paths[0], last_slash - result.paths[0]);
                current_directory[last_slash - result.paths[0]] = '\0';
            }
            
            /* Pour la décompression, définir archive_name avec le nom de l'archive sélectionnée */
            if (mode == FS_MODE_DECOMPRESS) {
                strcpy(archive_name, last_slash ? last_slash + 1 : result.paths[0]);
            }
        }
        
        /* Libérer le résultat */
        fs_free_result(&result);
        return 1;
    }
    
    /* Libérer le résultat */
    fs_free_result(&result);
    return 0;
}

/**
 * @brief Ouvre le sélecteur pour la compression
 * @param ctx Contexte graphique
 * @return true si des fichiers ont été sélectionnés, false sinon
 */
int open_compression_file_selector(GraphiqueContext* ctx) {
    return open_file_selector(ctx, FS_MODE_COMPRESS, "Selectionner des fichiers a compresser", "Compresser");
}

/**
 * @brief Ouvre le sélecteur pour la décompression
 * @param ctx Contexte graphique
 * @return true si des fichiers ont été sélectionnés, false sinon
 */
int open_decompression_file_selector(GraphiqueContext* ctx) {
    return open_file_selector(ctx, FS_MODE_DECOMPRESS, "Selectionner une archive a decompresser", "Decompresser");
}

int init_graphique(GraphiqueContext *ctx) {
    printf("Initializing SDL...\n");
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        return 0;
    }

    printf("Initializing TTF...\n");
    if (TTF_Init() < 0) {
        fprintf(stderr, "TTF initialization failed: %s\n", TTF_GetError());
        SDL_Quit();
        return 0;
    }

    printf("Creating window...\n");
    ctx->window = SDL_CreateWindow("Compresseur Huffman - Interface Professionnelle",
                                 SDL_WINDOWPOS_CENTERED,
                                 SDL_WINDOWPOS_CENTERED,
                                 WINDOW_WIDTH, WINDOW_HEIGHT,
                                 SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!ctx->window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 0;
    }

    printf("Creating renderer...\n");
    ctx->renderer = SDL_CreateRenderer(ctx->window, -1,
                                     SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ctx->renderer) {
        fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(ctx->window);
        TTF_Quit();
        SDL_Quit();
        return 0;
    }

    const char* font_paths[6];
    printf("Loading font...\n");
    /* Essayer plusieurs chemins de police pour macOS */
    font_paths[0] = "/System/Library/Fonts/Helvetica.ttc";
    font_paths[1] = "/System/Library/Fonts/Arial.ttf";
    font_paths[2] = "/System/Library/Fonts/Times.ttc";
    font_paths[3] = "/System/Library/Fonts/Courier.ttc";
    font_paths[4] = "/Library/Fonts/Arial.ttf";
    font_paths[5] = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
    
    int i = 0;
    while (font_paths[i] != NULL && !ctx->font) {
        ctx->font = TTF_OpenFont(font_paths[i], 18);
        if (ctx->font) {
            printf("Font loaded successfully: %s\n", font_paths[i]);
            break;
        }
        i++;
    }
    
    if (!ctx->font) {
        printf("Warning: No custom font loaded, using default font\n");
        /* Continuer sans police personnalisée - SDL_ttf peut utiliser une police par défaut */
    }

    ctx->running = 1;
    /* Initialiser le sélecteur de fichiers */
    if (!fs_init()) {
        fprintf(stderr, "File selector initialization failed\n");
        TTF_CloseFont(ctx->font);
        SDL_DestroyRenderer(ctx->renderer);
        SDL_DestroyWindow(ctx->window);
        TTF_Quit();
        SDL_Quit();
        return 0;
    }

    printf("Initialization complete!\n");
    return 1;
}

void quit_graphique(GraphiqueContext *ctx) {
    /* Fermer le sélecteur de fichiers */
    fs_shutdown();
    
    if (ctx->font) {
        TTF_CloseFont(ctx->font);
    }
    if (ctx->renderer) {
        SDL_DestroyRenderer(ctx->renderer);
    }
    if (ctx->window) {
        SDL_DestroyWindow(ctx->window);
    }
    TTF_Quit();
    SDL_Quit();
}

void draw_text_box(GraphiqueContext *ctx, int x, int y, int w, int h, 
                  const char *text, SDL_Color fg, SDL_Color bg) {
    SDL_Surface *surface = NULL;
    SDL_Texture *texture = NULL;
    SDL_Rect rect, text_rect;

    if (!ctx || !ctx->renderer) return;

    /* Dessiner le fond */
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    SDL_SetRenderDrawColor(ctx->renderer, bg.r, bg.g, bg.b, bg.a);
    SDL_RenderFillRect(ctx->renderer, &rect);
    
    /* Dessiner le contour */
    SDL_SetRenderDrawColor(ctx->renderer, fg.r, fg.g, fg.b, fg.a);
    SDL_RenderDrawRect(ctx->renderer, &rect);
    
    /* Dessiner le texte */
    if (ctx->font && text) {
    surface = TTF_RenderText_Blended_Wrapped(ctx->font, text, fg, w - 10);
    if (surface) {
        texture = SDL_CreateTextureFromSurface(ctx->renderer, surface);
        if (texture) {
            text_rect.x = x + (w - surface->w) / 2;
            text_rect.y = y + (h - surface->h) / 2;
            text_rect.w = surface->w;
            text_rect.h = surface->h;
            SDL_RenderCopy(ctx->renderer, texture, NULL, &text_rect);
            SDL_DestroyTexture(texture);
        }
        SDL_FreeSurface(surface);
        }
    } else if (text) {
        /* Si pas de police, dessiner un rectangle simple au centre */
        text_rect.x = x + 10;
        text_rect.y = y + h/2 - 8;
        text_rect.w = w - 20;
        text_rect.h = 16;
        SDL_SetRenderDrawColor(ctx->renderer, fg.r, fg.g, fg.b, fg.a);
        SDL_RenderFillRect(ctx->renderer, &text_rect);
    }
}

void draw_text(GraphiqueContext *ctx, int x, int y, const char *text, SDL_Color color) {
    SDL_Surface *surface = NULL;
    SDL_Texture *texture = NULL;
    SDL_Rect dest;
    
    char clean_text[512];
    int i, j = 0;
    
    if (!text || !ctx || !ctx->renderer) return;
    
    /* Nettoyer le texte des caractères bizarres mais garder les accents */
    for (i = 0; text[i] != '\0' && j < 511; i++) {
        char c = text[i];
        /* Garder les caractères ASCII imprimables et les caractères étendus (accents) */
        if ((c >= 32 && c <= 126) || ((unsigned char)c >= 128 && (unsigned char)c <= 255)) {
            clean_text[j++] = c;
        } else if (c == '\n') {
            clean_text[j++] = ' ';
        }
    }
    clean_text[j] = '\0';
    
    if (!ctx->font) {
        /* Si pas de police, dessiner un rectangle simple */
        SDL_Rect rect;
        rect.x = x;
        rect.y = y;
        rect.w = strlen(clean_text) * 8;
        rect.h = 16;
        SDL_SetRenderDrawColor(ctx->renderer, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(ctx->renderer, &rect);
        return;
    }
    
    surface = TTF_RenderText_Blended(ctx->font, clean_text, color);
    if (surface) {
        texture = SDL_CreateTextureFromSurface(ctx->renderer, surface);
        if (texture) {
            dest.x = x;
            dest.y = y;
            dest.w = surface->w;
            dest.h = surface->h;
            SDL_RenderCopy(ctx->renderer, texture, NULL, &dest);
            SDL_DestroyTexture(texture);
        }
        SDL_FreeSurface(surface);
    }
}

void clear_window(GraphiqueContext *ctx, SDL_Color color) {
    SDL_SetRenderDrawColor(ctx->renderer, color.r, color.g, color.b, color.a);
    SDL_RenderClear(ctx->renderer);
}

void update_window(GraphiqueContext *ctx) {
    SDL_RenderPresent(ctx->renderer);
}

void draw_button(GraphiqueContext *ctx, int x, int y, int w, int h, 
                const char *text, SDL_Color fg, SDL_Color bg, int hover) {
    SDL_Color button_bg, button_fg, shadow_color;
    SDL_Rect button_rect, shadow_rect;
    int shadow_offset = 3;
    int animation_offset = 0;
    
    if (!ctx || !ctx->renderer) return;
    
    /* Animation de hover */
    if (hover) {
        animation_offset = 2;
        button_animation = (button_animation + 1) % 20;
    }
    
    /* Couleurs modernes avec dégradé */
    if (hover) {
        button_bg.r = bg.r + 40;
        button_bg.g = bg.g + 40;
        button_bg.b = bg.b + 40;
        button_bg.a = bg.a;
        button_fg = fg;
    } else {
        button_bg = bg;
        button_fg = fg;
    }
    
    /* Ombre portée */
    shadow_color.r = 0;
    shadow_color.g = 0;
    shadow_color.b = 0;
    shadow_color.a = 80;
    
    shadow_rect.x = x + shadow_offset;
    shadow_rect.y = y + shadow_offset;
    shadow_rect.w = w;
    shadow_rect.h = h;
    SDL_SetRenderDrawColor(ctx->renderer, shadow_color.r, shadow_color.g, shadow_color.b, shadow_color.a);
    SDL_RenderFillRect(ctx->renderer, &shadow_rect);
    
    /* Bouton principal */
    button_rect.x = x - animation_offset;
    button_rect.y = y - animation_offset;
    button_rect.w = w;
    button_rect.h = h;
    SDL_SetRenderDrawColor(ctx->renderer, button_bg.r, button_bg.g, button_bg.b, button_bg.a);
    SDL_RenderFillRect(ctx->renderer, &button_rect);
    
    /* Bordure moderne */
    SDL_Color border_color;
    border_color.r = 255; border_color.g = 255; border_color.b = 255; border_color.a = 100;
    SDL_SetRenderDrawColor(ctx->renderer, border_color.r, border_color.g, border_color.b, border_color.a);
    SDL_RenderDrawRect(ctx->renderer, &button_rect);
    
    /* Texte parfaitement centré et visible */
    if (text) {
        int text_width, text_height, text_x, text_y;
        text_width = strlen(text) * 12; /* Estimation plus précise pour la largeur */
        text_height = 24; /* Hauteur plus grande pour la lisibilité */
        text_x = x + (w - text_width) / 2;
        text_y = y + (h - text_height) / 2;
        
        /* Assurer que le texte reste dans les limites du bouton */
        if (text_x < x + 10) text_x = x + 10;
        if (text_y < y + 10) text_y = y + 10;
        if (text_x + text_width > x + w - 10) text_x = x + w - text_width - 10;
        if (text_y + text_height > y + h - 10) text_y = y + h - text_height - 10;
        
        /* Dessiner une ombre pour la lisibilité */
        SDL_Color shadow_text;
        shadow_text.r = 0; shadow_text.g = 0; shadow_text.b = 0; shadow_text.a = 150;
        draw_text(ctx, text_x + 2, text_y + 2, text, shadow_text);
        
        /* Dessiner le texte principal */
        draw_text(ctx, text_x, text_y, text, button_fg);
    }
}

int point_in_rect(int x, int y, int rect_x, int rect_y, int rect_w, int rect_h) {
    return (x >= rect_x && x <= rect_x + rect_w && y >= rect_y && y <= rect_y + rect_h);
}

void draw_progress_bar(GraphiqueContext *ctx, int x, int y, int w, int h, int percent, const char *message) {
    SDL_Color bg_color = {30, 30, 30, 255};
    SDL_Color progress_color = {100, 200, 255, 255};
    SDL_Color text_color = {255, 255, 255, 255};
    SDL_Color border_color = {60, 60, 60, 255};
    SDL_Color glow_color = {150, 220, 255, 100};
    SDL_Rect bg_rect, progress_rect, glow_rect;
    char percent_text[32];
    int glow_width;
    
    if (!ctx || !ctx->renderer) return;
    
    /* Limiter le pourcentage entre 0 et 100 */
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    
    /* Animation de la barre */
    animation_frame = (animation_frame + 1) % 60;
    
    /* Fond de la barre de progression avec ombre */
    SDL_Rect shadow_rect;
    shadow_rect.x = x + 2; shadow_rect.y = y + 2; shadow_rect.w = w; shadow_rect.h = h;
    SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 100);
    SDL_RenderFillRect(ctx->renderer, &shadow_rect);
    
    bg_rect.x = x;
    bg_rect.y = y;
    bg_rect.w = w;
    bg_rect.h = h;
    SDL_SetRenderDrawColor(ctx->renderer, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
    SDL_RenderFillRect(ctx->renderer, &bg_rect);
    
    /* Barre de progression avec animation */
    if (percent > 0) {
        progress_rect.x = x + 3;
        progress_rect.y = y + 3;
        progress_rect.w = (w - 6) * percent / 100;
        progress_rect.h = h - 6;
        
        /* Effet de brillance animé */
        glow_width = progress_rect.w * (50 + 30 * sin(animation_frame * 0.1)) / 100;
        if (glow_width > 0) {
            glow_rect.x = progress_rect.x;
            glow_rect.y = progress_rect.y;
            glow_rect.w = glow_width;
            glow_rect.h = progress_rect.h;
            SDL_SetRenderDrawColor(ctx->renderer, glow_color.r, glow_color.g, glow_color.b, glow_color.a);
            SDL_RenderFillRect(ctx->renderer, &glow_rect);
        }
        
        SDL_SetRenderDrawColor(ctx->renderer, progress_color.r, progress_color.g, progress_color.b, progress_color.a);
        SDL_RenderFillRect(ctx->renderer, &progress_rect);
    }
    
    /* Bordure moderne */
    SDL_SetRenderDrawColor(ctx->renderer, border_color.r, border_color.g, border_color.b, border_color.a);
    SDL_RenderDrawRect(ctx->renderer, &bg_rect);
    
    /* Texte du pourcentage avec animation */
    sprintf(percent_text, "%d%%", percent);
    SDL_Color animated_text = text_color;
    animated_text.a = 200 + 55 * sin(animation_frame * 0.2);
    draw_text(ctx, x + w/2 - 20, y + h/2 - 8, percent_text, animated_text);
    
    /* Message avec effet de fade */
    if (message && strlen(message) > 0) {
        SDL_Color message_color = text_color;
        message_color.a = 180;
        draw_text(ctx, x, y - 30, message, message_color);
    }
}

void draw_file_browser(GraphiqueContext *ctx, char *current_dir, char **selected_files, int *nb_selected) {
    /* Ne dessiner que si le navigateur est ouvert */
    if (!file_browser_open || !ctx || !ctx->renderer) {
                            return;
                        }

    /* Couleurs modernes et professionnelles */
    SDL_Color bg_color, header_color, border_color, file_color, selected_color, dir_color;
    SDL_Color compressible_color, archive_color, button_color, button_hover, button_text;
    
    bg_color.r = 35; bg_color.g = 35; bg_color.b = 45; bg_color.a = 255;
    header_color.r = 50; header_color.g = 50; header_color.b = 60; header_color.a = 255;
    border_color.r = 80; border_color.g = 80; border_color.b = 90; border_color.a = 255;
    file_color.r = 220; file_color.g = 220; file_color.b = 220; file_color.a = 255;
    selected_color.r = 100; selected_color.g = 150; selected_color.b = 255; selected_color.a = 255;
    dir_color.r = 255; dir_color.g = 200; dir_color.b = 100; dir_color.a = 255;
    compressible_color.r = 100; compressible_color.g = 255; compressible_color.b = 150; compressible_color.a = 255;
    archive_color.r = 255; archive_color.g = 165; archive_color.b = 0; archive_color.a = 255;
    button_color.r = 70; button_color.g = 70; button_color.b = 80; button_color.a = 255;
    button_hover.r = 90; button_hover.g = 90; button_hover.b = 100; button_hover.a = 255;
    button_text.r = 255; button_text.g = 255; button_text.b = 255; button_text.a = 255;
    
    /* Rectangle principal du navigateur avec ombre */
    SDL_Rect shadow_rect;
    shadow_rect.x = 52; shadow_rect.y = 122; shadow_rect.w = WINDOW_WIDTH - 96; shadow_rect.h = WINDOW_HEIGHT - 196;
    SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 120);
    SDL_RenderFillRect(ctx->renderer, &shadow_rect);
    
    SDL_Rect browser_rect;
    browser_rect.x = 50; browser_rect.y = 120; browser_rect.w = WINDOW_WIDTH - 100; browser_rect.h = WINDOW_HEIGHT - 200;
    SDL_SetRenderDrawColor(ctx->renderer, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
    SDL_RenderFillRect(ctx->renderer, &browser_rect);
    
    /* Bordure du navigateur */
    SDL_SetRenderDrawColor(ctx->renderer, border_color.r, border_color.g, border_color.b, border_color.a);
    SDL_RenderDrawRect(ctx->renderer, &browser_rect);
    
    /* Header du navigateur */
    SDL_Rect header_rect = {50, 120, WINDOW_WIDTH - 100, 50};
    SDL_SetRenderDrawColor(ctx->renderer, header_color.r, header_color.g, header_color.b, header_color.a);
    SDL_RenderFillRect(ctx->renderer, &header_rect);
    
    /* Titre du navigateur */
    char title[256];
    sprintf(title, "[DIR] %s", current_dir);
    draw_text(ctx, 60, 135, title, file_color);
    
    /* Barre d'outils avec boutons de navigation */
    int button_width = 80;
    int button_height = 30;
    int button_spacing = 10;
    int start_x = 60;
    int start_y = 150;
    
    /* Bouton Retour */
    SDL_Rect back_rect;
    back_rect.x = start_x; back_rect.y = start_y; back_rect.w = button_width; back_rect.h = button_height;
    SDL_SetRenderDrawColor(ctx->renderer, button_color.r, button_color.g, button_color.b, button_color.a);
    SDL_RenderFillRect(ctx->renderer, &back_rect);
    SDL_SetRenderDrawColor(ctx->renderer, border_color.r, border_color.g, border_color.b, border_color.a);
    SDL_RenderDrawRect(ctx->renderer, &back_rect);
    draw_text(ctx, start_x + 25, start_y + 8, "<-", button_text);
    
    /* Bouton Home */
    SDL_Rect home_rect;
    home_rect.x = start_x + button_width + button_spacing; home_rect.y = start_y; home_rect.w = button_width; home_rect.h = button_height;
    SDL_SetRenderDrawColor(ctx->renderer, button_color.r, button_color.g, button_color.b, button_color.a);
    SDL_RenderFillRect(ctx->renderer, &home_rect);
    SDL_SetRenderDrawColor(ctx->renderer, border_color.r, border_color.g, border_color.b, border_color.a);
    SDL_RenderDrawRect(ctx->renderer, &home_rect);
    draw_text(ctx, start_x + button_width + button_spacing + 25, start_y + 8, "HOME", button_text);
    
    /* Bouton Actualiser */
    SDL_Rect refresh_rect;
    refresh_rect.x = start_x + 2 * (button_width + button_spacing); refresh_rect.y = start_y; refresh_rect.w = button_width; refresh_rect.h = button_height;
    SDL_SetRenderDrawColor(ctx->renderer, button_color.r, button_color.g, button_color.b, button_color.a);
    SDL_RenderFillRect(ctx->renderer, &refresh_rect);
    SDL_SetRenderDrawColor(ctx->renderer, border_color.r, border_color.g, border_color.b, border_color.a);
    SDL_RenderDrawRect(ctx->renderer, &refresh_rect);
    draw_text(ctx, start_x + 2 * (button_width + button_spacing) + 25, start_y + 8, "REFRESH", button_text);
    
    /* Bouton Racine */
    SDL_Rect root_rect;
    root_rect.x = start_x + 3 * (button_width + button_spacing); root_rect.y = start_y; root_rect.w = button_width; root_rect.h = button_height;
    SDL_SetRenderDrawColor(ctx->renderer, button_color.r, button_color.g, button_color.b, button_color.a);
    SDL_RenderFillRect(ctx->renderer, &root_rect);
    SDL_SetRenderDrawColor(ctx->renderer, border_color.r, border_color.g, border_color.b, border_color.a);
    SDL_RenderDrawRect(ctx->renderer, &root_rect);
    draw_text(ctx, start_x + 3 * (button_width + button_spacing) + 25, start_y + 8, "[DIR]", button_text);
    
    /* Bouton Bureau */
    SDL_Rect desktop_rect;
    desktop_rect.x = start_x + 4 * (button_width + button_spacing); desktop_rect.y = start_y; desktop_rect.w = button_width; desktop_rect.h = button_height;
    SDL_SetRenderDrawColor(ctx->renderer, button_color.r, button_color.g, button_color.b, button_color.a);
    SDL_RenderFillRect(ctx->renderer, &desktop_rect);
    SDL_SetRenderDrawColor(ctx->renderer, border_color.r, border_color.g, border_color.b, border_color.a);
    SDL_RenderDrawRect(ctx->renderer, &desktop_rect);
    draw_text(ctx, start_x + 4 * (button_width + button_spacing) + 25, start_y + 8, "SAVE", button_text);
    
    /* Zone de fichiers avec scrollbar */
    SDL_Rect files_rect = {50, 190, WINDOW_WIDTH - 120, WINDOW_HEIGHT - 280};
    SDL_SetRenderDrawColor(ctx->renderer, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
    SDL_RenderFillRect(ctx->renderer, &files_rect);
    
    /* Bordure de la zone de fichiers */
    SDL_SetRenderDrawColor(ctx->renderer, border_color.r, border_color.g, border_color.b, border_color.a);
    SDL_RenderDrawRect(ctx->renderer, &files_rect);
    
    /* Lire le contenu du dossier */
    DIR *dir = opendir(current_dir);
    if (dir) {
        struct dirent *entry;
        int file_count = 0;
        int max_files = 20;
        char **file_list = (char**)malloc(max_files * sizeof(char*));
        int i;
        
        /* Allouer la mémoire pour les noms de fichiers */
        for (i = 0; i < max_files; i++) {
            file_list[i] = (char*)malloc(256 * sizeof(char));
        }
        
        /* Lire les fichiers */
        while ((entry = readdir(dir)) != NULL && file_count < max_files) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                strcpy(file_list[file_count], entry->d_name);
                file_count++;
            }
        }
        closedir(dir);
        
        /* Afficher les fichiers */
        int start_index, files_to_show;
        start_index = file_browser_scroll;
        files_to_show = (file_count - start_index < 15) ? file_count - start_index : 15;
        
        for (i = 0; i < files_to_show; i++) {
            int file_index = start_index + i;
            int file_y = 200 + i * 25;
            int file_height = 20;
            
            /* Vérifier si le fichier est sélectionné */
            int is_selected = 0;
            int j;
            for (j = 0; j < *nb_selected; j++) {
                if (strcmp(selected_files[j], file_list[file_index]) == 0) {
                    is_selected = 1;
                    break;
                }
            }
            
            /* Couleur selon le type de fichier */
            SDL_Color text_color;
            struct stat file_stat;
            char full_path[512];
            text_color = file_color;
            sprintf(full_path, "%s/%s", current_dir, file_list[file_index]);
            
            if (stat(full_path, &file_stat) == 0) {
                if (S_ISDIR(file_stat.st_mode)) {
                    text_color = dir_color;
                } else {
                    char *ext = strrchr(file_list[file_index], '.');
                    if (ext) {
                        if (strcmp(ext, ".txt") == 0 || strcmp(ext, ".c") == 0 || strcmp(ext, ".h") == 0) {
                            text_color = compressible_color;
                        } else if (strcmp(ext, ".huff") == 0 || strcmp(ext, ".zip") == 0 || strcmp(ext, ".rar") == 0) {
                            text_color = archive_color;
                        }
                    }
                }
            }
            
            /* Fond de sélection */
            if (is_selected) {
                SDL_Rect selection_rect;
                selection_rect.x = 55; selection_rect.y = file_y - 1; selection_rect.w = files_rect.w - 10; selection_rect.h = file_height - 2;
                SDL_SetRenderDrawColor(ctx->renderer, selected_color.r, selected_color.g, selected_color.b, 100);
                SDL_RenderFillRect(ctx->renderer, &selection_rect);
            }
            
            /* Icône selon le type */
            char icon[10];
            if (S_ISDIR(file_stat.st_mode)) {
                strcpy(icon, "[DIR] ");
            } else {
                char *ext = strrchr(file_list[file_index], '.');
                if (ext) {
                    if (strcmp(ext, ".txt") == 0) strcpy(icon, "[TXT] ");
                    else if (strcmp(ext, ".c") == 0 || strcmp(ext, ".h") == 0) strcpy(icon, "[C] ");
                    else if (strcmp(ext, ".huff") == 0) strcpy(icon, "[HUFF] ");
                    else if (strcmp(ext, ".zip") == 0 || strcmp(ext, ".rar") == 0) strcpy(icon, "[ZIP] ");
                    else strcpy(icon, "[FILE] ");
                } else {
                    strcpy(icon, "[FILE] ");
                }
            }
            
            /* Afficher le fichier */
            char display_name[300];
            sprintf(display_name, "%s%s", icon, file_list[file_index]);
            draw_text(ctx, 60, file_y, display_name, text_color);
        }
        
            /* Libérer la mémoire */
    for (i = 0; i < max_files; i++) {
        free(file_list[i]);
    }
    free(file_list);
}

/* ============================================================================
 * SÉLECTEUR DE FICHIERS INTÉGRÉ
 * ============================================================================ */

    
    /* Boutons de validation modernes */
    SDL_Rect validate_rect = {WINDOW_WIDTH - 200, WINDOW_HEIGHT - 80, 140, 40};
    SDL_SetRenderDrawColor(ctx->renderer, 100, 200, 100, 255);
    SDL_RenderFillRect(ctx->renderer, &validate_rect);
    SDL_SetRenderDrawColor(ctx->renderer, border_color.r, border_color.g, border_color.b, border_color.a);
    SDL_RenderDrawRect(ctx->renderer, &validate_rect);
    draw_text(ctx, WINDOW_WIDTH - 180, WINDOW_HEIGHT - 70, "[OK] VALIDER", button_text);
    
    SDL_Rect cancel_rect = {WINDOW_WIDTH - 350, WINDOW_HEIGHT - 80, 140, 40};
    SDL_SetRenderDrawColor(ctx->renderer, 200, 100, 100, 255);
    SDL_RenderFillRect(ctx->renderer, &cancel_rect);
    SDL_SetRenderDrawColor(ctx->renderer, border_color.r, border_color.g, border_color.b, border_color.a);
    SDL_RenderDrawRect(ctx->renderer, &cancel_rect);
    draw_text(ctx, WINDOW_WIDTH - 330, WINDOW_HEIGHT - 70, "[X] ANNULER", button_text);
}

void draw_statistics(GraphiqueContext *ctx, long original_size, long compressed_size, const char *operation) {
    SDL_Color bg_color = {30, 30, 30, 255};
    SDL_Color text_color = {255, 255, 255, 255};
    SDL_Color highlight_color = {100, 200, 255, 255};
    SDL_Color border_color = {100, 100, 100, 255};
    SDL_Rect stats_rect;
    char original_text[100];
    char compressed_text[100];
    char ratio_text[100];
    char saved_text[100];
    char op_text[100];
    double ratio;
    long saved;
    
    if (!ctx || !ctx->renderer) return;
    
    /* Zone des statistiques */
    stats_rect.x = WINDOW_WIDTH - 300;
    stats_rect.y = 100;
    stats_rect.w = 250;
    stats_rect.h = 200;
    SDL_SetRenderDrawColor(ctx->renderer, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
    SDL_RenderFillRect(ctx->renderer, &stats_rect);
    SDL_SetRenderDrawColor(ctx->renderer, border_color.r, border_color.g, border_color.b, border_color.a);
    SDL_RenderDrawRect(ctx->renderer, &stats_rect);
    
    /* Titre */
    draw_text(ctx, stats_rect.x + 10, stats_rect.y + 10, "Statistiques", highlight_color);
    
    /* Taille originale */
    sprintf(original_text, "Taille originale: %ld octets", original_size);
    draw_text(ctx, stats_rect.x + 10, stats_rect.y + 40, original_text, text_color);
    
    /* Taille compressée */
    sprintf(compressed_text, "Taille finale: %ld octets", compressed_size);
    draw_text(ctx, stats_rect.x + 10, stats_rect.y + 70, compressed_text, text_color);
    
    /* Ratio de compression */
    if (original_size > 0) {
        ratio = (double)compressed_size / original_size * 100;
        sprintf(ratio_text, "Ratio: %.1f%%", ratio);
        draw_text(ctx, stats_rect.x + 10, stats_rect.y + 100, ratio_text, text_color);
        
        /* Économie */
        saved = original_size - compressed_size;
        sprintf(saved_text, "Economie: %ld octets", saved);
        draw_text(ctx, stats_rect.x + 10, stats_rect.y + 130, saved_text, text_color);
    }
    
    /* Opération */
    sprintf(op_text, "Operation: %s", operation);
    draw_text(ctx, stats_rect.x + 10, stats_rect.y + 160, op_text, highlight_color);
}

void navigate_directory(char *new_dir) {
    if (strlen(new_dir) < sizeof(current_directory)) {
        strcpy(current_directory, new_dir);
    }
}

void reset_interface_state(void) {
    nb_selected_files = 0;
    strcpy(archive_name, "");
    strcpy(output_filename, "");
    progress_percent = 0;
    strcpy(progress_message, "");
    show_progress = 0;
    total_original_size = 0;
    total_compressed_size = 0;
    file_browser_scroll = 0;
    file_browser_open = 0;
    text_input_active = 0;
    text_input_mode = 0;
    show_warning = 0;
    strcpy(warning_message, "");
}

void menu(GraphiqueContext *ctx) {
    SDL_Event event;
    SDL_Color bg = {15, 15, 25, 255}; /* Fond encore plus sombre et moderne */
    SDL_Color title_color = {255, 255, 255, 255};
    SDL_Color subtitle_color = {180, 180, 200, 255}; /* Plus lumineux */
    /* SDL_Color blue = {100, 150, 255, 255}; */ /* Variables inutilisées */
    /* SDL_Color green = {100, 255, 150, 255}; */
    /* SDL_Color red = {255, 100, 100, 255}; */
    SDL_Color accent = {255, 200, 100, 255};
    SDL_Color gradient_start = {30, 30, 50, 255}; /* Dégradé moderne */
    SDL_Color gradient_end = {20, 20, 40, 255};

    int compress_hover = 0, decompress_hover = 0, quit_hover = 0;
    
    while (ctx->running && current_mode >= 0) {
        /* Gérer les différents modes */
        if (current_mode == 1) {
            menu_compression(ctx, 0, NULL, NULL);
        } else if (current_mode == 2) {
            menu_decompression(ctx, 0, NULL);
        } else if (current_mode == 0) {
            /* Mode menu principal */
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                        ctx->running = 0;
            } else if (event.type == SDL_MOUSEMOTION) {
                int mouse_x = event.motion.x;
                int mouse_y = event.motion.y;
                
                compress_hover = point_in_rect(mouse_x, mouse_y, WINDOW_WIDTH/2 - 450, 250, 280, 80);
                decompress_hover = point_in_rect(mouse_x, mouse_y, WINDOW_WIDTH/2 - 140, 250, 280, 80);
                quit_hover = point_in_rect(mouse_x, mouse_y, WINDOW_WIDTH/2 + 170, 250, 280, 80);
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mouse_x = event.button.x;
                int mouse_y = event.button.y;
                
                if (point_in_rect(mouse_x, mouse_y, WINDOW_WIDTH/2 - 450, 250, 280, 80)) {
                    current_mode = 1; /* Mode compression */
                    reset_interface_state();
                    menu_compression(ctx, 0, NULL, NULL); /* Appel direct du menu compression */
                } else if (point_in_rect(mouse_x, mouse_y, WINDOW_WIDTH/2 - 140, 250, 280, 80)) {
                    current_mode = 2; /* Mode décompression */
                    reset_interface_state();
                    menu_decompression(ctx, 0, NULL); /* Appel direct du menu décompression */
                } else if (point_in_rect(mouse_x, mouse_y, WINDOW_WIDTH/2 + 170, 250, 280, 80)) {
                    ctx->running = 0;
                }
            }
        }
        
        clear_window(ctx, bg);

        /* Animation de fond avec dégradé moderne */
        animation_frame = (animation_frame + 1) % 360;
        
        /* Dégradé de fond moderne */
        int y;
        for (y = 0; y < WINDOW_HEIGHT; y++) {
            float ratio = (float)y / WINDOW_HEIGHT;
            SDL_Color current_color;
            current_color.r = gradient_start.r + (gradient_end.r - gradient_start.r) * ratio;
            current_color.g = gradient_start.g + (gradient_end.g - gradient_start.g) * ratio;
            current_color.b = gradient_start.b + (gradient_end.b - gradient_start.b) * ratio;
            current_color.a = 255;
            
            SDL_SetRenderDrawColor(ctx->renderer, current_color.r, current_color.g, current_color.b, current_color.a);
            SDL_RenderDrawLine(ctx->renderer, 0, y, WINDOW_WIDTH, y);
        }
        
        /* Titre principal avec effet */
        SDL_Color animated_title = title_color;
        animated_title.a = 200 + 55 * sin(animation_frame * 0.05);
        draw_text(ctx, WINDOW_WIDTH/2 - 70, 80, "HUFFMAN COMPRESSOR", animated_title);
        
        /* Sous-titre */
        draw_text(ctx, WINDOW_WIDTH/2 - 110, 120, "Compression & Decompression", subtitle_color);
        
        /* Ligne décorative animée */
        SDL_Rect line_rect = {WINDOW_WIDTH/2 - 250, 160, 500, 3};
        SDL_SetRenderDrawColor(ctx->renderer, accent.r, accent.g, accent.b, accent.a);
        SDL_RenderFillRect(ctx->renderer, &line_rect);
        
        /* Boutons ultra-modernes avec effets visuels - mieux centrés */
        SDL_Color compress_bg;
        compress_bg.r = compress_hover ? 50 : 60; compress_bg.g = compress_hover ? 100 : 80; compress_bg.b = compress_hover ? 180 : 120; compress_bg.a = 255;
        draw_button(ctx, WINDOW_WIDTH/2 - 450, 250, 280, 80, "COMPRESSION", 
                   title_color, compress_bg, compress_hover);
        
        SDL_Color decompress_bg;
        decompress_bg.r = decompress_hover ? 50 : 80; decompress_bg.g = decompress_hover ? 120 : 100; decompress_bg.b = decompress_hover ? 80 : 110; decompress_bg.a = 255;
        draw_button(ctx, WINDOW_WIDTH/2 - 140, 250, 280, 80, "DECOMPRESSION", 
                   title_color, decompress_bg, decompress_hover);
        
        SDL_Color quit_bg;
        quit_bg.r = quit_hover ? 180 : 110; quit_bg.g = quit_hover ? 60 : 80; quit_bg.b = quit_hover ? 60 : 80; quit_bg.a = 255;
        draw_button(ctx, WINDOW_WIDTH/2 + 170, 250, 280, 80, "QUITTER", 
                   title_color, quit_bg, quit_hover);
        
        /* Informations en bas avec animation */
        SDL_Color info_color = subtitle_color;
        info_color.a = 150 + 50 * sin(animation_frame * 0.1);
        draw_text(ctx, WINDOW_WIDTH/2 - 150, WINDOW_HEIGHT - 100, "Version 5.0 - Interface Graphique", info_color);
        draw_text(ctx, WINDOW_WIDTH/2 - 150, WINDOW_HEIGHT - 50, "Ghodbane Rachid - 2025", info_color);
        
        update_window(ctx);
        SDL_Delay(16); /* ~60 FPS */
        } /* Fin du mode menu principal */
        
        /* Si current_mode == -1, sortir de la boucle */
        if (current_mode == -1) {
            ctx->running = 0;
        }
    }
}

/* Fonctions existantes simplifiées pour la démonstration */
int fichier_present(char **liste_fichiers, char *nom_fichier, int nb_fichiers) {
    int i;
    for (i = 0; i < nb_fichiers; i++) {
        if (strcmp(liste_fichiers[i], nom_fichier) == 0) {
            return 1;
        }
    }
    return 0;
}

char **liste_fichiers(int *nb_fichiers) {
    static char *files[100];
    DIR *dir;
    *nb_fichiers = 0;
    
    dir = opendir(current_directory);
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL && *nb_fichiers < 100) {
            if (entry->d_name[0] != '.') {
                files[*nb_fichiers] = entry->d_name;
                (*nb_fichiers)++;
            }
        }
        closedir(dir);
    }
    
    return files;
}

void menu_compression(GraphiqueContext *ctx, int indice_depart, char **selected, int *nb_selected) {
    SDL_Event event;
    (void)indice_depart; /* Éviter le warning unused parameter */
    (void)selected; /* Éviter le warning unused parameter */
    (void)nb_selected; /* Éviter le warning unused parameter */
    SDL_Color bg = {25, 25, 35, 255}; /* Fond sombre moderne */
    SDL_Color black = {255, 255, 255, 255}; /* Texte blanc */
    SDL_Color green = {100, 255, 150, 255};
    /* SDL_Color accent = {255, 200, 100, 255}; */ /* Variable inutilisée */
    
    int compress_hover = 0, back_hover = 0;
    /* int nav_back_hover = 0, nav_home_hover = 0, nav_refresh_hover = 0; */ /* Variables inutilisées */
    
    while (ctx->running && current_mode == 1) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                ctx->running = 0;
            } else if (event.type == SDL_MOUSEMOTION) {
                int mouse_x = event.motion.x;
                int mouse_y = event.motion.y;
                
                compress_hover = point_in_rect(mouse_x, mouse_y, 50, WINDOW_HEIGHT - 100, 250, 60);
                back_hover = point_in_rect(mouse_x, mouse_y, 350, WINDOW_HEIGHT - 100, 250, 60);
                /* nav_back_hover = point_in_rect(mouse_x, mouse_y, 60, 150, 80, 30); */ /* Variables inutilisées */
                /* nav_home_hover = point_in_rect(mouse_x, mouse_y, 150, 150, 80, 30); */
                /* nav_refresh_hover = point_in_rect(mouse_x, mouse_y, 240, 150, 80, 30); */
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mouse_x = event.button.x;
                int mouse_y = event.button.y;
                
                /* Bouton pour ouvrir le sélecteur de fichiers */
                if (point_in_rect(mouse_x, mouse_y, 50, 150, 250, 50)) {
                    if (open_compression_file_selector(ctx)) {
                        /* Fichiers sélectionnés avec succès */
                        file_browser_open = 0;
                        text_input_active = 1;
                        text_input_mode = 0; /* Mode nom archive */
                    }
                } else if (file_browser_open) {
                    /* Boutons de navigation */
                    if (point_in_rect(mouse_x, mouse_y, 60, 150, 80, 30)) {
                    /* Retour au dossier parent */
                    char parent_dir[512];
                    char *last_slash = strrchr(current_directory, '/');
                    if (last_slash != NULL && last_slash != current_directory) {
                        strncpy(parent_dir, current_directory, last_slash - current_directory);
                        parent_dir[last_slash - current_directory] = '\0';
                        strcpy(current_directory, parent_dir);
                        file_browser_scroll = 0;
                    }
                } else if (point_in_rect(mouse_x, mouse_y, 150, 150, 80, 30)) {
                    /* Aller à l'accueil */
                    strcpy(current_directory, ".");
                    file_browser_scroll = 0;
                } else if (point_in_rect(mouse_x, mouse_y, 240, 150, 80, 30)) {
                    /* Rafraîchir */
                    file_browser_scroll = 0;
                } else if (point_in_rect(mouse_x, mouse_y, 330, 150, 80, 30)) {
                    /* Aller à la racine */
                    strcpy(current_directory, "/");
                    file_browser_scroll = 0;
                } else if (point_in_rect(mouse_x, mouse_y, 420, 150, 80, 30)) {
                    /* Aller au bureau */
                    char *home = getenv("HOME");
                    if (home) {
                        strcpy(current_directory, home);
                        strcat(current_directory, "/Desktop");
                    } else {
                        strcpy(current_directory, ".");
                    }
                    file_browser_scroll = 0;
                } else if (point_in_rect(mouse_x, mouse_y, 50, 160, WINDOW_WIDTH - 150, WINDOW_HEIGHT - 370)) {
                    /* Clic dans la zone de fichiers */
                    int file_index = (mouse_y - 160) / 25 + file_browser_scroll;
                    
                    /* Obtenir la liste des fichiers pour la sélection */
                    DIR *dir = opendir(current_directory);
                    if (dir) {
                        struct dirent *entry;
                        char **file_list = (char**)malloc(1000 * sizeof(char*));
                        int file_count = 0;
                        int i = 0;
                        
                        /* Collecter tous les fichiers */
                        while ((entry = readdir(dir)) != NULL && file_count < 1000) {
                            if (entry->d_name[0] == '.') continue;
                            file_list[file_count] = (char*)malloc(strlen(entry->d_name) + 1);
                            strcpy(file_list[file_count], entry->d_name);
                            file_count++;
                        }
                        closedir(dir);
                        
                        /* Sélectionner le fichier cliqué */
                        if (file_index >= 0 && file_index < file_count) {
                            char full_path[1024];
                            sprintf(full_path, "%s/%s", current_directory, file_list[file_index]);
                            
                            /* Vérifier si c'est un fichier compressible */
                            char *ext;
                            ext = strrchr(file_list[file_index], '.');
                            int is_compressible = 0;
                            if (ext != NULL) {
                                if (strcmp(ext, ".txt") == 0 || strcmp(ext, ".c") == 0 || strcmp(ext, ".h") == 0) {
                                    is_compressible = 1;
                                }
                            }
                            
                            if (is_compressible) {
                                /* Ajouter ou retirer de la sélection */
                                int already_selected = 0;
                                int j;
                                for (j = 0; j < nb_selected_files; j++) {
                                    if (strcmp(selected_files[j], full_path) == 0) {
                                        already_selected = 1;
                                        /* Retirer de la sélection */
                                        int k;
                                        for (k = j; k < nb_selected_files - 1; k++) {
                                            strcpy(selected_files[k], selected_files[k+1]);
                                        }
                                        nb_selected_files--;
                                        break;
                                    }
                                }
                                
                                if (!already_selected && nb_selected_files < 50) {
                                    /* Ajouter à la sélection */
                                    strcpy(selected_files[nb_selected_files], full_path);
                                    nb_selected_files++;
                                    /* Fermer le navigateur après sélection */
                                    file_browser_open = 0;
                                }
                            }
                        }
                        
                        /* Libérer la mémoire */
                        for (i = 0; i < file_count; i++) {
                            free(file_list[i]);
                        }
                        free(file_list);
                    }
                    
                    /* Boutons de validation */
                    if (point_in_rect(mouse_x, mouse_y, WINDOW_WIDTH - 200, WINDOW_HEIGHT - 80, 140, 40)) {
                        /* Bouton VALIDER */
                        file_browser_open = 0;
                    } else if (point_in_rect(mouse_x, mouse_y, WINDOW_WIDTH - 350, WINDOW_HEIGHT - 80, 140, 40)) {
                        /* Bouton ANNULER */
                        file_browser_open = 0;
                    }
                    }
                } else if (point_in_rect(mouse_x, mouse_y, 200, WINDOW_HEIGHT - 150, 300, 30)) {
                    /* Clic sur la zone de saisie du nom d'archive */
                    text_input_active = 1;
                    text_input_mode = 0;
                } else if (point_in_rect(mouse_x, mouse_y, 50, WINDOW_HEIGHT - 100, 250, 60)) {
                    if (nb_selected_files > 0 && strlen(archive_name) > 0) {
                        /* Vérifier si le fichier existe déjà */
                        char full_archive_path[512];
                        sprintf(full_archive_path, "%s.huff", archive_name);
                        
                        if (access(full_archive_path, F_OK) == 0) {
                            /* Le fichier existe, afficher un avertissement */
                            show_warning = 1;
                            sprintf(warning_message, "Le fichier %s existe deja. Voulez-vous l'ecraser ?", full_archive_path);
                        } else {
                            /* Le fichier n'existe pas, procéder à la compression */
                            perform_compression(ctx);
                        }
                    }
                } else if (point_in_rect(mouse_x, mouse_y, 350, WINDOW_HEIGHT - 100, 250, 60)) {
                    current_mode = 0; /* Retour au menu principal */
                } else if (show_warning && point_in_rect(mouse_x, mouse_y, 60, WINDOW_HEIGHT - 180, 100, 30)) {
                    /* Clic sur OUI - écraser le fichier */
                    show_warning = 0;
                    perform_compression(ctx);
                } else if (show_warning && point_in_rect(mouse_x, mouse_y, 180, WINDOW_HEIGHT - 180, 100, 30)) {
                    /* Clic sur NON - annuler */
                    show_warning = 0;
                } else {
                    /* Clic ailleurs, désactiver la saisie de texte */
                    text_input_active = 0;
                }
            } else if (event.type == SDL_MOUSEWHEEL) {
                /* Gestion du scroll */
                if (event.wheel.y > 0 && file_browser_scroll > 0) {
                    file_browser_scroll--;
                } else if (event.wheel.y < 0) {
                    file_browser_scroll++;
                }
            } else if (event.type == SDL_KEYDOWN) {
                /* Gestion de la saisie de texte */
                if (text_input_active && text_input_mode == 0) {
                    if (event.key.keysym.sym == SDLK_BACKSPACE) {
                        int len = strlen(archive_name);
                        if (len > 0) {
                            archive_name[len - 1] = '\0';
                        }
                    } else if (event.key.keysym.sym == SDLK_RETURN) {
                        text_input_active = 0;
                    } else if (event.key.keysym.sym == SDLK_ESCAPE) {
                        text_input_active = 0;
                    }
                }
            } else if (event.type == SDL_TEXTINPUT) {
                /* Saisie de caractères */
                if (text_input_active && text_input_mode == 0) {
                    int len = strlen(archive_name);
                    if (len < 255) {
                        archive_name[len] = event.text.text[0];
                        archive_name[len + 1] = '\0';
                    }
                }
            }
        }
        
        clear_window(ctx, bg);
        
        /* Titre */
        draw_text(ctx, 50, 20, "Mode Compression", black);
        
        /* Bouton pour ouvrir le sélecteur de fichiers */
        SDL_Color select_bg;
        select_bg.r = 100; select_bg.g = 150; select_bg.b = 255; select_bg.a = 255;
        draw_button(ctx, 50, 150, 250, 50, "Selectionner des fichiers", 
                   black, select_bg, 0);
        
        /* Navigateur de fichiers (seulement si ouvert) */
        draw_file_browser(ctx, current_directory, (char**)selected_files, &nb_selected_files);
        
        /* Affichage des fichiers sélectionnés */
        int i;
        if (nb_selected_files > 0) {
            draw_text(ctx, 50, 200, "Fichiers selectionnes:", black);
            for (i = 0; i < nb_selected_files && i < 5; i++) {
                char file_display[100];
                char *filename = strrchr(selected_files[i], '/');
                if (filename) filename++;
                else filename = selected_files[i];
                sprintf(file_display, "- %s", filename);
                draw_text(ctx, 60, 220 + i * 20, file_display, green);
            }
            if (nb_selected_files > 5) {
                char more_text[50];
                sprintf(more_text, "... et %d autres", nb_selected_files - 5);
                draw_text(ctx, 60, 220 + 5 * 20, more_text, black);
            }
        }
        
        /* Zone de saisie du nom d'archive (seulement si des fichiers sont sélectionnés) */
        if (nb_selected_files > 0) {
            draw_text(ctx, 50, WINDOW_HEIGHT - 150, "Nom de l'archive:", black);
            
            /* Zone de saisie moderne */
            SDL_Color input_bg, input_border, placeholder_color;
            if (text_input_active) {
                input_bg.r = 50; input_bg.g = 50; input_bg.b = 60; input_bg.a = 255;
                input_border.r = 100; input_border.g = 150; input_border.b = 255; input_border.a = 255;
            } else {
                input_bg.r = 40; input_bg.g = 40; input_bg.b = 50; input_bg.a = 255;
                input_border.r = 80; input_border.g = 80; input_border.b = 90; input_border.a = 255;
            }
            placeholder_color.r = 150; placeholder_color.g = 150; placeholder_color.b = 150; placeholder_color.a = 255;
            
            /* Ombre de la zone de saisie */
            SDL_Rect shadow_rect = {202, WINDOW_HEIGHT - 148, 400, 35};
            SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 100);
            SDL_RenderFillRect(ctx->renderer, &shadow_rect);
            
            /* Fond de la zone de saisie */
            SDL_Rect input_rect = {200, WINDOW_HEIGHT - 150, 400, 35};
            SDL_SetRenderDrawColor(ctx->renderer, input_bg.r, input_bg.g, input_bg.b, input_bg.a);
            SDL_RenderFillRect(ctx->renderer, &input_rect);
            SDL_SetRenderDrawColor(ctx->renderer, input_border.r, input_border.g, input_border.b, input_border.a);
            SDL_RenderDrawRect(ctx->renderer, &input_rect);
            
            /* Texte dans la zone de saisie */
            char display_text[300];
            if (strlen(archive_name) == 0) {
                strcpy(display_text, "Entrez le nom de l'archive...");
                draw_text(ctx, 210, WINDOW_HEIGHT - 140, display_text, placeholder_color);
            } else {
                strcpy(display_text, archive_name);
                draw_text(ctx, 210, WINDOW_HEIGHT - 140, display_text, black);
            }
            
            /* Curseur animé si actif */
            if (text_input_active) {
                int cursor_x;
                SDL_Rect cursor_rect;
                SDL_Color cursor_color;
                cursor_x = 210 + strlen(archive_name) * 8;
                cursor_rect.x = cursor_x; cursor_rect.y = WINDOW_HEIGHT - 140; cursor_rect.w = 2; cursor_rect.h = 16;
                cursor_color.r = 100; cursor_color.g = 150; cursor_color.b = 255; cursor_color.a = 200 + 55 * sin(animation_frame * 0.3);
                SDL_SetRenderDrawColor(ctx->renderer, cursor_color.r, cursor_color.g, cursor_color.b, cursor_color.a);
                SDL_RenderFillRect(ctx->renderer, &cursor_rect);
            }
        }
        
        /* Boutons */
        SDL_Color compress_bg2;
        compress_bg2.r = compress_hover ? 20 : 60; compress_bg2.g = compress_hover ? 60 : 60; compress_bg2.b = compress_hover ? 40 : 70; compress_bg2.a = 255;
        draw_button(ctx, 50, WINDOW_HEIGHT - 100, 250, 60, "COMPRESSER", 
                   black, compress_bg2, compress_hover);
        
        SDL_Color back_bg;
        back_bg.r = back_hover ? 80 : 60; back_bg.g = back_hover ? 20 : 60; back_bg.b = back_hover ? 20 : 70; back_bg.a = 255;
        draw_button(ctx, 350, WINDOW_HEIGHT - 100, 250, 60, "RETOUR", 
                   black, back_bg, back_hover);
        
        /* Affichage de l'avertissement si actif */
        if (show_warning) {
            SDL_Color warning_bg = {255, 240, 240, 255};
            SDL_Color warning_border = {255, 100, 100, 255};
            SDL_Color warning_text = {200, 0, 0, 255};
            
            /* Fond de l'avertissement */
            SDL_Rect warning_rect = {50, WINDOW_HEIGHT - 250, WINDOW_WIDTH - 100, 100};
            SDL_SetRenderDrawColor(ctx->renderer, warning_bg.r, warning_bg.g, warning_bg.b, warning_bg.a);
            SDL_RenderFillRect(ctx->renderer, &warning_rect);
            SDL_SetRenderDrawColor(ctx->renderer, warning_border.r, warning_border.g, warning_border.b, warning_border.b);
            SDL_RenderDrawRect(ctx->renderer, &warning_rect);
            
            /* Texte d'avertissement */
            draw_text(ctx, 60, WINDOW_HEIGHT - 240, "ATTENTION !", warning_text);
            draw_text(ctx, 60, WINDOW_HEIGHT - 220, warning_message, warning_text);
            
            /* Boutons de confirmation */
            SDL_Color yes_bg, no_bg;
            yes_bg.r = 255; yes_bg.g = 200; yes_bg.b = 200; yes_bg.a = 255;
            no_bg.r = 200; no_bg.g = 255; no_bg.b = 200; no_bg.a = 255;
            draw_button(ctx, 60, WINDOW_HEIGHT - 180, 100, 30, "OUI", 
                       black, yes_bg, 0);
            draw_button(ctx, 180, WINDOW_HEIGHT - 180, 100, 30, "NON", 
                       black, no_bg, 0);
        }
        
        /* Affichage de la progression si active */
        if (show_progress) {
            draw_progress_bar(ctx, 50, WINDOW_HEIGHT - 200, 500, 30, progress_percent, progress_message);
        }

        update_window(ctx);
        SDL_Delay(16);
    }
}

void menu_decompression(GraphiqueContext *ctx, int indice_depart, char *selected) {
    SDL_Event event;
    (void)indice_depart; /* Éviter le warning unused parameter */
    (void)selected; /* Éviter le warning unused parameter */
    SDL_Color bg = {25, 25, 35, 255}; /* Fond sombre moderne */
    SDL_Color black = {255, 255, 255, 255}; /* Texte blanc */
    SDL_Color green = {100, 255, 150, 255};
    /* SDL_Color accent = {255, 200, 100, 255}; */ /* Variable inutilisée */
    
    int decompress_hover = 0, back_hover = 0;
    /* int nav_back_hover = 0, nav_home_hover = 0, nav_refresh_hover = 0; */ /* Variables inutilisées */
    
    while (ctx->running && current_mode == 2) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                    ctx->running = 0;
            } else if (event.type == SDL_MOUSEMOTION) {
                int mouse_x = event.motion.x;
                int mouse_y = event.motion.y;
                
                decompress_hover = point_in_rect(mouse_x, mouse_y, 50, WINDOW_HEIGHT - 100, 250, 60);
                back_hover = point_in_rect(mouse_x, mouse_y, 350, WINDOW_HEIGHT - 100, 250, 60);
                /* nav_back_hover = point_in_rect(mouse_x, mouse_y, 60, 150, 80, 30); */ /* Variables inutilisées */
                /* nav_home_hover = point_in_rect(mouse_x, mouse_y, 150, 150, 80, 30); */
                /* nav_refresh_hover = point_in_rect(mouse_x, mouse_y, 240, 150, 80, 30); */
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mouse_x = event.button.x;
                int mouse_y = event.button.y;
                
                /* Bouton pour ouvrir le sélecteur d'archives */
                if (point_in_rect(mouse_x, mouse_y, 50, 150, 250, 50)) {
                    if (open_decompression_file_selector(ctx)) {
                        /* Archive sélectionnée avec succès */
                        file_browser_open = 0;
                        text_input_active = 1;
                        text_input_mode = 1; /* Mode nom sortie décompression */
                    }
                } else if (file_browser_open) {
                    /* Boutons de navigation */
                    if (point_in_rect(mouse_x, mouse_y, 60, 150, 80, 30)) {
                    /* Retour au dossier parent */
                    char parent_dir[512];
                    char *last_slash = strrchr(current_directory, '/');
                    if (last_slash != NULL && last_slash != current_directory) {
                        strncpy(parent_dir, current_directory, last_slash - current_directory);
                        parent_dir[last_slash - current_directory] = '\0';
                        strcpy(current_directory, parent_dir);
                        file_browser_scroll = 0;
                    }
                } else if (point_in_rect(mouse_x, mouse_y, 150, 150, 80, 30)) {
                    /* Aller à l'accueil */
                    strcpy(current_directory, ".");
                    file_browser_scroll = 0;
                } else if (point_in_rect(mouse_x, mouse_y, 240, 150, 80, 30)) {
                    /* Rafraîchir */
                    file_browser_scroll = 0;
                } else if (point_in_rect(mouse_x, mouse_y, 330, 150, 80, 30)) {
                    /* Aller à la racine */
                    strcpy(current_directory, "/");
                    file_browser_scroll = 0;
                } else if (point_in_rect(mouse_x, mouse_y, 420, 150, 80, 30)) {
                    /* Aller au bureau */
                    char *home = getenv("HOME");
                    if (home) {
                        strcpy(current_directory, home);
                        strcat(current_directory, "/Desktop");
                    } else {
                        strcpy(current_directory, ".");
                    }
                    file_browser_scroll = 0;
                } else if (point_in_rect(mouse_x, mouse_y, 50, 160, WINDOW_WIDTH - 150, WINDOW_HEIGHT - 370)) {
                    /* Clic dans la zone de fichiers */
                    int file_index = (mouse_y - 160) / 25 + file_browser_scroll;
                    
                    /* Obtenir la liste des fichiers pour la sélection */
                    DIR *dir = opendir(current_directory);
                    if (dir) {
                        struct dirent *entry;
                        char **file_list = (char**)malloc(1000 * sizeof(char*));
                        int file_count = 0;
                        int i = 0;
                        
                        /* Collecter tous les fichiers */
                        while ((entry = readdir(dir)) != NULL && file_count < 1000) {
                            if (entry->d_name[0] == '.') continue;
                            file_list[file_count] = (char*)malloc(strlen(entry->d_name) + 1);
                            strcpy(file_list[file_count], entry->d_name);
                            file_count++;
                        }
                        closedir(dir);
                        
                        /* Sélectionner le fichier cliqué */
                        if (file_index >= 0 && file_index < file_count) {
                            char full_path[1024];
                            sprintf(full_path, "%s/%s", current_directory, file_list[file_index]);
                            
                            /* Vérifier si c'est une archive */
                            char *ext;
                            int is_archive = 0;
                            ext = strrchr(file_list[file_index], '.');
                            if (ext != NULL) {
                                if (strcmp(ext, ".huff") == 0 || strcmp(ext, ".zip") == 0 || strcmp(ext, ".rar") == 0) {
                                    is_archive = 1;
                                }
                            }
                            
                            if (is_archive) {
                                /* Sélectionner cette archive */
                                strcpy(archive_name, full_path);
                                /* Fermer le navigateur après sélection */
                                file_browser_open = 0;
        }
    }

    /* Libérer la mémoire */
                        for (i = 0; i < file_count; i++) {
                            free(file_list[i]);
                        }
                        free(file_list);
                    }
                    
                    /* Boutons de validation */
                    if (point_in_rect(mouse_x, mouse_y, WINDOW_WIDTH - 200, WINDOW_HEIGHT - 80, 140, 40)) {
                        /* Bouton VALIDER */
                        file_browser_open = 0;
                    } else if (point_in_rect(mouse_x, mouse_y, WINDOW_WIDTH - 350, WINDOW_HEIGHT - 80, 140, 40)) {
                        /* Bouton ANNULER */
                        file_browser_open = 0;
                    }
                    }
                } else if (point_in_rect(mouse_x, mouse_y, 200, WINDOW_HEIGHT - 250, 400, 35)) {
                    /* Clic sur la zone de saisie du nom de fichier de sortie */
                    text_input_active = 1;
                    text_input_mode = 1;
                } else if (point_in_rect(mouse_x, mouse_y, 50, WINDOW_HEIGHT - 100, 250, 60)) {
                    if (strlen(archive_name) > 0) {
                        perform_decompression(ctx, archive_name);
                    }
                } else if (point_in_rect(mouse_x, mouse_y, 350, WINDOW_HEIGHT - 100, 250, 60)) {
                    current_mode = 0; /* Retour au menu principal */
                } else {
                    /* Clic ailleurs, désactiver la saisie de texte */
                    text_input_active = 0;
                }
            } else if (event.type == SDL_MOUSEWHEEL) {
                /* Gestion du scroll */
                if (event.wheel.y > 0 && file_browser_scroll > 0) {
                    file_browser_scroll--;
                } else if (event.wheel.y < 0) {
                    file_browser_scroll++;
                }
            } else if (event.type == SDL_KEYDOWN) {
                /* Gestion de la saisie de texte */
                if (text_input_active && text_input_mode == 1) {
                    if (event.key.keysym.sym == SDLK_BACKSPACE) {
                        int len = strlen(output_filename);
                        if (len > 0) {
                            output_filename[len - 1] = '\0';
                        }
                    } else if (event.key.keysym.sym == SDLK_RETURN) {
                        text_input_active = 0;
                    } else if (event.key.keysym.sym == SDLK_ESCAPE) {
                        text_input_active = 0;
                    }
                }
            } else if (event.type == SDL_TEXTINPUT) {
                /* Saisie de caractères */
                if (text_input_active && text_input_mode == 1) {
                    int len = strlen(output_filename);
                    if (len < 255) {
                        output_filename[len] = event.text.text[0];
                        output_filename[len + 1] = '\0';
                    }
                }
            }
        }
        
        clear_window(ctx, bg);
        
        /* Titre */
        draw_text(ctx, 50, 20, "Mode Decompression", black);
        
        /* Bouton pour ouvrir le sélecteur d'archives */
        SDL_Color archive_bg;
        archive_bg.r = 255; archive_bg.g = 165; archive_bg.b = 0; archive_bg.a = 255;
        draw_button(ctx, 50, 150, 250, 50, "Selectionner une archive", 
                   black, archive_bg, 0);
        
        /* Navigateur de fichiers (seulement si ouvert) */
        draw_file_browser(ctx, current_directory, (char**)&archive_name, &nb_selected_files);
        
        /* Afficher l'archive sélectionnée */
        if (strlen(archive_name) > 0) {
            char selected_text[600];
            char *filename = strrchr(archive_name, '/');
            if (filename) filename++;
            else filename = archive_name;
            sprintf(selected_text, "Archive sélectionnée: %s", filename);
            draw_text(ctx, 50, 200, selected_text, green);
        }
        
        /* Zone de saisie du nom du fichier de sortie (seulement si une archive est sélectionnée) */
        if (strlen(archive_name) > 0) {
            draw_text(ctx, 50, WINDOW_HEIGHT - 250, "Nom du fichier de sortie:", black);
            
            /* Zone de saisie moderne */
            SDL_Color input_bg, input_border, placeholder_color;
            if (text_input_active && text_input_mode == 1) {
                input_bg.r = 50; input_bg.g = 50; input_bg.b = 60; input_bg.a = 255;
                input_border.r = 100; input_border.g = 150; input_border.b = 255; input_border.a = 255;
            } else {
                input_bg.r = 40; input_bg.g = 40; input_bg.b = 50; input_bg.a = 255;
                input_border.r = 80; input_border.g = 80; input_border.b = 90; input_border.a = 255;
            }
            placeholder_color.r = 150; placeholder_color.g = 150; placeholder_color.b = 150; placeholder_color.a = 255;
            
            /* Ombre de la zone de saisie */
            SDL_Rect shadow_rect = {202, WINDOW_HEIGHT - 248, 400, 35};
            SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 100);
            SDL_RenderFillRect(ctx->renderer, &shadow_rect);
            
            /* Fond de la zone de saisie */
            SDL_Rect input_rect = {200, WINDOW_HEIGHT - 250, 400, 35};
            SDL_SetRenderDrawColor(ctx->renderer, input_bg.r, input_bg.g, input_bg.b, input_bg.a);
            SDL_RenderFillRect(ctx->renderer, &input_rect);
            SDL_SetRenderDrawColor(ctx->renderer, input_border.r, input_border.g, input_border.b, input_border.a);
            SDL_RenderDrawRect(ctx->renderer, &input_rect);
            
            /* Texte dans la zone de saisie */
            char display_text[300];
            if (strlen(output_filename) == 0) {
                strcpy(display_text, "Entrez le nom du fichier de sortie...");
                draw_text(ctx, 210, WINDOW_HEIGHT - 240, display_text, placeholder_color);
            } else {
                strcpy(display_text, output_filename);
                draw_text(ctx, 210, WINDOW_HEIGHT - 240, display_text, black);
            }
            
            /* Curseur animé si actif */
            if (text_input_active && text_input_mode == 1) {
                int cursor_x;
                SDL_Rect cursor_rect;
                SDL_Color cursor_color;
                cursor_x = 210 + strlen(output_filename) * 8;
                cursor_rect.x = cursor_x; cursor_rect.y = WINDOW_HEIGHT - 240; cursor_rect.w = 2; cursor_rect.h = 16;
                cursor_color.r = 100; cursor_color.g = 150; cursor_color.b = 255; cursor_color.a = 200 + 55 * sin(animation_frame * 0.3);
                SDL_SetRenderDrawColor(ctx->renderer, cursor_color.r, cursor_color.g, cursor_color.b, cursor_color.a);
                SDL_RenderFillRect(ctx->renderer, &cursor_rect);
            }
        }
        
        /* Boutons */
        SDL_Color decompress_bg2;
        decompress_bg2.r = decompress_hover ? 20 : 60; decompress_bg2.g = decompress_hover ? 60 : 60; decompress_bg2.b = decompress_hover ? 40 : 70; decompress_bg2.a = 255;
        draw_button(ctx, 50, WINDOW_HEIGHT - 100, 250, 60, "DECOMPRESSER", 
                   black, decompress_bg2, decompress_hover);
        
        SDL_Color back_bg;
        back_bg.r = back_hover ? 80 : 60; back_bg.g = back_hover ? 20 : 60; back_bg.b = back_hover ? 20 : 70; back_bg.a = 255;
        draw_button(ctx, 350, WINDOW_HEIGHT - 100, 250, 60, "RETOUR", 
                   black, back_bg, back_hover);
        
        /* Affichage de la progression si active */
        if (show_progress) {
            draw_progress_bar(ctx, 50, WINDOW_HEIGHT - 200, 500, 30, progress_percent, progress_message);
        }

        update_window(ctx);
        SDL_Delay(16);
    }
}

void perform_compression(GraphiqueContext *ctx) {
    SDL_Color bg = {25, 25, 35, 255}; /* Fond sombre moderne */
    SDL_Color white = {255, 255, 255, 255}; /* Texte blanc */
    SDL_Color green = {100, 255, 150, 255}; /* Vert moderne */
    SDL_Color blue = {100, 150, 255, 255}; /* Bleu moderne */
    SDL_Color yellow = {255, 255, 100, 255}; /* Jaune moderne */
    FILE *fic_depart, *fic_dest;
    noeud *alphabet[256] = {NULL};
    noeud *huffman[256] = {NULL};
    int i, j;
    char archive_path[512];
    long compressed_size = 0;
    
    if (!ctx || !ctx->renderer) return;
    
    /* Vérifier qu'il y a des fichiers sélectionnés */
    if (nb_selected_files == 0) {
        show_warning = 1;
        strcpy(warning_message, "Aucun fichier selectionne pour la compression !");
        return;
    }
    
    /* Vérifier qu'un nom d'archive est fourni */
    if (strlen(archive_name) == 0) {
        show_warning = 1;
        strcpy(warning_message, "Veuillez entrer un nom d'archive !");
        return;
    }
    
    /* Phase 1: Calcul de la taille originale */
    show_progress = 1;
    progress_percent = 0;
    strcpy(progress_message, "Calcul de la taille des fichiers...");
    
    clear_window(ctx, bg);
    draw_text(ctx, 50, 50, "Compression Huffman", white);
    draw_text(ctx, 50, 80, "Phase 1: Analyse des fichiers", blue);
    draw_progress_bar(ctx, 50, 120, 500, 30, progress_percent, progress_message);
    update_window(ctx);
    SDL_Delay(1000);
    
    /* Calculer la taille totale des fichiers */
    total_original_size = 0;
    for (i = 0; i < nb_selected_files; i++) {
        FILE *file = fopen(selected_files[i], "rb");
        if (file) {
            long file_size;
            fseek(file, 0, SEEK_END);
            file_size = ftell(file);
            total_original_size += file_size;
            fclose(file);
        }
        
        progress_percent = ((i + 1) * 20) / nb_selected_files;
        sprintf(progress_message, "Analyse du fichier %d/%d...", i + 1, nb_selected_files);
        
        clear_window(ctx, bg);
        draw_text(ctx, 50, 50, "Compression Huffman", white);
        draw_text(ctx, 50, 80, "Phase 1: Analyse des fichiers", blue);
        draw_progress_bar(ctx, 50, 120, 500, 30, progress_percent, progress_message);
        
        /* Afficher le nom du fichier */
        char *filename;
        filename = strrchr(selected_files[i], '/');
        if (filename) filename++;
        else filename = selected_files[i];
        char file_info[200];
        sprintf(file_info, "Fichier: %s", filename);
        SDL_Color file_info_color;
        file_info_color.r = 200; file_info_color.g = 200; file_info_color.b = 200; file_info_color.a = 255;
        draw_text(ctx, 50, 160, file_info, file_info_color);
        
        update_window(ctx);
        SDL_Delay(300);
    }
    
    /* Afficher la taille calculée */
    char size_msg[256];
    sprintf(size_msg, "Taille totale: %ld bytes", total_original_size);
    draw_text(ctx, 50, 190, size_msg, yellow);
    update_window(ctx);
    SDL_Delay(1500);
    
    /* Phase 2: Compression */
    sprintf(archive_path, "%s.huff", archive_name);
    
    fic_dest = fopen(archive_path, "wb");
    if (!fic_dest) {
        show_warning = 1;
        strcpy(warning_message, "Impossible de creer l'archive !");
        show_progress = 0;
        return;
    }
    
    /* Compresser chaque fichier */
    for (i = 0; i < nb_selected_files; i++) {
        progress_percent = 20 + ((i * 60) / nb_selected_files);
        sprintf(progress_message, "Compression du fichier %d/%d...", i + 1, nb_selected_files);
        
        clear_window(ctx, bg);
        draw_text(ctx, 50, 50, "Compression Huffman", white);
        draw_text(ctx, 50, 80, "Phase 2: Compression des fichiers", blue);
        draw_progress_bar(ctx, 50, 120, 500, 30, progress_percent, progress_message);
        
        /* Afficher le nom du fichier en cours */
        char *filename;
        filename = strrchr(selected_files[i], '/');
        if (filename) filename++;
        else filename = selected_files[i];
        char file_info[200];
        sprintf(file_info, "Fichier: %s", filename);
        SDL_Color file_info_color;
        file_info_color.r = 200; file_info_color.g = 200; file_info_color.b = 200; file_info_color.a = 255;
        draw_text(ctx, 50, 160, file_info, file_info_color);
        
        update_window(ctx);
        
        /* Ouvrir le fichier source */
        fic_depart = fopen(selected_files[i], "rb");
        if (!fic_depart) {
            continue; /* Passer au fichier suivant */
        }
        
        /* Réinitialiser l'alphabet */
        for (j = 0; j < 256; j++) {
            if (alphabet[j]) {
                free(alphabet[j]);
                alphabet[j] = NULL;
            }
        }
        
        /* Analyser le fichier */
        int c;
        while ((c = fgetc(fic_depart)) != EOF) {
            if (alphabet[c] == NULL) {
                alphabet[c] = creer_st_noeud(c, 1);
            } else {
                alphabet[c]->occurence++;
            }
        }
        
        /* Reconstruire l'arbre Huffman */
        recreation_huffman(alphabet, huffman);
        
        /* Créer les codes */
        creer_code(huffman[0], 0, 0, alphabet);
        
        /* Écrire l'en-tête */
        en_tete(fic_dest, alphabet, filename);
        
        /* Compresser le contenu */
        fseek(fic_depart, 0, SEEK_SET);
        codes_fichier(fic_depart, fic_dest, alphabet);
        
        fclose(fic_depart);
    }
    
    /* Obtenir la taille du fichier compressé */
    fseek(fic_dest, 0, SEEK_END);
    compressed_size = ftell(fic_dest);
    fclose(fic_dest);
    total_compressed_size = compressed_size;
    
    /* Phase 3: Affichage des résultats */
    progress_percent = 100;
    strcpy(progress_message, "Compression terminee !");
    
    clear_window(ctx, bg);
    draw_text(ctx, 50, 50, "Compression Huffman", white);
    draw_text(ctx, 50, 80, "Phase 3: Resultats", green);
    draw_progress_bar(ctx, 50, 120, 500, 30, progress_percent, progress_message);
    
    /* Afficher les statistiques */
    char stats_msg[512];
    sprintf(stats_msg, "Taille originale: %ld bytes", total_original_size);
    draw_text(ctx, 50, 160, stats_msg, white);
    
    sprintf(stats_msg, "Taille compressee: %ld bytes", total_compressed_size);
    draw_text(ctx, 50, 190, stats_msg, white);
    
    if (total_compressed_size > 0) {
        float ratio, savings;
        ratio = (float)total_original_size / total_compressed_size;
        sprintf(stats_msg, "Ratio de compression: %.2f:1", ratio);
        draw_text(ctx, 50, 220, stats_msg, yellow);
        
        savings = ((float)(total_original_size - total_compressed_size) / total_original_size) * 100;
        sprintf(stats_msg, "Espace economise: %.1f%%", savings);
        draw_text(ctx, 50, 250, stats_msg, green);
    }
    
    sprintf(stats_msg, "Archive creee: %s", archive_path);
    draw_text(ctx, 50, 280, stats_msg, blue);
    
    /* Instructions pour continuer */
    SDL_Color continue_color;
    continue_color.r = 150; continue_color.g = 150; continue_color.b = 150; continue_color.a = 255;
    draw_text(ctx, 50, 320, "Appuyez sur une touche pour continuer...", continue_color);
    
    update_window(ctx);
    
    /* Attendre qu'une touche soit pressée */
    SDL_Event key_event;
    int key_pressed = 0;
    while (!key_pressed && ctx->running) {
        while (SDL_PollEvent(&key_event)) {
            if (key_event.type == SDL_QUIT) {
                ctx->running = 0;
                key_pressed = 1;
            } else if (key_event.type == SDL_KEYDOWN) {
                key_pressed = 1;
            }
        }
        SDL_Delay(16);
    }
    
    /* Nettoyer l'alphabet seulement (les nœuds huffman sont des références) */
    for (j = 0; j < 256; j++) {
        if (alphabet[j]) {
            free(alphabet[j]);
            alphabet[j] = NULL;
        }
    }
    
    /* Ne pas libérer huffman car les nœuds sont des références dans l'arbre */
    /* L'arbre sera libéré automatiquement quand le programme se termine */
    
    /* Réinitialiser le tableau huffman */
    for (j = 0; j < 256; j++) {
        huffman[j] = NULL;
    }
    
    show_progress = 0;
    
    /* Afficher le menu de choix final */
    clear_window(ctx, bg);
    draw_text(ctx, 50, 50, "Compression terminee avec succes!", green);
    draw_text(ctx, 50, 80, "Que voulez-vous faire ?", white);
    
    /* Boutons de choix */
    SDL_Color menu_fg, menu_bg, quit_fg, quit_bg;
    menu_fg.r = 100; menu_fg.g = 150; menu_fg.b = 255; menu_fg.a = 255;
    menu_bg.r = 80; menu_bg.g = 120; menu_bg.b = 200; menu_bg.a = 255;
    quit_fg.r = 255; quit_fg.g = 100; quit_fg.b = 100; quit_fg.a = 255;
    quit_bg.r = 200; quit_bg.g = 80; quit_bg.b = 80; quit_bg.a = 255;
    draw_button(ctx, 50, 120, 200, 40, "Retour au menu", menu_fg, menu_bg, 0);
    draw_button(ctx, 300, 120, 200, 40, "Quitter", quit_fg, quit_bg, 0);
    
    update_window(ctx);
    
    /* Attendre le choix de l'utilisateur */
    SDL_Event event;
    int choice_made = 0;
    while (!choice_made) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mouse_x = event.button.x;
                int mouse_y = event.button.y;
                
                /* Bouton "Retour au menu" */
                if (mouse_x >= 50 && mouse_x <= 250 && mouse_y >= 120 && mouse_y <= 160) {
                    current_mode = 0;
                    choice_made = 1;
                }
                /* Bouton "Quitter" */
                else if (mouse_x >= 300 && mouse_x <= 500 && mouse_y >= 120 && mouse_y <= 160) {
                    ctx->running = 0; /* Quitter l'application */
                    choice_made = 1;
                }
            }
        }
        SDL_Delay(16);
    }
}

void perform_decompression(GraphiqueContext *ctx, char *archive_name) {
    SDL_Color bg = {25, 25, 35, 255}; /* Fond sombre moderne */
    SDL_Color white = {255, 255, 255, 255}; /* Texte blanc */
    SDL_Color green = {100, 255, 150, 255}; /* Vert moderne */
    SDL_Color blue = {100, 150, 255, 255}; /* Bleu moderne */
    SDL_Color yellow = {255, 255, 100, 255}; /* Jaune moderne */
    int i;
    FILE *fic_comp, *fic_decom;
    noeud *alphabet[256] = {NULL};
    noeud *huffman[256] = {NULL};
    char nom_fichier[500];
    char *nom_ptr = nom_fichier;
    char output_path[512];
    long original_size = 0, compressed_size = 0;
    
    if (!ctx || !ctx->renderer) return;
    
    /* Vérifier qu'une archive est sélectionnée */
    if (strlen(archive_name) == 0) {
        show_warning = 1;
        strcpy(warning_message, "Aucune archive selectionnee pour la decompression !");
        return;
    }
    
    /* Phase 1: Ouverture et analyse de l'archive */
    show_progress = 1;
    progress_percent = 0;
    strcpy(progress_message, "Ouverture de l'archive...");
    
    clear_window(ctx, bg);
    draw_text(ctx, 50, 50, "Decompression Huffman", white);
    draw_text(ctx, 50, 80, "Phase 1: Analyse de l'archive", blue);
    draw_progress_bar(ctx, 50, 120, 500, 30, progress_percent, progress_message);
    update_window(ctx);
    SDL_Delay(1000);
    
    /* Ouvrir le fichier compressé */
    fic_comp = fopen(archive_name, "r");
    if (fic_comp == NULL) {
        show_warning = 1;
        strcpy(warning_message, "Impossible d'ouvrir l'archive !");
        show_progress = 0;
        return;
    }
    
    /* Obtenir la taille de l'archive (fichier compressé) */
    fseek(fic_comp, 0, SEEK_END);
    compressed_size = ftell(fic_comp);
    fseek(fic_comp, 0, SEEK_SET);
    total_compressed_size = compressed_size; /* Taille de l'archive */
    
    progress_percent = 20;
    strcpy(progress_message, "Lecture de l'en-tete...");
    
    clear_window(ctx, bg);
    draw_text(ctx, 50, 50, "Decompression Huffman", white);
    draw_text(ctx, 50, 80, "Phase 1: Analyse de l'archive", blue);
    draw_progress_bar(ctx, 50, 120, 500, 30, progress_percent, progress_message);
    update_window(ctx);
    SDL_Delay(500);
    
    /* Lire l'en-tête */
    rec_alph_fich(fic_comp, alphabet, &nom_ptr);
    
    /* Phase 2: Reconstruction de l'arbre Huffman */
    progress_percent = 40;
    strcpy(progress_message, "Reconstruction de l'arbre Huffman...");
    
    clear_window(ctx, bg);
    draw_text(ctx, 50, 50, "Decompression Huffman", white);
    draw_text(ctx, 50, 80, "Phase 2: Reconstruction de l'arbre", blue);
    draw_progress_bar(ctx, 50, 120, 500, 30, progress_percent, progress_message);
    update_window(ctx);
    SDL_Delay(500);
    
    /* Reconstruire l'arbre Huffman */
    recreation_huffman(alphabet, huffman);
    
    /* Créer le fichier de sortie */
    if (strlen(output_filename) > 0) {
        sprintf(output_path, "%s/%s", current_directory, output_filename);
    } else {
        sprintf(output_path, "%s/%s", current_directory, nom_fichier);
    }
    
    progress_percent = 60;
    strcpy(progress_message, "Creation du fichier de sortie...");
    
    clear_window(ctx, bg);
    draw_text(ctx, 50, 50, "Decompression Huffman", white);
    draw_text(ctx, 50, 80, "Phase 2: Reconstruction de l'arbre", blue);
    draw_progress_bar(ctx, 50, 120, 500, 30, progress_percent, progress_message);
    
    char file_info[200];
    SDL_Color file_info_color;
    sprintf(file_info, "Fichier de sortie: %s", output_path);
    file_info_color.r = 200; file_info_color.g = 200; file_info_color.b = 200; file_info_color.a = 255;
    draw_text(ctx, 50, 160, file_info, file_info_color);
    update_window(ctx);
    SDL_Delay(500);
    
    fic_decom = fopen(output_path, "w");
    if (fic_decom == NULL) {
        show_warning = 1;
        strcpy(warning_message, "Impossible de creer le fichier de sortie !");
        fclose(fic_comp);
        show_progress = 0;
        return;
    }
    
    /* Phase 3: Décompression des données */
    progress_percent = 80;
    strcpy(progress_message, "Decompression des donnees...");
    
    clear_window(ctx, bg);
    draw_text(ctx, 50, 50, "Decompression Huffman", white);
    draw_text(ctx, 50, 80, "Phase 3: Decompression des donnees", blue);
    draw_progress_bar(ctx, 50, 120, 500, 30, progress_percent, progress_message);
    file_info_color.r = 200; file_info_color.g = 200; file_info_color.b = 200; file_info_color.a = 255;
    draw_text(ctx, 50, 160, file_info, file_info_color);
    update_window(ctx);
    SDL_Delay(500);
    
    /* Décompresser */
    decompression(fic_comp, fic_decom, huffman[0], alphabet);
    
    /* Obtenir la taille du fichier original (décompressé) */
    fseek(fic_decom, 0, SEEK_END);
    original_size = ftell(fic_decom);
    total_original_size = original_size; /* Taille du fichier original */
    
    /* Fermer les fichiers */
    fclose(fic_comp);
    fclose(fic_decom);
    
    /* Phase 4: Affichage des résultats */
    progress_percent = 100;
    strcpy(progress_message, "Decompression terminee !");
    
    clear_window(ctx, bg);
    draw_text(ctx, 50, 50, "Decompression Huffman", white);
    draw_text(ctx, 50, 80, "Phase 4: Resultats", green);
    draw_progress_bar(ctx, 50, 120, 500, 30, progress_percent, progress_message);
    
    /* Afficher les statistiques de décompression */
    char stats_msg[512];
    sprintf(stats_msg, "Taille de l'archive: %ld bytes", total_compressed_size);
    draw_text(ctx, 50, 160, stats_msg, white);
    
    sprintf(stats_msg, "Taille du fichier original: %ld bytes", total_original_size);
    draw_text(ctx, 50, 190, stats_msg, white);
    
    if (total_compressed_size > 0) {
        float ratio, expansion;
        ratio = (float)total_original_size / total_compressed_size;
        sprintf(stats_msg, "Ratio de decompression: %.2f:1", ratio);
        draw_text(ctx, 50, 220, stats_msg, yellow);
        
        expansion = ((float)(total_original_size - total_compressed_size) / total_compressed_size) * 100;
        sprintf(stats_msg, "Expansion: +%.1f%%", expansion);
        draw_text(ctx, 50, 250, stats_msg, green);
    }
    
    sprintf(stats_msg, "Fichier cree: %s", output_path);
    draw_text(ctx, 50, 280, stats_msg, blue);
    
    /* Instructions pour continuer */
    SDL_Color continue_color;
    continue_color.r = 150; continue_color.g = 150; continue_color.b = 150; continue_color.a = 255;
    draw_text(ctx, 50, 320, "Appuyez sur une touche pour continuer...", continue_color);
    
    update_window(ctx);
    
    /* Attendre qu'une touche soit pressée */
    SDL_Event key_event2;
    int key_pressed = 0;
    while (!key_pressed && ctx->running) {
        while (SDL_PollEvent(&key_event2)) {
            if (key_event2.type == SDL_QUIT) {
                    ctx->running = 0;
                key_pressed = 1;
            } else if (key_event2.type == SDL_KEYDOWN) {
                key_pressed = 1;
            }
        }
        SDL_Delay(16);
    }
    
    /* Nettoyer l'alphabet seulement (les nœuds huffman sont des références) */
    for (i = 0; i < 256; i++) {
        if (alphabet[i]) {
            free(alphabet[i]);
            alphabet[i] = NULL;
        }
    }
    
    /* Ne pas libérer huffman car les nœuds sont des références dans l'arbre */
    /* L'arbre sera libéré automatiquement quand le programme se termine */
    
    /* Réinitialiser le tableau huffman */
    for (i = 0; i < 256; i++) {
        huffman[i] = NULL;
    }
    
    show_progress = 0;
    
    /* Afficher le menu de choix final */
    clear_window(ctx, bg);
    draw_text(ctx, 50, 50, "Decompression terminee avec succes!", green);
    draw_text(ctx, 50, 80, "Que voulez-vous faire ?", white);
    
    /* Boutons de choix */
    SDL_Color menu_fg, menu_bg, quit_fg, quit_bg;
    menu_fg.r = 100; menu_fg.g = 150; menu_fg.b = 255; menu_fg.a = 255;
    menu_bg.r = 80; menu_bg.g = 120; menu_bg.b = 200; menu_bg.a = 255;
    quit_fg.r = 255; quit_fg.g = 100; quit_fg.b = 100; quit_fg.a = 255;
    quit_bg.r = 200; quit_bg.g = 80; quit_bg.b = 80; quit_bg.a = 255;
    draw_button(ctx, 50, 120, 200, 40, "Retour au menu", menu_fg, menu_bg, 0);
    draw_button(ctx, 300, 120, 200, 40, "Quitter", quit_fg, quit_bg, 0);
    
    update_window(ctx);
    
    /* Attendre le choix de l'utilisateur */
    SDL_Event event;
    int choice_made = 0;
    while (!choice_made) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mouse_x = event.button.x;
                int mouse_y = event.button.y;
                
                /* Bouton "Retour au menu" */
                if (mouse_x >= 50 && mouse_x <= 250 && mouse_y >= 120 && mouse_y <= 160) {
                    current_mode = 0;
                    choice_made = 1;
                }
                /* Bouton "Quitter" */
                else if (mouse_x >= 300 && mouse_x <= 500 && mouse_y >= 120 && mouse_y <= 160) {
                    ctx->running = 0; /* Quitter l'application */
                    choice_made = 1;
                }
            }
        }
        SDL_Delay(16);
    }
}

/* Fonctions utilitaires existantes */
void show_compression_stats(GraphiqueContext *ctx, char *filename, int original_size, int compressed_size) {
    (void)ctx; (void)filename; (void)original_size; (void)compressed_size; /* Éviter les warnings */
    /* Fonction existante simplifiée */
}

void draw_huffman_tree(GraphiqueContext *ctx, noeud *root, int x, int y, int level) {
    (void)ctx; (void)root; (void)x; (void)y; (void)level; /* Éviter les warnings */
    /* Fonction existante simplifiée */
}

