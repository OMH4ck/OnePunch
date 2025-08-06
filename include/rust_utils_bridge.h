#ifndef RUST_UTILS_BRIDGE_H_
#define RUST_UTILS_BRIDGE_H_

#include "utils.h"
#include <vector>
extern "C" {
#include "../rust-onepunch/bindings.h"
}

// Utility functions using Rust implementations
std::string transfer_operation_len_to_str_rust(unsigned dtype);
unsigned long fuck_hash_rust(const std::string& str);
unsigned long gen_id_rust();
bool is_imm_rust(const std::string& str);
std::vector<std::string> str_split_rust(const std::string& str, const std::string& delimiter);
std::string str_trim_rust(const std::string& str);
double get_cur_time_rust();

#endif  // RUST_UTILS_BRIDGE_H_