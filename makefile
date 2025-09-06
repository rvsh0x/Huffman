CC = gcc
CFLAGS = -W -Wall -std=c99 -O2 -I./src/headers `sdl2-config --cflags`
LDFLAGS = `sdl2-config --libs`
LDLIBS = -lSDL2_ttf

EXEC = huffman
SRC_DIR = ./src

# Pour le projet principal, inclure tous les fichiers .c
SRCS = $(wildcard $(SRC_DIR)/*.c)
# Exclure les fichiers qui ne sont pas compatibles C89
SRCS := $(filter-out $(SRC_DIR)/file_selector_old.c $(SRC_DIR)/filepicker.c $(SRC_DIR)/fp_fs.c $(SRC_DIR)/fp_native.c $(SRC_DIR)/fp_persist.c $(SRC_DIR)/fp_preview.c $(SRC_DIR)/fp_ui_theme.c $(SRC_DIR)/fp_ui.c, $(SRCS))
OBJS = $(SRCS:.c=.o)

# Compilation du projet principal
all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) -g $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

# Règle générique pour les .o
%.o: %.c
	$(CC) -g $(CFLAGS) -c $< -o $@

# Règle générique pour les versions (exclure main.c)
v%: ./src/v%.o $(filter-out ./src/main.o, $(OBJS))
	$(CC) -g $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

./src/v%.o: ./src/versions/huff_v%.c
	$(CC) -g $(CFLAGS) -c $< -o $@

# Nettoyage
clean:
	rm -rf $(OBJS) src/*.o *~

cleanall: clean
	rm -rf $(EXEC) v[0-5]