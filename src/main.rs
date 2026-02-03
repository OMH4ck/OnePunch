//! OnePunch CLI - A tool for finding gadgets to achieve arbitrary code execution

use clap::Parser;
use onepunch::search::{parse_input_regs, parse_must_control_regs, OnePunch};

/// OnePunch - Find gadget chains for exploit development
#[derive(Parser)]
#[command(name = "OnePunch", version, about, long_about = None)]
struct Args {
    /// The registers that we control
    #[arg(short = 'i', long = "input", required = true, num_args = 1..)]
    input: Vec<String>,

    /// The registers we want to control (format: reg:level, e.g., rdi:1)
    #[arg(short = 'c', long = "control", required = true, num_args = 1..)]
    control: Vec<String>,

    /// The binary file to analyze
    #[arg(short = 'f', long = "file", required = true)]
    file: String,

    /// The search level (higher = more thorough but slower)
    #[arg(short = 'l', long = "level", default_value = "1")]
    level: u32,
}

fn print_usage_and_exit() -> ! {
    eprintln!("Example: ./OnePunch -i rdi rsi -c rsp:0 rbp:1 -f libc.so.6");
    eprintln!(
        "1 means we want to completely control the value of the register, and 0 means we allow \
         the register to be a pointer value as long as it can point to a buffer that we control."
    );
    std::process::exit(1);
}

fn main() {
    let args = Args::parse();

    let Some(reg_list) = parse_input_regs(&args.input) else {
        eprintln!("Error: Invalid input registers");
        print_usage_and_exit();
    };

    let Some(must_control_reg_list) = parse_must_control_regs(&args.control) else {
        eprintln!("Error: Invalid control registers");
        print_usage_and_exit();
    };

    let onepunch = OnePunch::new(args.file, reg_list, must_control_reg_list, args.level);
    onepunch.run();
}
