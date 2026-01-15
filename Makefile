# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude
LDFLAGS = -pthread -lm
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

# Benchmark-related files
BENCH_SRC = benchmark/benchmark.c
BENCH_OBJ = $(BENCH_SRC:benchmark/%.c=build/benchmark/%.o)
DEBUG_BENCH_OBJ = $(BENCH_SRC:benchmark/%.c=build/debug/benchmark/%.o)
BENCH_EXE = build/allocator_benchmark
DEBUG_BENCH_EXE = build/debug/allocator_benchmark

# Directories
OBJ_DIR = build
SRC_OBJ_DIR = build/src
TEST_OBJ_DIR = build/test
BENCH_OBJ_DIR = build/benchmark
DEBUG_DIR = build/debug
DEBUG_SRC_DIR = build/debug/src
DEBUG_TEST_DIR = build/debug/test
DEBUG_BENCH_DIR = build/debug/benchmark

# Default rule
all: $(EXE) $(TEST_EXE) $(BENCH_EXE) $(DEBUG_EXE) $(DEBUG_TEST_EXE) $(DEBUG_BENCH_EXE)

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

# Rule for benchmark objects
build/benchmark/%.o: benchmark/%.c
	@mkdir -p $(BENCH_OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Rule for debug benchmark objects
build/debug/benchmark/%.o: benchmark/%.c
	@mkdir -p $(DEBUG_BENCH_DIR)
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

# Benchmark executable rule
$(BENCH_EXE): $(BENCH_OBJ) build/allocator.o
	@mkdir -p $(OBJ_DIR)
	$(CC) $^ -o $@ $(LDFLAGS)

# Debug benchmark executable rule
$(DEBUG_BENCH_EXE): $(DEBUG_BENCH_OBJ) build/debug/allocator.o
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

# Run benchmarks
benchmark: $(BENCH_EXE)
	./$(BENCH_EXE)

# Run debug benchmarks
debug_benchmark: $(DEBUG_BENCH_EXE)
	./$(DEBUG_BENCH_EXE)

# Save benchmark results to file with timestamp
benchmark_save: $(BENCH_EXE)
	@mkdir -p benchmark/results
	./$(BENCH_EXE) | tee benchmark/results/benchmark_$(shell date +%Y%m%d_%H%M%S).txt

# Clean build
clean:
	rm -rf $(OBJ_DIR)

# Help command
help:
	@echo "Available targets:"
	@echo "  all              - Build all executables (default)"
	@echo "  run              - Build and run main program"
	@echo "  test             - Build and run tests"
	@echo "  benchmark        - Build and run benchmarks"
	@echo "  benchmark_save   - Run benchmarks and save results with timestamp"
	@echo "  debug_run        - Build and run main program (debug mode)"
	@echo "  debug_test       - Build and run tests (debug mode)"
	@echo "  debug_benchmark  - Build and run benchmarks (debug mode)"
	@echo "  clean            - Remove all build files"
	@echo "  help             - Show this help message"

.PHONY: all run debug_run test debug_test benchmark debug_benchmark benchmark_save main clean help
