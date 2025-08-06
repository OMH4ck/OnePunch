#ifndef RUST_SOLVER_BRIDGE_H_
#define RUST_SOLVER_BRIDGE_H_

#include "../rust-onepunch/bindings.h"
#include <vector>
#include <list>
#include "asmutils.h"
#include "register.h"
#include "preprocessor.h"
#include "onepunch.h"

namespace onepunch {

class RustSolverBridge {
private:
    RustSolver* rust_solver_;

public:
    // Constructor
    RustSolverBridge(
        std::vector<SegmentPtr>& code_segments,
        const std::vector<std::pair<REG, int>>& must_control_list,
        const std::list<RegisterPtr>& reg_list,
        unsigned long search_level,
        const Preprocessor& preprocessor
    );
    
    // Destructor
    ~RustSolverBridge();
    
    // Copy constructor and assignment (delete to prevent issues)
    RustSolverBridge(const RustSolverBridge&) = delete;
    RustSolverBridge& operator=(const RustSolverBridge&) = delete;
    
    // Perform depth-first search to find gadget chain
    bool Dfs(std::list<RegisterPtr>& output_register,
             std::vector<std::pair<SegmentPtr, unsigned>>& output_segments);
    
    // Get the raw Rust pointer for advanced usage
    RustSolver* get_raw_ptr() const { return rust_solver_; }
};

// Static utility functions
class RustSolverUtilsBridge {
public:
    static unsigned long hash_reg_list(const std::list<RegisterPtr>& reg_list);
    static bool is_solution(
        const std::vector<std::pair<REG, int>>& must_control_list,
        const std::list<RegisterPtr>& reg_list
    );
};

}  // namespace onepunch

#endif  // RUST_SOLVER_BRIDGE_H_