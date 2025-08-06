#include <doctest/doctest.h>
#include "../../include/rust_asmutils_bridge.h"

TEST_CASE("Rust AsmUtils Basic Test") {
    // Test opcode conversion
    OPCODE mov_opcode = transfer_str_to_op_rust("mov");
    CHECK(mov_opcode == OP_MOV);
    
    std::string mov_str = transfer_op_to_str_rust(OP_MOV);
    CHECK(mov_str == "mov");
    
    // Test call instruction
    OPCODE call_opcode = transfer_str_to_op_rust("call");
    CHECK(call_opcode == OP_CALL);
    
    // Test conditional jump
    OPCODE jcc_opcode = transfer_str_to_op_rust("jne");
    CHECK(jcc_opcode == OP_JCC);
}

TEST_CASE("Rust Register String Conversion Test") {
    // Test register name conversion
    REG rax_reg = get_reg_by_str_rust("rax");
    CHECK(rax_reg == REG_RAX);
    
    std::string rax_str = get_reg_str_by_reg_rust(REG_RAX);
    CHECK(rax_str == "rax");
    
    // Test 64-bit registers
    REG r8_reg = get_reg_by_str_rust("r8");
    CHECK(r8_reg == REG_R8);
    
    std::string r8_str = get_reg_str_by_reg_rust(REG_R8);
    CHECK(r8_str == "r8");
    
    // Test 32-bit registers
    REG eax_reg = get_reg_by_str_rust("eax");
    CHECK(eax_reg == REG_EAX);
    
    std::string eax_str = get_reg_str_by_reg_rust(REG_EAX);
    CHECK(eax_str == "eax");
}

TEST_CASE("Rust Type Conversion Test") {
    // Test C++ to Rust register conversion
    RustReg rust_rdi = cpp_reg_to_rust_reg(REG_RDI);
    CHECK(rust_rdi == RUST_REG_REG_RDI);
    
    // Test Rust to C++ register conversion
    REG cpp_rdi = rust_reg_to_cpp_reg(RUST_REG_REG_RDI);
    CHECK(cpp_rdi == REG_RDI);
    
    // Test opcode conversions
    RustOpcode rust_mov = cpp_opcode_to_rust_opcode(OP_MOV);
    CHECK(rust_mov == RUST_OPCODE_OP_MOV);
    
    OPCODE cpp_mov = rust_opcode_to_cpp_opcode(RUST_OPCODE_OP_MOV);
    CHECK(cpp_mov == OP_MOV);
}