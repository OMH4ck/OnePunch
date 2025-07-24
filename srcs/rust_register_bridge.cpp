#include "../include/rust_register_bridge.h"
#include <stdexcept>

RustRegisterBridge::RustRegisterBridge(bool alloc_mem) {
    rust_register_ = rust_register_new(alloc_mem ? 1 : 0);
    if (!rust_register_) {
        throw std::runtime_error("Failed to create Rust register");
    }
}

RustRegisterBridge::~RustRegisterBridge() {
    if (rust_register_) {
        rust_register_free(rust_register_);
    }
}

void RustRegisterBridge::set_name(REG name) {
    RustRegType rust_reg = cpp_reg_to_rust_reg(name);
    rust_register_set_name(rust_register_, rust_reg);
}

REG RustRegisterBridge::get_name() const {
    RustRegType rust_reg = rust_register_get_name(rust_register_);
    return rust_reg_to_cpp_reg(rust_reg);
}

void RustRegisterBridge::alias(const RustRegisterBridge& other, bool copy_mem) {
    rust_register_alias(rust_register_, other.rust_register_, copy_mem ? 1 : 0);
}

RustRegType RustRegisterBridge::cpp_reg_to_rust_reg(REG cpp_reg) {
    switch (cpp_reg) {
        case REG_NONE: return RUST_REG_TYPE_REG_NONE;
        case REG_RAX: return RUST_REG_TYPE_REG_RAX;
        case REG_RCX: return RUST_REG_TYPE_REG_RCX;
        case REG_RDX: return RUST_REG_TYPE_REG_RDX;
        case REG_RBX: return RUST_REG_TYPE_REG_RBX;
        case REG_RSI: return RUST_REG_TYPE_REG_RSI;
        case REG_RDI: return RUST_REG_TYPE_REG_RDI;
        case REG_RSP: return RUST_REG_TYPE_REG_RSP;
        case REG_RBP: return RUST_REG_TYPE_REG_RBP;
        case REG_R8: return RUST_REG_TYPE_REG_R8;
        case REG_R9: return RUST_REG_TYPE_REG_R9;
        case REG_R10: return RUST_REG_TYPE_REG_R10;
        case REG_R11: return RUST_REG_TYPE_REG_R11;
        case REG_R12: return RUST_REG_TYPE_REG_R12;
        case REG_R13: return RUST_REG_TYPE_REG_R13;
        case REG_R14: return RUST_REG_TYPE_REG_R14;
        case REG_R15: return RUST_REG_TYPE_REG_R15;
        case REG_RIP: return RUST_REG_TYPE_REG_RIP;
        default: return RUST_REG_TYPE_REG_NONE;
    }
}

REG RustRegisterBridge::rust_reg_to_cpp_reg(RustRegType rust_reg) {
    switch (rust_reg) {
        case RUST_REG_TYPE_REG_NONE: return REG_NONE;
        case RUST_REG_TYPE_REG_RAX: return REG_RAX;
        case RUST_REG_TYPE_REG_RCX: return REG_RCX;
        case RUST_REG_TYPE_REG_RDX: return REG_RDX;
        case RUST_REG_TYPE_REG_RBX: return REG_RBX;
        case RUST_REG_TYPE_REG_RSI: return REG_RSI;
        case RUST_REG_TYPE_REG_RDI: return REG_RDI;
        case RUST_REG_TYPE_REG_RSP: return REG_RSP;
        case RUST_REG_TYPE_REG_RBP: return REG_RBP;
        case RUST_REG_TYPE_REG_R8: return REG_R8;
        case RUST_REG_TYPE_REG_R9: return REG_R9;
        case RUST_REG_TYPE_REG_R10: return REG_R10;
        case RUST_REG_TYPE_REG_R11: return REG_R11;
        case RUST_REG_TYPE_REG_R12: return REG_R12;
        case RUST_REG_TYPE_REG_R13: return REG_R13;
        case RUST_REG_TYPE_REG_R14: return REG_R14;
        case RUST_REG_TYPE_REG_R15: return REG_R15;
        case RUST_REG_TYPE_REG_RIP: return REG_RIP;
        default: return REG_NONE;
    }
}

RustValue RustRegisterBridge::cpp_value_to_rust_value(const Value& val) {
    RustValueType rust_type;
    switch (val.type_) {
        case kCallValue: rust_type = RUST_VALUE_TYPE_CALL_VALUE; break;
        case kMemValue: rust_type = RUST_VALUE_TYPE_MEM_VALUE; break;
        case kCallRegValue: rust_type = RUST_VALUE_TYPE_CALL_REG_VALUE; break;
        case kImmValue: rust_type = RUST_VALUE_TYPE_IMM_VALUE; break;
        case kOtherValue: rust_type = RUST_VALUE_TYPE_OTHER_VALUE; break;
        default: rust_type = RUST_VALUE_TYPE_OTHER_VALUE; break;
    }
    
    RustValue rust_val;
    rust_val.value_type = rust_type;
    rust_val.value = val.value_;
    return rust_val;
}

Value RustRegisterBridge::rust_value_to_cpp_value(const RustValue& val) {
    VALUETYPE cpp_type;
    switch (val.value_type) {
        case RUST_VALUE_TYPE_CALL_VALUE: cpp_type = kCallValue; break;
        case RUST_VALUE_TYPE_MEM_VALUE: cpp_type = kMemValue; break;
        case RUST_VALUE_TYPE_CALL_REG_VALUE: cpp_type = kCallRegValue; break;
        case RUST_VALUE_TYPE_IMM_VALUE: cpp_type = kImmValue; break;
        case RUST_VALUE_TYPE_OTHER_VALUE: cpp_type = kOtherValue; break;
        default: cpp_type = kOtherValue; break;
    }
    
    return Value(cpp_type, val.value);
}