CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude
SRC = src/allocator.c src/main.c
OBJ = $(SRC:src/%.c=build/%.o)
EXE = build/allocator_example
OBJ_DIR = build
OBJ_FILES = $(OBJ)

$(OBJ): build/%.o: src/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(EXE): $(OBJ_FILES)
	$(CC) $(OBJ_FILES) -o $(EXE)

run: $(EXE)
	./$(EXE)

clean:
	rm -rf $(OBJ_DIR) $(EXE)
