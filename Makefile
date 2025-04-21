CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude
LDFLAGS = -pthread
SRC = src/allocator.c src/main.c
OBJ = $(SRC:src/%.c=build/%.o)
EXE = build/allocator_example

# Test-related variables
TEST_SRC = test/allocator_test.c src/allocator.c
TEST_OBJ = $(TEST_SRC:%.c=build/%.o)
TEST_EXE = build/allocator_test

# Create build directory structure
OBJ_DIR = build
TEST_OBJ_DIR = build/test
SRC_OBJ_DIR = build/src

# Default rule
all: $(EXE) $(TEST_EXE)

# Rule for source objects
build/src/%.o: src/%.c
	@mkdir -p $(SRC_OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Rule for test objects
build/test/%.o: test/%.c
	@mkdir -p $(TEST_OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Main executable
$(EXE): $(OBJ)
	@mkdir -p $(OBJ_DIR)
	$(CC) $^ -o $@ $(LDFLAGS)

# Test executable
$(TEST_EXE): build/test/allocator_test.o build/src/allocator.o
	@mkdir -p $(OBJ_DIR)
	$(CC) $^ -o $@ $(LDFLAGS)

# Run main program
run: $(EXE)
	./$(EXE)

# Run tests
test: $(TEST_EXE)
	./$(TEST_EXE)

# Clean build artifacts
clean:
	rm -rf $(OBJ_DIR)

.PHONY: all run test clean
