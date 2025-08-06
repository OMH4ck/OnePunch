#include <doctest/doctest.h>
#include "../../include/rust_utils_bridge.h"
#include <vector>

TEST_CASE("Rust Utils Basic Test") {
    // Test operation length conversion
    std::string byte_str = transfer_operation_len_to_str_rust(1);
    CHECK(byte_str == "BYTE");
    
    std::string qword_str = transfer_operation_len_to_str_rust(8);
    CHECK(qword_str == "QWORD");
    
    std::string none_str = transfer_operation_len_to_str_rust(0);
    CHECK(none_str == "NONE");
}

TEST_CASE("Rust Hash Function Test") {
    // Test string hashing
    unsigned long hash1 = fuck_hash_rust("test");
    unsigned long hash2 = fuck_hash_rust("test");
    unsigned long hash3 = fuck_hash_rust("different");
    
    CHECK(hash1 == hash2);  // Same string should produce same hash
    CHECK(hash1 != hash3);  // Different strings should produce different hashes
}

TEST_CASE("Rust Immediate Value Test") {
    // Test immediate value detection
    CHECK(is_imm_rust("123") == true);
    CHECK(is_imm_rust("-456") == true);
    CHECK(is_imm_rust("0x1a2b") == true);
    CHECK(is_imm_rust("0X1A2B") == true);
    CHECK(is_imm_rust("abc") == false);
    CHECK(is_imm_rust("") == false);
    CHECK(is_imm_rust("12.34") == false);
}

TEST_CASE("Rust String Split Test") {
    // Test string splitting
    std::vector<std::string> result = str_split_rust("a,b,c", ",");
    CHECK(result.size() == 3);
    CHECK(result[0] == "a");
    CHECK(result[1] == "b");  
    CHECK(result[2] == "c");
    
    std::vector<std::string> result2 = str_split_rust("hello world", " ");
    CHECK(result2.size() == 2);
    CHECK(result2[0] == "hello");
    CHECK(result2[1] == "world");
}

TEST_CASE("Rust String Trim Test") {
    // Test string trimming
    CHECK(str_trim_rust("  hello  ") == "hello");
    CHECK(str_trim_rust("\t\nworld\t\n") == "world");
    CHECK(str_trim_rust("notrim") == "notrim");
}

TEST_CASE("Rust ID Generation Test") {
    // Test ID generation
    unsigned long id1 = gen_id_rust();
    unsigned long id2 = gen_id_rust();
    CHECK(id2 > id1);  // IDs should be increasing
}

TEST_CASE("Rust Time Function Test") {
    // Test time function
    double time = get_cur_time_rust();
    CHECK(time > 0.0);  // Should return a positive timestamp
}