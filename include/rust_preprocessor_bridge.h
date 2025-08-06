#ifndef RUST_PREPROCESSOR_BRIDGE_H_
#define RUST_PREPROCESSOR_BRIDGE_H_

#include "../rust-onepunch/bindings.h"
#include <vector>
#include <map>
#include "asmutils.h"

class RustPreprocessorBridge {
private:
    RustPreprocessor* rust_preprocessor_;

public:
    // Constructor
    explicit RustPreprocessorBridge();
    
    // Destructor
    ~RustPreprocessorBridge();
    
    // Copy constructor and assignment (delete to prevent issues)
    RustPreprocessorBridge(const RustPreprocessorBridge&) = delete;
    RustPreprocessorBridge& operator=(const RustPreprocessorBridge&) = delete;
    
    // Process segments to compute constraints
    void process(const std::vector<SegmentPtr>& segments);
    
    // Get the raw Rust pointer for advanced usage
    RustPreprocessor* get_raw_ptr() const { return rust_preprocessor_; }
};

// Static constraint analyzer functions
class RustConstraintAnalyzerBridge {
public:
    static unsigned long compute_constraint(const SegmentPtr segment);
    static bool hash_match(unsigned long needed, unsigned long src);
};

#endif  // RUST_PREPROCESSOR_BRIDGE_H_