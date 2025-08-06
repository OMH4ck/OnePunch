#include "../include/rust_utils_bridge.h"
#include <cstring>

std::string transfer_operation_len_to_str_rust(unsigned dtype) {
    const char* str_ptr = rust_transfer_operation_len_to_str(dtype);
    return std::string(str_ptr);
}

unsigned long fuck_hash_rust(const std::string& str) {
    return rust_string_hash(str.c_str());
}

unsigned long gen_id_rust() {
    return rust_gen_id();
}

bool is_imm_rust(const std::string& str) {
    return rust_is_imm(str.c_str()) != 0;
}

std::vector<std::string> str_split_rust(const std::string& str, const std::string& delimiter) {
    std::vector<std::string> result;
    const size_t MAX_PARTS = 256; // Reasonable limit
    char* parts[MAX_PARTS];
    
    unsigned int count = rust_str_split(str.c_str(), delimiter.c_str(), parts, MAX_PARTS);
    
    for (unsigned int i = 0; i < count; ++i) {
        if (parts[i] != nullptr) {
            result.emplace_back(parts[i]);
            rust_free_string(parts[i]); // Clean up Rust-allocated string
        }
    }
    
    return result;
}

std::string str_trim_rust(const std::string& str) {
    char* trimmed = rust_str_trim(str.c_str());
    if (trimmed == nullptr) {
        return str; // Fallback to original string
    }
    
    std::string result(trimmed);
    rust_free_string(trimmed); // Clean up Rust-allocated string
    return result;
}

double get_cur_time_rust() {
    return rust_get_cur_time();
}