# OnePunch Rust Migration Guide

## Overview

This document outlines the incremental approach to migrating OnePunch from C++ to Rust while maintaining functionality throughout the process.

## Current Status

✅ **Phase 1: Foundation Setup**
- Created Rust crate (`rust-onepunch`) with FFI capabilities
- Set up CMake integration for building Rust alongside C++
- Implemented basic Register module in Rust with C FFI bindings
- Created bridge classes for seamless C++/Rust interoperability

## Architecture

### Project Structure
```
OnePunch/
├── rust-onepunch/          # Rust crate
│   ├── src/
│   │   ├── lib.rs          # Main Rust library
│   │   ├── ffi.rs          # C FFI bindings
│   │   └── register.rs     # Register implementation
│   ├── Cargo.toml          # Rust dependencies
│   ├── build.rs            # Build script for C bindings
│   ├── cbindgen.toml       # C header generation config
│   └── bindings.h          # Generated C headers
├── include/
│   └── rust_register_bridge.h  # C++ bridge class
├── srcs/
│   └── rust_register_bridge.cpp # Bridge implementation
└── CMakeLists.txt          # Updated with Rust integration
```

### FFI Integration

The migration uses Foreign Function Interface (FFI) to allow C++ and Rust code to interoperate:

1. **Rust Side**: Functions marked with `#[no_mangle]` and `extern "C"`
2. **C Bindings**: Auto-generated headers using `cbindgen`
3. **C++ Bridge**: Wrapper classes that provide C++ interfaces to Rust functionality

### Migration Strategy

**Phase 1: Register Module (COMPLETED)**
- ✅ Implement `RustValue`, `RustMemory`, and `RustRegister` in Rust
- ✅ Create FFI functions for all operations
- ✅ Build C++ bridge class `RustRegisterBridge`
- ✅ Integrate with existing CMake build system

**Phase 2: Assembly Utils (PLANNED)**
- Migrate `Operand`, `Instruction`, and `Segment` classes
- Implement instruction parsing in Rust
- Create bridge for assembly utilities

**Phase 3: Symbolic Execution (PLANNED)**
- Port `SymbolicExecutor` to Rust
- Implement register tracking and constraint solving
- Maintain compatibility with existing solver interface

**Phase 4: Core Logic (PLANNED)**
- Migrate `Preprocessor`, `Solver`, and `OnePunch` classes
- Implement DFS search algorithm in Rust
- Complete the migration

## Technical Decisions

### Memory Management
- Rust code uses `Box<T>` for heap allocation
- FFI functions use raw pointers for C compatibility
- Bridge classes handle memory lifecycle automatically

### Type Safety
- Rust enums are prefixed (e.g., `RustRegType`) to avoid name conflicts
- All FFI types use `#[repr(C)]` for C compatibility
- Bridge classes provide type-safe conversion between C++ and Rust types

### Build System
- CMake orchestrates the entire build process
- Cargo builds the Rust library as a static archive
- `cbindgen` automatically generates C headers during build

## Usage Example

```cpp
#include "rust_register_bridge.h"

// Create a new register using Rust implementation
RustRegisterBridge reg(true);  // allocate memory

// Set register name
reg.set_name(REG_RAX);

// Verify the operation worked
assert(reg.get_name() == REG_RAX);

// Create alias relationship
RustRegisterBridge reg2(true);
reg2.alias(reg, true);  // copy memory
assert(reg2.get_name() == REG_RAX);
```

## Build Instructions

1. **Configure Build**:
   ```bash
   cmake -S. -Bbuild_main
   ```

2. **Build Project**:
   ```bash
   cmake --build build_main -j4
   ```

3. **Run Tests**:
   ```bash
   ./build_main/test/OnePunch_Tests
   ```

## Benefits of This Approach

1. **Incremental Migration**: Code can be migrated module by module
2. **Compatibility**: Existing C++ code continues to work unchanged
3. **Safety**: Rust provides memory safety and thread safety
4. **Performance**: Rust offers zero-cost abstractions
5. **Testing**: Each module can be tested independently

## Next Steps

1. Fix any remaining linking issues with the FFI integration
2. Begin migrating the next module (AsmUtils)
3. Gradually replace C++ implementations with Rust versions
4. Eventually remove C++ code and bridge classes

## Troubleshooting

**Linking Issues**: If you encounter undefined reference errors:
- Ensure `rust_onepunch_static` is linked to targets that use bridge classes
- Verify Rust library is built before C++ code that depends on it
- Add required system libraries (`-lpthread -ldl`)

**Symbol Conflicts**: If there are naming conflicts:
- Use prefixed enum names (e.g., `RUST_REG_TYPE_REG_RAX`)
- Update bridge conversion functions accordingly