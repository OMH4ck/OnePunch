# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

OnePunch is a tool for finding ROP gadgets to achieve arbitrary code execution by controlling registers and RIP. Given controlled input registers and target output registers, it finds gadget chains in x86_64 binaries using symbolic execution and constraint solving.

## Build Commands

**Main executable (Release build):**
```bash
cmake -Sstandalone -Bbuild
cmake --build build -j4
```

**Debug build with sanitizers:**
```bash
cmake -Sstandalone -Bbuild -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j4
```

**Tests:**
```bash
cmake -S. -Bbuild_test
cmake --build build_test -j4
./build_test/test/OnePunch_Tests
```

**Run single test:**
```bash
./build_test/OnePunch_Tests --test-case="Find and Minimize Solution Test"
```

## Core Architecture

### Main Components

- **OnePunch** (`include/onepunch.h`, `srcs/onepunch.cpp`): Main orchestrator class that coordinates the entire analysis pipeline
- **Preprocessor** (`include/preprocessor.h`, `srcs/preprocessor.cpp`): Groups gadgets by constraints for efficient searching
- **SymbolicExecutor** (`include/symbolic_executor.h`, `srcs/symbolic_executor.cpp`): Executes instruction sequences symbolically to track register states
- **Solver** (`include/solver.h`, `srcs/solver.cpp`): DFS-based search algorithm to find valid gadget chains
- **Register** (`include/register.h`, `srcs/register.cpp`): Models register state with memory aliasing and value tracking
- **AsmUtils** (`include/asmutils.h`, `srcs/asmutils.cpp`): Assembly parsing and instruction representation

### Data Flow

1. **Binary Analysis**: Parse binary file and extract gadgets ending in call/jmp instructions
2. **Preprocessing**: Group gadgets by hash of input/output register constraints  
3. **Search**: DFS through gadget combinations using symbolic execution to verify feasibility
4. **Minimization**: Remove redundant instructions while preserving solution validity
5. **Memory Layout**: Generate final memory layout showing required buffer contents

### Key Data Structures

- **RegisterPtr**: Shared pointer to Register objects modeling x86_64 registers with memory backing
- **SegmentPtr**: Instruction sequences (gadgets) with metadata about useful instruction ranges
- **Memory**: Tracks memory ranges, content values, and reference counts for aliasing
- **Solution**: Contains found gadget chain, register states, and memory requirements

## Development Notes

- **Language**: C++17 with shared_ptr for memory management
- **Dependencies**: fmt (formatting), argparse (CLI), doctest (testing)
- **Platform**: Linux x86_64 only
- **Sanitizers**: AddressSanitizer enabled in debug builds and tests
- **Search**: Uses randomization - re-run for different results if solution doesn't work

## Testing Strategy

Tests use deterministic output by seeding random number generator. The main test verifies end-to-end functionality by finding a gadget chain in libc that controls `rdi` from `r8`.
