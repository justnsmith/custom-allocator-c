# custom-allocator-c
![GitHub release (latest by semver)](https://img.shields.io/github/v/release/justnsmith/custom-allocator-c)

A custom memory allocator written in C from scratch that mimics standard memory management functions like `malloc`, `free`, and `realloc`. It features manual heap management, multiple allocation strategies (First-Fit, Best-Fit, Worst-Fit), and heap integrity checks.

## Table of Contents

1. [Features](#features)
2. [File Structure](#file-structure)
3. [Setup and Usage](#setup-and-usage)
   - [Build](#build)
   - [Run Main](#run-main)
   - [Run Tests](#run-tests)
4. [Testing](#testing)
5. [Benchmarks](#benchmarks)
6. [Development Notes](#development-notes)
    - [Memory Layout](#memory-layout-design)
    - [Design Decisions](#design-decisions)
    - [Performance Considerations](#performance-considerations)

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
    └── allocator_test.c # Tests for the allocator implementation
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

### Run Main
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

## Testing

The test suite in `test/allocator_test.c` includes comprehensive tests to verify the correct functioning of the memory allocator:

- **Basic Operations**: Tests for `heap_alloc`, `heap_free`, and `heap_realloc` functionality
- **Allocation Strategies**: Verifies correct behavior of First-Fit, Best-Fit, and Worst-Fit strategies
- **Edge Cases**:
  - Zero-size allocations
  - NULL pointer handling
  - Out-of-memory conditions
  - Boundary alignments
- **Performance**: Stress tests with repeated allocations and deallocations
- **Memory Integrity**: Checks for proper block coalescing and prevention of memory leaks
- **Error Handling**: Validates proper error reporting and recovery

The test framework includes color-coded output for easy visual identification of passing and failing tests.

## Benchmarks

The benchmark suite tests all three allocation strategies across different workloads:
- Sequential allocation (1000 blocks)
- Random size allocation (varying 32-512 byte blocks)
- Fragmentation analysis (mixed alloc/free patterns)
- Allocation/deallocation cycles
- Reallocation performance (growing blocks from 64 to 1024 bytes)
- Worst-case scenarios (pathological patterns)
- Memory efficiency (overhead and utilization)

Results show Worst-Fit consistently outperforms the others in allocation speed, hitting ~677k ops/sec for sequential allocations compared to First-Fit's ~440k. Fragmentation stays nearly identical across all strategies (0.0055-0.0066 ratio), and memory overhead is the same at 18.82% regardless of strategy. In practice, the choice between strategies matters less than expected since coalescing works well across the board.

## Development Notes

### Memory Layout Design
- **Block Header Structure**: Each memory block is prefixed with a header containing metadata:

```c
  typedef struct BlockHeader {
      size_t size;
      bool free;
      struct BlockHeader* next;
  } BlockHeader;
```

- Chose this 24 byte header over a 32 byte header which contains a prev pointer due to simplicity and memory effiency
- Forward-only traversal trades some coalescing performance for the reduced overhead
- Proper alignment ensures consistent memory access patterns

### Design Decisions
- **Static Heap**: Using a fixed memory region makes the allocator usable in embedded systems and educational purposes without OS memory management dependencies
- **Alignment**: Chose 16 byte alignment since it satisfies common alignment requirements for most data types and ensures that allocated memory is compatible with standard C data strutures on modern 64-bit systems.
- **Block Splitting**: When a large block is allocated, remaining space will be split into a new free block when benefical, this balances fragmentation against header overhead.

### Performance Considerations
- Memory coalescing during free operations keeps fragmentation manageable
- Strategy selection impacts performance:
    - First-Fit optimizes for allocation speed
    - Best-Fit minimizes wasted space
    - Worst-Fit reduces creation of unusably small fragments
