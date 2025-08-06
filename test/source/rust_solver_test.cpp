#include "doctest/doctest.h"
#include "rust_solver_bridge.h"
#include <vector>
#include <list>

using namespace onepunch;

TEST_CASE("RustSolverBridge - Basic Creation and Destruction") {
    std::vector<SegmentPtr> code_segments;
    std::vector<std::pair<REG, int>> must_control_list = {{REG_RAX, 1}};
    std::list<RegisterPtr> reg_list;
    Preprocessor preprocessor;
    
    RustSolverBridge solver(code_segments, must_control_list, reg_list, 1, preprocessor);
    
    // TODO: When FFI is linked, this should be != nullptr
    CHECK(solver.get_raw_ptr() == nullptr);  // Currently returns nullptr as placeholder
}

TEST_CASE("RustSolverBridge - DFS with Empty Input") {
    std::vector<SegmentPtr> code_segments;
    std::vector<std::pair<REG, int>> must_control_list = {{REG_RAX, 1}};
    std::list<RegisterPtr> reg_list;
    Preprocessor preprocessor;
    
    RustSolverBridge solver(code_segments, must_control_list, reg_list, 1, preprocessor);
    
    std::list<RegisterPtr> output_register;
    std::vector<std::pair<SegmentPtr, unsigned>> output_segments;
    
    // Should return false with empty input
    bool result = solver.Dfs(output_register, output_segments);
    CHECK(result == false);
    CHECK(output_register.empty());
    CHECK(output_segments.empty());
}

TEST_CASE("RustSolverUtilsBridge - Hash Empty Register List") {
    std::list<RegisterPtr> empty_reg_list;
    
    unsigned long hash = RustSolverUtilsBridge::hash_reg_list(empty_reg_list);
    
    // Empty list should hash to 0
    CHECK(hash == 0);
}

TEST_CASE("RustSolverUtilsBridge - Is Solution with Empty Lists") {
    std::vector<std::pair<REG, int>> must_control_list = {{REG_RAX, 1}};
    std::list<RegisterPtr> empty_reg_list;
    
    bool result = RustSolverUtilsBridge::is_solution(must_control_list, empty_reg_list);
    
    // Should return false since we can't satisfy the constraint with an empty register list
    CHECK(result == false);
}

TEST_CASE("RustSolverUtilsBridge - Is Solution with Empty Constraints") {
    std::vector<std::pair<REG, int>> empty_constraints;
    std::list<RegisterPtr> empty_reg_list;
    
    bool result = RustSolverUtilsBridge::is_solution(empty_constraints, empty_reg_list);
    
    // Should return false (placeholder behavior)
    CHECK(result == false);
}