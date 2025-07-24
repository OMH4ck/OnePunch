#ifndef RUST_REGISTER_BRIDGE_H_
#define RUST_REGISTER_BRIDGE_H_

#include "register.h"
extern "C" {
#include "../rust-onepunch/bindings.h"
}

class RustRegisterBridge {
private:
    RustRegister* rust_register_;
    
public:
    RustRegisterBridge(bool alloc_mem = true);
    ~RustRegisterBridge();
    
    // Bridge methods that wrap Rust FFI calls
    void set_name(REG name);
    REG get_name() const;
    void alias(const RustRegisterBridge& other, bool copy_mem = true);
    
    // Conversion methods
    static RustRegType cpp_reg_to_rust_reg(REG cpp_reg);
    static REG rust_reg_to_cpp_reg(RustRegType rust_reg);
    
    // Access to underlying Rust register
    RustRegister* get_rust_register() const { return rust_register_; }
    
    // Bridge memory operations
    bool contain_range(const std::pair<long, long>& range);
    bool remove_range(const std::pair<long, long>& range);
    void set_content(long offset, const Value& val);
    
private:
    RustValue cpp_value_to_rust_value(const Value& val);
    Value rust_value_to_cpp_value(const RustValue& val);
};

#endif  // RUST_REGISTER_BRIDGE_H_