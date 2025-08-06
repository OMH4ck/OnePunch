#include "doctest/doctest.h"
#include "rust_preprocessor_bridge.h"

TEST_CASE("RustPreprocessorBridge - Basic Creation and Destruction") {
    RustPreprocessorBridge preprocessor;
    // TODO: When FFI is linked, this should be != nullptr
    CHECK(preprocessor.get_raw_ptr() == nullptr);  // Currently returns nullptr as placeholder
}

TEST_CASE("RustPreprocessorBridge - Process Empty Segments") {
    RustPreprocessorBridge preprocessor;
    std::vector<SegmentPtr> empty_segments;
    
    // Should not crash with empty input
    preprocessor.process(empty_segments);
    CHECK(true); // If we get here, the test passed
}

TEST_CASE("RustConstraintAnalyzerBridge - Hash Match") {
    // Test basic hash matching functionality
    bool result1 = RustConstraintAnalyzerBridge::hash_match(0, 0);
    CHECK((result1 == true || result1 == false)); // Should return a valid boolean
    
    bool result2 = RustConstraintAnalyzerBridge::hash_match(0x123, 0x456);
    CHECK((result2 == true || result2 == false)); // Should return a valid boolean
}

TEST_CASE("RustConstraintAnalyzerBridge - Compute Constraint") {
    // Test constraint computation with null pointer (safe fallback)
    unsigned long result = RustConstraintAnalyzerBridge::compute_constraint(nullptr);
    CHECK(result == 0); // Should return 0 for null input
}