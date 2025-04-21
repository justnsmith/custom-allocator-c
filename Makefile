# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude
LDFLAGS = -pthread
DEBUG_FLAGS = -DDEBUG

# Source files
SRC = src/allocator.c src/main.c
OBJ = $(SRC:src/%.c=build/%.o)
DEBUG_OBJ = $(SRC:src/%.c=build/debug/%.o)
EXE = build/allocator_example
DEBUG_EXE = build/debug/allocator_example

# Test-related files
TEST_SRC = test/allocator_test.c
TEST_OBJ = $(TEST_SRC:test/%.c=build/test/%.o)
DEBUG_TEST_OBJ = $(TEST_SRC:test/%.c=build/debug/test/%.o)
TEST_EXE = build/allocator_test
DEBUG_TEST_EXE = build/debug/allocator_test

# Directories
OBJ_DIR = build
SRC_OBJ_DIR = build/src
TEST_OBJ_DIR = build/test
DEBUG_DIR = build/debug
DEBUG_SRC_DIR = build/debug/src
DEBUG_TEST_DIR = build/debug/test

# Default rule
all: $(EXE) $(TEST_EXE) $(DEBUG_EXE) $(DEBUG_TEST_EXE)

# Rule for source objects
build/%.o: src/%.c
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(SRC_OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Rule for debug source objects
build/debug/%.o: src/%.c
	@mkdir -p $(DEBUG_DIR)
	@mkdir -p $(DEBUG_SRC_DIR)
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -c $< -o $@

# Rule for test objects
build/test/%.o: test/%.c
	@mkdir -p $(TEST_OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Rule for debug test objects
build/debug/test/%.o: test/%.c
	@mkdir -p $(DEBUG_TEST_DIR)
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -c $< -o $@

# Main executable rule
$(EXE): $(OBJ)
	@mkdir -p $(OBJ_DIR)
	$(CC) $^ -o $@ $(LDFLAGS)

# Debug executable rule
$(DEBUG_EXE): $(DEBUG_OBJ)
	@mkdir -p $(DEBUG_DIR)
	$(CC) $^ -o $@ $(LDFLAGS)

# Test executable rule
$(TEST_EXE): $(TEST_OBJ) build/allocator.o
	@mkdir -p $(OBJ_DIR)
	$(CC) $^ -o $@ $(LDFLAGS)

# Debug test executable rule
$(DEBUG_TEST_EXE): $(DEBUG_TEST_OBJ) build/debug/allocator.o
	@mkdir -p $(DEBUG_DIR)
	$(CC) $^ -o $@ $(LDFLAGS)

# Custom rule for main program
main: $(EXE)
	./$(EXE)

# Run the main program
run: $(EXE)
	./$(EXE)

# Run debug version
debug_run: $(DEBUG_EXE)
	./$(DEBUG_EXE)

# Run tests
test: $(TEST_EXE)
	./$(TEST_EXE)

# Run debug tests
debug_test: $(DEBUG_TEST_EXE)
	./$(DEBUG_TEST_EXE)

# Clean build
clean:
	rm -rf $(OBJ_DIR)

.PHONY: all run debug_run test debug_test main clean
