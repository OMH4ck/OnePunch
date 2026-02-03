//! Search module - DFS search, minimization, and output

use crate::asmutils::{get_reg_by_str, locate_next_inst_addr};
use crate::core::*;
use crate::types::*;
use crate::utils::{get_cur_time, str_split};
use std::collections::HashSet;
use std::sync::Arc;

/// Check if current state is a solution
pub fn is_solution(must_control_list: &[(Reg, i32)], reg_list: &[Register]) -> bool {
    for (reg, level) in must_control_list {
        if !is_in_input(*reg, reg_list) {
            return false;
        }
        if *level == 1 && !is_independent(*reg, reg_list) {
            return false;
        }
    }
    true
}

/// DFS search for gadget chains
pub fn dfs(
    code_segments: &[SegmentPtr],
    preprocessor: &Preprocessor,
    must_control_list: &[(Reg, i32)],
    reg_list: &[Register],
    output_register: &mut Vec<Register>,
    output_segments: &mut Vec<(SegmentPtr, usize)>,
    search_level: u32,
    visited: &mut HashSet<u64>,
) -> bool {
    let tmp_h = hash_reg_list(reg_list);
    if search_level == 1 {
        if visited.contains(&tmp_h) {
            return false;
        }
        visited.insert(tmp_h);
    }

    let save_rsp_usable = is_rsp_usable();
    let save_rbp_usable = is_rbp_usable();

    for (seg_idx, segment) in code_segments.iter().enumerate() {
        if search_level <= 2 && !hash_match(preprocessor.get_constraint(seg_idx), tmp_h) {
            continue;
        }

        let mut seg_clone = Segment::new(segment.inst_list.clone());
        seg_clone.useful_inst_index = 0;
        let start_index = remove_useless_instructions(&mut seg_clone, reg_list);

        if segment.inst_list.len() - seg_clone.useful_inst_index < 2 {
            continue;
        }

        // Recompute constraint after removing useless instructions
        let seg_arc = Arc::new(seg_clone.clone());
        if search_level <= 2 && !hash_match(compute_constraint(&seg_arc), tmp_h) {
            continue;
        }

        let mut tmp_reg_list = copy_reg_list(reg_list);
        set_rsp_usable(save_rsp_usable);
        set_rbp_usable(save_rbp_usable);

        if !execute_instructions(&seg_clone, &mut tmp_reg_list, false) {
            continue;
        }

        if tmp_reg_list.len() > 16 {
            continue;
        }

        // Check solution before size
        if is_solution(must_control_list, &tmp_reg_list) {
            output_segments.push((Arc::clone(segment), start_index));
            *output_register = tmp_reg_list;
            return true;
        }

        if tmp_reg_list.len() > reg_list.len() {
            output_segments.push((Arc::clone(segment), start_index));

            if dfs(
                code_segments,
                preprocessor,
                must_control_list,
                &tmp_reg_list,
                output_register,
                output_segments,
                search_level,
                visited,
            ) {
                return true;
            }
            output_segments.pop();
        }
    }
    false
}

/// Run a segment list and update registers
fn run_segment_list(run_code: &[(SegmentPtr, usize)], registers: &mut Vec<Register>) -> bool {
    for (segment, start_idx) in run_code {
        let mut seg_clone = Segment::new(segment.inst_list.clone());
        seg_clone.useful_inst_index = *start_idx;
        if !execute_instructions(&seg_clone, registers, false) {
            return false;
        }
    }
    true
}

/// Recursive minimization helper
fn minimize_segment_nb(
    sol_register: &mut Vec<Register>,
    sol_segments: &mut Vec<(SegmentPtr, usize)>,
    input_regs: &[Register],
    must_control_list: &[(Reg, i32)],
    idx: usize,
    run_code: &mut Vec<(SegmentPtr, usize)>,
    orig_segments: &[(SegmentPtr, usize)],
) {
    for i in idx..orig_segments.len() {
        run_code.push(orig_segments[i].clone());
        let mut registers = copy_reg_list(input_regs);
        if !run_segment_list(run_code, &mut registers) {
            run_code.pop();
            continue;
        }
        if is_solution(must_control_list, &registers) && sol_segments.len() > run_code.len() {
            *sol_segments = run_code.clone();
            *sol_register = registers.clone();
        }

        minimize_segment_nb(
            sol_register,
            sol_segments,
            input_regs,
            must_control_list,
            i + 1,
            run_code,
            orig_segments,
        );
        run_code.pop();
    }
}

/// Minimize segments
fn minimize_segment(
    sol_register: &mut Vec<Register>,
    sol_segments: &mut Vec<(SegmentPtr, usize)>,
    input_regs: &[Register],
    must_control_list: &[(Reg, i32)],
) -> bool {
    let solution_size = sol_segments.len();
    let mut tmp = Vec::new();
    let orig_segment = sol_segments.clone();
    minimize_segment_nb(
        sol_register,
        sol_segments,
        input_regs,
        must_control_list,
        0,
        &mut tmp,
        &orig_segment,
    );
    solution_size != sol_segments.len()
}

/// Minimize instructions within segments
fn minimize_instruction(
    sol_register: &mut Vec<Register>,
    sol_segments: &mut Vec<(SegmentPtr, usize)>,
    input_regs: &[Register],
    must_control_list: &[(Reg, i32)],
) -> bool {
    let mut is_optimized = false;
    for seg_idx in 0..sol_segments.len() {
        let max_inst_size = sol_segments[seg_idx].0.inst_list.len();
        let mut segment_inst_index = sol_segments[seg_idx].1;

        for i in (segment_inst_index + 1)..max_inst_size {
            sol_segments[seg_idx].1 = i;
            let mut registers = copy_reg_list(input_regs);
            if !run_segment_list(sol_segments, &mut registers) {
                continue;
            }
            if is_solution(must_control_list, &registers) {
                segment_inst_index = i;
                *sol_register = registers;
                is_optimized = true;
            }
        }
        sol_segments[seg_idx].1 = segment_inst_index;
    }
    is_optimized
}

/// Minimize result (both segments and instructions)
pub fn minimize_result(
    sol_register: &mut Vec<Register>,
    sol_segments: &mut Vec<(SegmentPtr, usize)>,
    input_regs: &[Register],
    must_control_list: &[(Reg, i32)],
) {
    loop {
        let segment_minimize =
            minimize_segment(sol_register, sol_segments, input_regs, must_control_list);
        let deep_minimize =
            minimize_instruction(sol_register, sol_segments, input_regs, must_control_list);
        if !segment_minimize && !deep_minimize {
            break;
        }
    }
}

/// Match and print memory layout
pub fn match_and_print(
    code_segments: &[(SegmentPtr, usize)],
    must_control_list: &[(Reg, i32)],
    reg_list: &[Register],
) {
    MEM_LIST.with(|mem_list| {
        let mem_list = mem_list.borrow_mut();

        for mem in mem_list.iter() {
            let mut mem = mem.borrow_mut();
            let keys: Vec<i64> = mem.content.keys().copied().collect();

            for key in keys {
                if let Some(value) = mem.content.get(&key).cloned() {
                    match value.value_type {
                        ValueType::CallValue => {
                            let real_addr =
                                locate_next_inst_addr(value.value as u64, code_segments);
                            mem.content.insert(
                                key,
                                Value::new(ValueType::CallValue, real_addr as i64),
                            );
                        }
                        ValueType::CallRegValue => {
                            let tmp_mem_id = (value.value >> 32) as u32;
                            let tmp_inst_offset = (value.value & 0xffffffff) as u64;

                            for other_mem in mem_list.iter() {
                                let mut other_mem = other_mem.borrow_mut();
                                let other_keys: Vec<i64> =
                                    other_mem.content.keys().copied().collect();
                                for key2 in other_keys {
                                    if let Some(value2) = other_mem.content.get(&key2).cloned() {
                                        if value2.value_type == ValueType::MemValue
                                            && value2.value as u32 == tmp_mem_id
                                        {
                                            let real_addr =
                                                locate_next_inst_addr(tmp_inst_offset, code_segments);
                                            other_mem.content.insert(
                                                key2,
                                                Value::new(
                                                    ValueType::CallRegValue,
                                                    real_addr as i64,
                                                ),
                                            );
                                        }
                                    }
                                }
                            }
                        }
                        ValueType::MemValue => {
                            for (reg_name, _) in must_control_list {
                                if let Some(reg) = reg_list.iter().find(|r| r.name == *reg_name) {
                                    if reg.mem.borrow().mem_id as i64 == value.value {
                                        mem.content.insert(
                                            key,
                                            Value::new(
                                                ValueType::MemValue,
                                                reg.mem.borrow().mem_id as i64,
                                            ),
                                        );
                                    }
                                }
                            }
                        }
                        _ => {}
                    }
                }
            }
        }
    });
}

/// Record memory state
pub fn record_memory(
    reg_names: &[Reg],
    code_segments: &mut [(SegmentPtr, usize)],
    must_control_list: &[(Reg, i32)],
) {
    RECORD_MEM.with(|v| *v.borrow_mut() = true);
    let mut reg_list = prepare_reg_list(reg_names);

    for (segment, start_idx) in code_segments.iter() {
        let mut seg_clone = Segment::new(segment.inst_list.clone());
        seg_clone.useful_inst_index = *start_idx;
        execute_instructions(&seg_clone, &mut reg_list, true);
    }

    RECORD_MEM.with(|v| *v.borrow_mut() = false);

    match_and_print(code_segments, must_control_list, &reg_list);

    println!("\nMemory list:");
    MEM_LIST.with(|mem_list| {
        for mem in mem_list.borrow().iter() {
            println!("{}", mem.borrow().to_string());
        }
    });

    println!("\nFinal state");
    for reg in &reg_list {
        println!("{}", reg.to_string());
    }

    // Clear memory list
    MEM_LIST.with(|mem_list| mem_list.borrow_mut().clear());
}

/// Parse input registers from string
pub fn parse_input_regs(input_regs: &[String]) -> Option<Vec<Register>> {
    let mut control_reg_for_prepare = Vec::new();
    let mut control_reg_remove_ranges: std::collections::HashMap<Reg, Vec<(i64, i64)>> =
        std::collections::HashMap::new();

    for i in input_regs {
        let tmp_vec = str_split(i, ":");
        let r_name = &tmp_vec[0];
        let reg = get_reg_by_str(r_name)?;

        for idx in 1..tmp_vec.len() {
            let range = str_split(&tmp_vec[idx], "-");
            if range.len() != 2 {
                continue;
            }
            let start: i64 = range[0].parse().ok()?;
            let end: i64 = range[1].parse().ok()?;
            control_reg_remove_ranges
                .entry(reg)
                .or_default()
                .push((start, end));
        }
        control_reg_for_prepare.push(reg);
    }

    let mut reg_list = prepare_reg_list(&control_reg_for_prepare);
    for reg in &mut reg_list {
        if let Some(ranges) = control_reg_remove_ranges.get(&reg.name) {
            for range in ranges {
                reg.remove_range(range);
            }
        }
    }
    Some(reg_list)
}

/// Parse must control registers from string
pub fn parse_must_control_regs(must_control_list: &[String]) -> Option<Vec<(Reg, i32)>> {
    let mut result = Vec::new();

    for i in must_control_list {
        let a = str_split(i, ":");
        if a.len() != 2 {
            return None;
        }
        let reg = get_reg_by_str(&a[0])?;
        let level = if a[1].starts_with('1') { 1 } else { 0 };
        result.push((reg, level));
    }
    Some(result)
}

/// Main OnePunch struct
pub struct OnePunch {
    input_file: String,
    input_regs: Vec<Register>,
    must_control_list: Vec<(Reg, i32)>,
    search_level: u32,
}

impl OnePunch {
    pub fn new(
        input_file: String,
        input_regs: Vec<Register>,
        must_control_list: Vec<(Reg, i32)>,
        search_level: u32,
    ) -> Self {
        Self {
            input_file,
            input_regs,
            must_control_list,
            search_level,
        }
    }

    /// Find a solution
    pub fn find_solution(&self, code_segments: &[SegmentPtr], preprocessor: &Preprocessor) -> Solution {
        let mut sol = Solution::default();
        let mut visited = HashSet::new();
        sol.found = dfs(
            code_segments,
            preprocessor,
            &self.must_control_list,
            &self.input_regs,
            &mut sol.output_reg_list,
            &mut sol.output_segments,
            self.search_level,
            &mut visited,
        );
        sol
    }

    /// Minimize a solution
    pub fn minimize_solution(&self, solution: &mut Solution) {
        solution.minimized_reg_list = solution.output_reg_list.clone();
        minimize_result(
            &mut solution.minimized_reg_list,
            &mut solution.output_segments,
            &self.input_regs,
            &self.must_control_list,
        );
    }

    /// Record memory stage
    pub fn record_memory_stage(&self, solution: &mut Solution) {
        let controlled_regs: Vec<Reg> = self.input_regs.iter().map(|r| r.name).collect();
        record_memory(
            &controlled_regs,
            &mut solution.output_segments,
            &self.must_control_list,
        );
    }

    /// Run the full analysis
    pub fn run(&self) {
        let t_start = get_cur_time();
        let instruction_list = crate::asmutils::get_disasm_code(&self.input_file);
        let mut code_segments = crate::asmutils::get_call_segment(&instruction_list);

        // Sort segments
        code_segments.sort_by(|a, b| a.format_with_offset(false).cmp(&b.format_with_offset(false)));

        println!("Segment size: {}", code_segments.len());
        println!("Collect segment time: {}", get_cur_time() - t_start);
        let t_start = get_cur_time();

        let mut preprocessor = Preprocessor::new();
        preprocessor.process(&code_segments);
        println!("Preprocess time: {}", get_cur_time() - t_start);
        let _t_start = get_cur_time();

        let mut solution = self.find_solution(&code_segments, &preprocessor);

        if !solution.found {
            println!("No solution found!");
            return;
        }

        println!("Solution found!");
        for (seg, start_idx) in &solution.output_segments {
            for idx in *start_idx..seg.inst_list.len() {
                println!("{}", seg.inst_list[idx].original_inst);
            }
            println!("------");
        }

        self.minimize_solution(&mut solution);

        println!("after minimize:");
        for (seg, start_idx) in &solution.output_segments {
            for idx in *start_idx..seg.inst_list.len() {
                println!("{}", seg.inst_list[idx].original_inst);
            }
            println!("------");
        }

        self.record_memory_stage(&mut solution);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_is_solution_empty() {
        let regs = prepare_reg_list(&[Reg::Rax]);
        assert!(is_solution(&[], &regs));
    }

    #[test]
    fn test_is_solution_with_control() {
        let regs = prepare_reg_list(&[Reg::Rax, Reg::Rdi]);
        assert!(is_solution(&[(Reg::Rdi, 1)], &regs));
    }

    #[test]
    fn test_is_solution_missing_reg() {
        let regs = prepare_reg_list(&[Reg::Rax]);
        assert!(!is_solution(&[(Reg::Rdi, 1)], &regs));
    }

    #[test]
    fn test_parse_input_regs() {
        let result = parse_input_regs(&["rax".to_string(), "rbx".to_string()]);
        assert!(result.is_some());
        let regs = result.unwrap();
        assert_eq!(regs.len(), 2);
        assert_eq!(regs[0].name, Reg::Rax);
        assert_eq!(regs[1].name, Reg::Rbx);
    }

    #[test]
    fn test_parse_input_regs_invalid() {
        let result = parse_input_regs(&["invalid".to_string()]);
        assert!(result.is_none());
    }

    #[test]
    fn test_parse_must_control_regs() {
        let result = parse_must_control_regs(&["rdi:1".to_string(), "rsi:0".to_string()]);
        assert!(result.is_some());
        let regs = result.unwrap();
        assert_eq!(regs.len(), 2);
        assert_eq!(regs[0], (Reg::Rdi, 1));
        assert_eq!(regs[1], (Reg::Rsi, 0));
    }

    #[test]
    fn test_parse_must_control_regs_invalid() {
        let result = parse_must_control_regs(&["invalid:1".to_string()]);
        assert!(result.is_none());
    }

    #[test]
    fn test_onepunch_new() {
        let input_regs = parse_input_regs(&["r8".to_string()]).unwrap();
        let must_control = parse_must_control_regs(&["rdi:1".to_string()]).unwrap();
        let onepunch = OnePunch::new("test".to_string(), input_regs, must_control, 1);
        assert_eq!(onepunch.input_file, "test");
        assert_eq!(onepunch.search_level, 1);
    }

    #[test]
    fn test_run_segment_list_empty() {
        let mut regs = prepare_reg_list(&[Reg::Rax]);
        assert!(run_segment_list(&[], &mut regs));
    }
}
