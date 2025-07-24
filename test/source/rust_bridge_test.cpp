#include <doctest/doctest.h>
#include "../../include/rust_register_bridge.h"

TEST_CASE("Rust Register Bridge Basic Test") {
    RustRegisterBridge reg(true);
    
    // Test setting and getting register name
    reg.set_name(REG_RAX);
    CHECK(reg.get_name() == REG_RAX);
    
    // Test setting a different register
    reg.set_name(REG_RDI);
    CHECK(reg.get_name() == REG_RDI);
}

TEST_CASE("Rust Register Bridge Alias Test") {
    RustRegisterBridge reg1(true);
    RustRegisterBridge reg2(true);
    
    reg1.set_name(REG_R8);
    reg2.alias(reg1, true);
    
    CHECK(reg2.get_name() == REG_R8);
}

TEST_CASE("Rust Value Conversion Test") {
    // Test that we can create a register and access its underlying pointer
    RustRegisterBridge reg(true);
    CHECK(reg.get_rust_register() != nullptr);
}