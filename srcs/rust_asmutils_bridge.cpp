#include "../include/rust_asmutils_bridge.h"
#include <stdexcept>

RustReg cpp_reg_to_rust_reg(REG cpp_reg) {
    switch (cpp_reg) {
        case REG_NONE: return RUST_REG_REG_NONE;
        case REG_RAX: return RUST_REG_REG_RAX;
        case REG_RCX: return RUST_REG_REG_RCX;
        case REG_RDX: return RUST_REG_REG_RDX;
        case REG_RBX: return RUST_REG_REG_RBX;
        case REG_RSI: return RUST_REG_REG_RSI;
        case REG_RDI: return RUST_REG_REG_RDI;
        case REG_RSP: return RUST_REG_REG_RSP;
        case REG_RBP: return RUST_REG_REG_RBP;
        case REG_R8: return RUST_REG_REG_R8;
        case REG_R9: return RUST_REG_REG_R9;
        case REG_R10: return RUST_REG_REG_R10;
        case REG_R11: return RUST_REG_REG_R11;
        case REG_R12: return RUST_REG_REG_R12;
        case REG_R13: return RUST_REG_REG_R13;
        case REG_R14: return RUST_REG_REG_R14;
        case REG_R15: return RUST_REG_REG_R15;
        case REG_RIP: return RUST_REG_REG_RIP;
        case REG_EAX: return RUST_REG_REG_EAX;
        case REG_ECX: return RUST_REG_REG_ECX;
        case REG_EDX: return RUST_REG_REG_EDX;
        case REG_EBX: return RUST_REG_REG_EBX;
        case REG_ESI: return RUST_REG_REG_ESI;
        case REG_EDI: return RUST_REG_REG_EDI;
        case REG_ESP: return RUST_REG_REG_ESP;
        case REG_EBP: return RUST_REG_REG_EBP;
        case REG_R8D: return RUST_REG_REG_R8D;
        case REG_R9D: return RUST_REG_REG_R9D;
        case REG_R10D: return RUST_REG_REG_R10D;
        case REG_R11D: return RUST_REG_REG_R11D;
        case REG_R12D: return RUST_REG_REG_R12D;
        case REG_R13D: return RUST_REG_REG_R13D;
        case REG_R14D: return RUST_REG_REG_R14D;
        case REG_R15D: return RUST_REG_REG_R15D;
        case REG_EIP: return RUST_REG_REG_EIP;
        default: return RUST_REG_REG_NONE;
    }
}

REG rust_reg_to_cpp_reg(RustReg rust_reg) {
    switch (rust_reg) {
        case RUST_REG_REG_NONE: return REG_NONE;
        case RUST_REG_REG_RAX: return REG_RAX;
        case RUST_REG_REG_RCX: return REG_RCX;
        case RUST_REG_REG_RDX: return REG_RDX;
        case RUST_REG_REG_RBX: return REG_RBX;
        case RUST_REG_REG_RSI: return REG_RSI;
        case RUST_REG_REG_RDI: return REG_RDI;
        case RUST_REG_REG_RSP: return REG_RSP;
        case RUST_REG_REG_RBP: return REG_RBP;
        case RUST_REG_REG_R8: return REG_R8;
        case RUST_REG_REG_R9: return REG_R9;
        case RUST_REG_REG_R10: return REG_R10;
        case RUST_REG_REG_R11: return REG_R11;
        case RUST_REG_REG_R12: return REG_R12;
        case RUST_REG_REG_R13: return REG_R13;
        case RUST_REG_REG_R14: return REG_R14;
        case RUST_REG_REG_R15: return REG_R15;
        case RUST_REG_REG_RIP: return REG_RIP;
        case RUST_REG_REG_EAX: return REG_EAX;
        case RUST_REG_REG_ECX: return REG_ECX;
        case RUST_REG_REG_EDX: return REG_EDX;
        case RUST_REG_REG_EBX: return REG_EBX;
        case RUST_REG_REG_ESI: return REG_ESI;
        case RUST_REG_REG_EDI: return REG_EDI;
        case RUST_REG_REG_ESP: return REG_ESP;
        case RUST_REG_REG_EBP: return REG_EBP;
        case RUST_REG_REG_R8D: return REG_R8D;
        case RUST_REG_REG_R9D: return REG_R9D;
        case RUST_REG_REG_R10D: return REG_R10D;
        case RUST_REG_REG_R11D: return REG_R11D;
        case RUST_REG_REG_R12D: return REG_R12D;
        case RUST_REG_REG_R13D: return REG_R13D;
        case RUST_REG_REG_R14D: return REG_R14D;
        case RUST_REG_REG_R15D: return REG_R15D;
        case RUST_REG_REG_EIP: return REG_EIP;
        default: return REG_NONE;
    }
}

RustOpcode cpp_opcode_to_rust_opcode(OPCODE cpp_opcode) {
    switch (cpp_opcode) {
        case OP_NONE: return RUST_OPCODE_OP_NONE;
        case OP_MOV: return RUST_OPCODE_OP_MOV;
        case OP_LEA: return RUST_OPCODE_OP_LEA;
        case OP_POP: return RUST_OPCODE_OP_POP;
        case OP_ADD: return RUST_OPCODE_OP_ADD;
        case OP_SUB: return RUST_OPCODE_OP_SUB;
        case OP_IMUL: return RUST_OPCODE_OP_IMUL;
        case OP_MUL: return RUST_OPCODE_OP_MUL;
        case OP_DIV: return RUST_OPCODE_OP_DIV;
        case OP_PUSH: return RUST_OPCODE_OP_PUSH;
        case OP_XOR: return RUST_OPCODE_OP_XOR;
        case OP_OR: return RUST_OPCODE_OP_OR;
        case OP_AND: return RUST_OPCODE_OP_AND;
        case OP_SHR: return RUST_OPCODE_OP_SHR;
        case OP_SHL: return RUST_OPCODE_OP_SHL;
        case OP_ROR: return RUST_OPCODE_OP_ROR;
        case OP_SAR: return RUST_OPCODE_OP_SAR;
        case OP_TEST: return RUST_OPCODE_OP_TEST;
        case OP_NOP: return RUST_OPCODE_OP_NOP;
        case OP_CMP: return RUST_OPCODE_OP_CMP;
        case OP_CALL: return RUST_OPCODE_OP_CALL;
        case OP_JMP: return RUST_OPCODE_OP_JMP;
        case OP_XCHG: return RUST_OPCODE_OP_XCHG;
        case OP_JCC: return RUST_OPCODE_OP_JCC;
        case OP_RET: return RUST_OPCODE_OP_RET;
        case OP_SYSCALL: return RUST_OPCODE_OP_SYSCALL;
        case OP_INT3: return RUST_OPCODE_OP_INT3;
        case OP_SFENCE: return RUST_OPCODE_OP_SFENCE;
        case OP_BSWAP: return RUST_OPCODE_OP_BSWAP;
        case OP_MOVAPS: return RUST_OPCODE_OP_MOVAPS;
        case OP_MOVDQA: return RUST_OPCODE_OP_MOVDQA;
        case OP_MOVNTDQ: return RUST_OPCODE_OP_MOVNTDQ;
        case OP_MOVSXD: return RUST_OPCODE_OP_MOVSXD;
        default: return RUST_OPCODE_OP_NONE;
    }
}

OPCODE rust_opcode_to_cpp_opcode(RustOpcode rust_opcode) {
    switch (rust_opcode) {
        case RUST_OPCODE_OP_NONE: return OP_NONE;
        case RUST_OPCODE_OP_MOV: return OP_MOV;
        case RUST_OPCODE_OP_LEA: return OP_LEA;
        case RUST_OPCODE_OP_POP: return OP_POP;
        case RUST_OPCODE_OP_ADD: return OP_ADD;
        case RUST_OPCODE_OP_SUB: return OP_SUB;
        case RUST_OPCODE_OP_IMUL: return OP_IMUL;
        case RUST_OPCODE_OP_MUL: return OP_MUL;
        case RUST_OPCODE_OP_DIV: return OP_DIV;
        case RUST_OPCODE_OP_PUSH: return OP_PUSH;
        case RUST_OPCODE_OP_XOR: return OP_XOR;
        case RUST_OPCODE_OP_OR: return OP_OR;
        case RUST_OPCODE_OP_AND: return OP_AND;
        case RUST_OPCODE_OP_SHR: return OP_SHR;
        case RUST_OPCODE_OP_SHL: return OP_SHL;
        case RUST_OPCODE_OP_ROR: return OP_ROR;
        case RUST_OPCODE_OP_SAR: return OP_SAR;
        case RUST_OPCODE_OP_TEST: return OP_TEST;
        case RUST_OPCODE_OP_NOP: return OP_NOP;
        case RUST_OPCODE_OP_CMP: return OP_CMP;
        case RUST_OPCODE_OP_CALL: return OP_CALL;
        case RUST_OPCODE_OP_JMP: return OP_JMP;
        case RUST_OPCODE_OP_XCHG: return OP_XCHG;
        case RUST_OPCODE_OP_JCC: return OP_JCC;
        case RUST_OPCODE_OP_RET: return OP_RET;
        case RUST_OPCODE_OP_SYSCALL: return OP_SYSCALL;
        case RUST_OPCODE_OP_INT3: return OP_INT3;
        case RUST_OPCODE_OP_SFENCE: return OP_SFENCE;
        case RUST_OPCODE_OP_BSWAP: return OP_BSWAP;
        case RUST_OPCODE_OP_MOVAPS: return OP_MOVAPS;
        case RUST_OPCODE_OP_MOVDQA: return OP_MOVDQA;
        case RUST_OPCODE_OP_MOVNTDQ: return OP_MOVNTDQ;
        case RUST_OPCODE_OP_MOVSXD: return OP_MOVSXD;
        default: return OP_NONE;
    }
}

// Utility functions using Rust implementations
OPCODE transfer_str_to_op_rust(const std::string& op_str) {
    RustOpcode rust_opcode = rust_transfer_str_to_op(op_str.c_str());
    return rust_opcode_to_cpp_opcode(rust_opcode);
}

std::string transfer_op_to_str_rust(OPCODE opcode) {
    RustOpcode rust_opcode = cpp_opcode_to_rust_opcode(opcode);
    const char* str_ptr = rust_transfer_op_to_str(rust_opcode);
    return std::string(str_ptr);
}

REG get_reg_by_str_rust(const std::string& reg_str) {
    RustReg rust_reg = rust_get_reg_by_str(reg_str.c_str());
    return rust_reg_to_cpp_reg(rust_reg);
}

std::string get_reg_str_by_reg_rust(REG reg) {
    RustReg rust_reg = cpp_reg_to_rust_reg(reg);
    const char* str_ptr = rust_get_reg_str_by_reg(rust_reg);
    return std::string(str_ptr);
}