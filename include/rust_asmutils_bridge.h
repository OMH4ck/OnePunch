#ifndef RUST_ASMUTILS_BRIDGE_H_
#define RUST_ASMUTILS_BRIDGE_H_

#include "asmutils.h"
extern "C" {
#include "../rust-onepunch/bindings.h"
}

// Utility functions that use Rust implementations
OPCODE transfer_str_to_op_rust(const std::string& op_str);
std::string transfer_op_to_str_rust(OPCODE opcode);
REG get_reg_by_str_rust(const std::string& reg_str);
std::string get_reg_str_by_reg_rust(REG reg);

// Conversion functions between C++ and Rust types
RustReg cpp_reg_to_rust_reg(REG cpp_reg);
REG rust_reg_to_cpp_reg(RustReg rust_reg);
RustOpcode cpp_opcode_to_rust_opcode(OPCODE cpp_opcode);
OPCODE rust_opcode_to_cpp_opcode(RustOpcode rust_opcode);

#endif  // RUST_ASMUTILS_BRIDGE_H_