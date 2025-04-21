# custom-allocator-c

A custom memory allocator written in C from scratch that mimics standard memory management functions like `malloc`, `free`, and `realloc`. It features manual heap management, multiple allocation strategies (First-Fit, Best-Fit, Worst-Fit), and heap integrity checks.

## Table of Contents

1. [Features](#features)
2. [File Structure](#file-structure)
3. [Setup and Usage](#setup-and-usage)
   - [Build](#build)
   - [Run](#run)
   - [Run Tests](#run-tests)
4. [Memory Layout](#memory-layout)
5. [Testing](#testing)
6. [Development Notes](#development-notes)

## Features

- Custom implementations of:
  - `heap_alloc(size_t size)` - Allocates memory
  - `heap_free(void* ptr)` - Frees allocated memory
  - `heap_realloc(void* ptr, size_t new_size)` - Resizes an allocated block
- Allocation strategies:
  - **First-Fit**: Finds the first suitable block
  - **Best-Fit**: Chooses the smallest block that fits
  - **Worst-Fit**: Chooses the largest block available
- Manual memory coalescing and fragmentation handling
- Heap integrity checks to ensure no invalid memory access or corruption
- Alignment handling for block headers
- Unit tests with color-coded output

## File Structure
```
├── include/
│   └── allocator.h      # Header file with allocator interface
├── src/
│   ├── allocator.c      # Implementation of the memory allocator
│   └── main.c           # Main application entry point
└── test/
└── allocator_test.c     # Tests for the allocator implementation
```

## Setup and Usage

### Build
```bash
# Clone the repository
git clone https://github.com/justnsmith/custom-allocator-c.git
cd custom-allocator-c

# Compile the project
make
```

### Run
```bash
# Run the main program
make run

# Run with debug flags enabled
make debug_run
```

### Run Tests
```bash
# Run the test version
make test

# Run tests with debug flags enabled
make debug_test
```
