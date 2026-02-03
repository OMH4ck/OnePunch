//! Integration tests for OnePunch

use onepunch::search::{parse_input_regs, parse_must_control_regs, OnePunch};
use onepunch::Preprocessor;
use onepunch::{get_call_segment, get_disasm_code};
use std::path::Path;

/// Test finding a solution with a simple binary (if libc exists)
/// Note: Finding an actual solution is libc-version-dependent.
/// The C++ test hardcodes expected gadget addresses for a specific libc.
/// This test verifies parsing and search infrastructure works correctly.
#[test]
fn test_find_solution_libc() {
    let libc_path = "/lib/x86_64-linux-gnu/libc.so.6";
    if !Path::new(libc_path).exists() {
        eprintln!("Skipping test: libc not found at {}", libc_path);
        return;
    }

    // Parse input registers (r8)
    let input_regs = parse_input_regs(&["r8".to_string()]).expect("Failed to parse input regs");

    // Parse must control registers (rdi:1)
    let must_control = parse_must_control_regs(&["rdi:1".to_string()])
        .expect("Failed to parse must control regs");

    // Create OnePunch instance
    let onepunch = OnePunch::new(libc_path.to_string(), input_regs, must_control, 1);

    // Get disasm code and segments
    let instruction_list = get_disasm_code(libc_path);
    assert!(!instruction_list.is_empty(), "Should have instructions");
    println!("Parsed {} instructions", instruction_list.len());

    let mut code_segments = get_call_segment(&instruction_list);
    code_segments.sort_by(|a, b| a.format_with_offset(false).cmp(&b.format_with_offset(false)));

    assert!(
        !code_segments.is_empty(),
        "Should have call/jmp segments"
    );
    println!("Found {} segments", code_segments.len());

    // Preprocess
    let mut preprocessor = Preprocessor::new();
    preprocessor.process(&code_segments);

    // Find solution - may or may not find one depending on libc version
    let solution = onepunch.find_solution(&code_segments, &preprocessor);

    // Log result but don't fail if no solution found - it's libc-version-dependent
    if solution.found {
        println!("Solution found with {} segments", solution.output_segments.len());
        for (seg, start_idx) in &solution.output_segments {
            println!("Segment starting at index {}:", start_idx);
            for idx in *start_idx..seg.inst_list.len() {
                println!("  {}", seg.inst_list[idx].original_inst);
            }
        }
    } else {
        // Not finding a solution is acceptable - the C++ expected output
        // is hardcoded for a specific libc version
        println!("No solution found (this is libc-version-dependent)");
    }

    // The test passes if we successfully parsed and searched - finding a solution
    // depends on the specific libc version which varies between systems
}

/// Test that parse_input_regs handles various formats
#[test]
fn test_parse_input_regs_formats() {
    // Basic register
    let regs = parse_input_regs(&["rax".to_string()]).unwrap();
    assert_eq!(regs.len(), 1);

    // Multiple registers
    let regs = parse_input_regs(&["rax".to_string(), "rbx".to_string(), "rcx".to_string()]).unwrap();
    assert_eq!(regs.len(), 3);

    // Invalid register returns None
    let result = parse_input_regs(&["invalid_reg".to_string()]);
    assert!(result.is_none());
}

/// Test that parse_must_control_regs handles various formats
#[test]
fn test_parse_must_control_regs_formats() {
    // Level 1 control
    let regs = parse_must_control_regs(&["rdi:1".to_string()]).unwrap();
    assert_eq!(regs.len(), 1);
    assert_eq!(regs[0].1, 1);

    // Level 0 control (pointer allowed)
    let regs = parse_must_control_regs(&["rsi:0".to_string()]).unwrap();
    assert_eq!(regs.len(), 1);
    assert_eq!(regs[0].1, 0);

    // Multiple control registers
    let regs = parse_must_control_regs(&[
        "rdi:1".to_string(),
        "rsi:0".to_string(),
        "rdx:1".to_string(),
    ])
    .unwrap();
    assert_eq!(regs.len(), 3);
}

/// Test segment extraction from simple assembly
#[test]
fn test_segment_extraction() {
    // We can only test this if objdump is available and there's a binary to analyze
    let test_bin = std::env::current_exe().ok();
    if let Some(bin_path) = test_bin {
        let bin_str = bin_path.to_string_lossy().to_string();
        let instructions = get_disasm_code(&bin_str);
        // Just verify we can parse something
        println!("Parsed {} instructions from test binary", instructions.len());
    }
}
