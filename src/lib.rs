//! OnePunch - A tool for finding gadgets to achieve arbitrary code execution
//!
//! This library provides functionality to analyze binaries and find gadget chains
//! that allow controlling target registers from a set of input registers.

pub mod asmutils;
pub mod core;
pub mod search;
pub mod types;
pub mod utils;

pub use asmutils::{get_call_segment, get_disasm_code};
pub use core::{execute_instructions, prepare_reg_list, Preprocessor};
pub use search::OnePunch;
pub use types::*;
