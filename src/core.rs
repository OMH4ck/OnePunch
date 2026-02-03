//! Core logic - instruction handlers, execution engine, preprocessing

use crate::asmutils::get_reg_str_by_reg;
use crate::types::*;
use std::cell::RefCell;
use std::collections::{HashMap, HashSet};
use std::rc::Rc;


// Global state for RSP/RBP usability
thread_local! {
    static G_IS_RSP_USABLE: RefCell<bool> = const { RefCell::new(true) };
    static G_IS_RBP_USABLE: RefCell<bool> = const { RefCell::new(true) };
    pub static MEM_LIST: RefCell<Vec<MemoryRef>> = const { RefCell::new(Vec::new()) };
    pub static RECORD_MEM: RefCell<bool> = const { RefCell::new(false) };
}

pub fn is_rsp_usable() -> bool {
    G_IS_RSP_USABLE.with(|v| *v.borrow())
}

pub fn is_rbp_usable() -> bool {
    G_IS_RBP_USABLE.with(|v| *v.borrow())
}

pub fn set_rsp_usable(flag: bool) {
    G_IS_RSP_USABLE.with(|v| *v.borrow_mut() = flag);
}

pub fn set_rbp_usable(flag: bool) {
    G_IS_RBP_USABLE.with(|v| *v.borrow_mut() = flag);
}

fn is_stack_frame_reg(r: Reg) -> bool {
    r == Reg::Rsp || r == Reg::Rbp
}

fn is_stack_frame_reg_usable(r: Reg) -> bool {
    match r {
        Reg::Rsp => is_rsp_usable(),
        Reg::Rbp => is_rbp_usable(),
        _ => false,
    }
}

fn set_stack_frame_reg(r: Reg, flag: bool) {
    match r {
        Reg::Rsp => set_rsp_usable(flag),
        Reg::Rbp => set_rbp_usable(flag),
        _ => {}
    }
}

/// Check if a register is in the input list
pub fn is_in_input(reg: Reg, reg_list: &[Register]) -> bool {
    reg_list.iter().any(|r| r.name == reg)
}

/// Get register by name from list
pub fn get_reg_by_idx(reg: Reg, reg_list: &[Register]) -> Option<usize> {
    reg_list.iter().position(|r| r.name == reg)
}

/// Get register reference by name
pub fn get_reg_ref<'a>(reg: Reg, reg_list: &'a [Register]) -> Option<&'a Register> {
    reg_list.iter().find(|r| r.name == reg)
}

/// Get mutable register reference by name
pub fn get_reg_mut<'a>(reg: Reg, reg_list: &'a mut [Register]) -> Option<&'a mut Register> {
    reg_list.iter_mut().find(|r| r.name == reg)
}

/// Remove register by name from list
pub fn remove_reg_by_idx(reg: Reg, reg_list: &mut Vec<Register>) {
    reg_list.retain(|r| r.name != reg);
}

/// Make an alias of a register
pub fn make_alias(alias_reg_name: Reg, reg: &Register, copy_mem: bool) -> Register {
    let mut new_reg = Register::new(false);
    new_reg.name = alias_reg_name;
    new_reg.alias(reg, copy_mem);
    new_reg
}

/// Check if register is an alias (memory shared with another)
pub fn is_alias(reg: Reg, reg_list: &[Register]) -> bool {
    if let Some(reg_ptr) = get_reg_ref(reg, reg_list) {
        return reg_ptr.mem.borrow().ref_count > 1;
    }
    false
}

/// Check if register is independent (not aliased, single range)
pub fn is_independent(reg: Reg, reg_list: &[Register]) -> bool {
    if is_alias(reg, reg_list) {
        return false;
    }
    if let Some(regptr) = get_reg_ref(reg, reg_list) {
        return regptr.mem.borrow().range.len() == 1;
    }
    false
}

/// Prepare register list from register names
pub fn prepare_reg_list(reg_names: &[Reg]) -> Vec<Register> {
    let record = RECORD_MEM.with(|v| *v.borrow());
    reg_names
        .iter()
        .map(|name| {
            let mut reg = Register::new(true);
            reg.name = *name;
            reg.input_src = get_reg_str_by_reg(*name).to_string();
            reg.input_action = false;
            {
                let mut mem = reg.mem.borrow_mut();
                mem.input_src = reg.input_src.clone();
                mem.input_offset = 0;
                mem.input_action = false;
            }
            if record {
                MEM_LIST.with(|list| list.borrow_mut().push(Rc::clone(&reg.mem)));
            }
            reg
        })
        .collect()
}

/// Copy a register list (deep copy)
pub fn copy_reg_list(reg_list: &[Register]) -> Vec<Register> {
    let mut result: Vec<Register> = Vec::new();
    for reg in reg_list {
        let mut new_reg = Register::from_register(reg);
        // Check if memory should be shared with already copied register
        for existing in &result {
            if existing.mem.borrow().mem_id == new_reg.mem.borrow().mem_id {
                new_reg.mem = Rc::clone(&existing.mem);
                break;
            }
        }
        result.push(new_reg);
    }
    result
}

/// Check if instruction accesses uncontrolled memory
pub fn contain_uncontrol_memory_access(inst: &Instruction, reg_list: &[Register]) -> bool {
    if inst.operand_num == 0 || inst.opcode == Opcode::Lea || inst.opcode == Opcode::Nop {
        return false;
    }

    let Some(ref op_dst) = inst.op_dst else {
        return false;
    };

    let operand = if !op_dst.is_dereference {
        if let Some(ref op_src) = inst.op_src {
            if !op_src.is_dereference {
                return false;
            }
            op_src
        } else {
            return false;
        }
    } else {
        op_dst
    };

    if operand.contain_segment_reg() {
        return false;
    }

    for (reg, coef) in &operand.reg_list {
        if *coef != 1 {
            return true;
        }
        if !is_in_input(*reg, reg_list) {
            if matches!(*reg, Reg::Rsp | Reg::Rip | Reg::Rbp) {
                continue;
            }
            return true;
        }
    }
    false
}

/// Remove useless instructions from segment start
pub fn remove_useless_instructions(segment: &mut Segment, reg_list: &[Register]) -> usize {
    let mut index = segment.useful_inst_index;
    let size = segment.inst_list.len();

    while index < size {
        let inst = &segment.inst_list[index];
        if inst.operation_length != OperationLength::QWord || inst.opcode == Opcode::Push {
            index += 1;
            continue;
        }

        if inst.operand_num == 2 {
            let op_src = inst.op_src.as_ref().unwrap();
            let op_dst = inst.op_dst.as_ref().unwrap();

            if op_src.reg_num == 0 {
                index += 1;
                continue;
            }
            if op_dst.is_dereference {
                if op_dst.reg_num != 1 {
                    index += 1;
                    continue;
                }
                if !is_in_input(op_dst.get_reg_op(), reg_list) {
                    index += 1;
                    continue;
                }
            }
            if op_src.reg_num == 1 && op_src.reg_list[0].1 == 1 && is_in_input(op_src.reg_list[0].0, reg_list) {
                break;
            }
        } else {
            let op_dst = inst.op_dst.as_ref().unwrap();
            if op_dst.reg_num == 0 {
                index += 1;
                continue;
            }
            if op_dst.reg_num == 1 && op_dst.reg_list[0].1 == 1 && is_in_input(op_dst.reg_list[0].0, reg_list) {
                break;
            }
        }
        index += 1;
    }
    segment.useful_inst_index = index;
    index
}

/// XCHG instruction handler
pub fn xchg_handler(inst: &Instruction, reg_list: &mut Vec<Register>, record_flag: bool) -> bool {
    let op_src = inst.op_src.as_ref().unwrap();
    let op_dst = inst.op_dst.as_ref().unwrap();

    if inst.operation_length != OperationLength::QWord {
        if !op_src.is_dereference && !op_dst.is_dereference {
            // xchg reg, reg
            let reg_src = op_src.get_reg_op().to_reg64().unwrap_or(Reg::None);
            let reg_dst = op_dst.get_reg_op().to_reg64().unwrap_or(Reg::None);
            remove_reg_by_idx(reg_src, reg_list);
            remove_reg_by_idx(reg_dst, reg_list);
            return true;
        }

        let (mem_op, reg_op) = if op_dst.is_dereference {
            (op_dst, op_src)
        } else {
            (op_src, op_dst)
        };

        if mem_op.reg_num != 1 {
            return false;
        }
        let src_range = mem_op.get_used_range();
        let src_idx = get_reg_by_idx(src_range.0, reg_list);
        if let Some(idx) = src_idx {
            if !reg_list[idx].contain_range(&src_range.1) {
                return false;
            }
            reg_list[idx].remove_range(&src_range.1);
        } else {
            return false;
        }

        let reg_dst = reg_op.get_reg_op();
        remove_reg_by_idx(reg_dst, reg_list);
        return true;
    }

    if !op_src.is_dereference && !op_dst.is_dereference {
        // xchg reg, reg
        let reg_src = op_src.get_reg_op();
        let reg_dst = op_dst.get_reg_op();

        if reg_src == reg_dst {
            return true;
        }

        // Swap names
        if let Some(src_idx) = get_reg_by_idx(reg_src, reg_list) {
            reg_list[src_idx].name = reg_dst;
        }
        if let Some(dst_idx) = get_reg_by_idx(reg_dst, reg_list) {
            reg_list[dst_idx].name = reg_src;
        }
        return true;
    }

    let (mem_op, reg_op) = if op_dst.is_dereference {
        (op_dst, op_src)
    } else {
        (op_src, op_dst)
    };

    if mem_op.reg_num != 1 {
        return false;
    }

    let src_range = mem_op.get_used_range();
    let src_idx = get_reg_by_idx(src_range.0, reg_list);
    if src_idx.is_none() {
        return false;
    }
    let src_idx = src_idx.unwrap();

    if !reg_list[src_idx].contain_range(&src_range.1) {
        return false;
    }
    reg_list[src_idx].remove_range(&src_range.1);

    let reg_dst = reg_op.get_reg_op();
    remove_reg_by_idx(reg_dst, reg_list);

    let mut new_reg = Register::new(true);
    new_reg.name = reg_dst;
    if record_flag {
        let src_relation = reg_list[src_idx].get_input_relation();
        new_reg.set_input_relation(&src_relation, src_range.1 .0, true);
    }
    reg_list.push(new_reg);
    true
}

/// MOV instruction handler
pub fn mov_handler(inst: &Instruction, reg_list: &mut Vec<Register>, record_flag: bool) -> bool {
    let op_dst = inst.op_dst.as_ref().unwrap();
    let op_src = inst.op_src.as_ref().unwrap();

    if inst.operation_length != OperationLength::QWord {
        if op_dst.is_dereference {
            if op_dst.reg_num != 1 {
                return false;
            }
            let reg_dst = op_dst.get_reg_op();
            if !is_in_input(reg_dst, reg_list) {
                return false;
            }
            let range = op_dst.get_used_range();
            if let Some(idx) = get_reg_by_idx(range.0, reg_list) {
                reg_list[idx].remove_range(&range.1);
            }
            return true;
        } else {
            if op_dst.contain_segment_reg() {
                return true;
            }
            let t_reg = op_dst.reg_list[0].0;
            let reg64 = t_reg.to_reg64().unwrap_or(Reg::None);
            remove_reg_by_idx(reg64, reg_list);
            return true;
        }
    }

    if !op_dst.is_dereference && !op_src.is_dereference {
        if op_dst.contain_segment_reg() {
            return true;
        }
        let reg_dst = op_dst.get_reg_op();
        if op_src.reg_num == 0 {
            // mov reg, imm
            if is_stack_frame_reg(reg_dst) {
                set_stack_frame_reg(reg_dst, false);
            }
            remove_reg_by_idx(reg_dst, reg_list);
            return true;
        }

        // mov reg, reg
        let reg_src = op_src.reg_list[0].0;
        if reg_src == reg_dst {
            return true;
        }
        remove_reg_by_idx(reg_dst, reg_list);
        if is_in_input(reg_src, reg_list) {
            let src_idx = get_reg_by_idx(reg_src, reg_list).unwrap();
            let new_reg = make_alias(reg_dst, &reg_list[src_idx], true);
            reg_list.push(new_reg);
        } else if is_stack_frame_reg(reg_dst) {
            set_stack_frame_reg(reg_dst, false);
        }
    } else if op_src.is_dereference {
        // mov reg, []
        let reg_dst = op_dst.reg_list[0].0;
        let reg_src = op_src.reg_list[0].0;

        if reg_src != reg_dst {
            remove_reg_by_idx(reg_dst, reg_list);
        }

        if op_src.contain_segment_reg() {
            return true;
        }
        if !is_in_input(reg_src, reg_list) {
            let mut tmp_res = false;
            if is_stack_frame_reg(reg_src) {
                tmp_res = is_stack_frame_reg_usable(reg_src);
            } else if reg_src == Reg::Rip {
                tmp_res = true;
            }
            if is_stack_frame_reg(reg_dst) {
                set_stack_frame_reg(reg_dst, false);
            }
            return tmp_res;
        }

        if op_src.reg_list.len() != 1 {
            return false;
        }

        let range = op_src.get_used_range();
        let reg_idx = get_reg_by_idx(reg_src, reg_list).unwrap();

        if reg_list[reg_idx].contain_range(&range.1) {
            reg_list[reg_idx].remove_range(&range.1);

            let record = RECORD_MEM.with(|v| *v.borrow());
            let mut new_reg = Register::new(true);
            new_reg.name = reg_dst;

            let src_relation = reg_list[reg_idx].get_input_relation();
            new_reg.set_input_relation(&src_relation, op_src.imm, true);

            if record {
                MEM_LIST.with(|list| list.borrow_mut().push(Rc::clone(&new_reg.mem)));
            }

            if reg_src == reg_dst {
                remove_reg_by_idx(reg_dst, reg_list);
            }

            if record_flag {
                reg_list[reg_idx].set_content(
                    op_src.imm,
                    Value::new(ValueType::MemValue, new_reg.mem.borrow().mem_id as i64),
                    OperationLength::QWord,
                );
            }
            reg_list.push(new_reg);
        } else {
            if reg_src == reg_dst {
                remove_reg_by_idx(reg_dst, reg_list);
            }
            return false;
        }
    } else {
        // mov [], reg
        if op_dst.contain_segment_reg() {
            return true;
        }
        if op_dst.reg_num != 1 {
            return false;
        }

        let range = op_dst.get_used_range();
        if !is_in_input(range.0, reg_list) {
            if range.0 == Reg::Rip {
                return true;
            }
            return is_stack_frame_reg(range.0) && is_stack_frame_reg_usable(range.0);
        }
        let reg_idx = get_reg_by_idx(range.0, reg_list).unwrap();
        if !reg_list[reg_idx].contain_range(&range.1) {
            return false;
        }
        reg_list[reg_idx].remove_range(&range.1);
    }

    true
}

/// LEA instruction handler
pub fn lea_handler(inst: &Instruction, reg_list: &mut Vec<Register>) -> bool {
    let op_dst = inst.op_dst.as_ref().unwrap();
    let op_src = inst.op_src.as_ref().unwrap();

    let mut reg_dst = op_dst.reg_list[0].0;
    reg_dst = reg_dst.to_reg64().unwrap_or(reg_dst);

    if inst.operation_length != OperationLength::QWord || op_src.reg_num != 1 {
        remove_reg_by_idx(reg_dst, reg_list);
        return true;
    }

    let range = op_src.get_used_range();
    if !is_in_input(range.0, reg_list) {
        remove_reg_by_idx(reg_dst, reg_list);
        return true;
    }

    let reg_idx = get_reg_by_idx(range.0, reg_list).unwrap();
    let mut new_reg = make_alias(reg_dst, &reg_list[reg_idx], true);
    new_reg.base_offset = reg_list[reg_idx].base_offset + range.1 .0;
    let src_relation = reg_list[reg_idx].get_input_relation();
    new_reg.set_input_relation(&src_relation, range.1 .0, false);
    remove_reg_by_idx(reg_dst, reg_list);
    reg_list.push(new_reg);
    true
}

/// POP instruction handler
pub fn pop_handler(inst: &Instruction, reg_list: &mut Vec<Register>, record_flag: bool) -> bool {
    let op_dst = inst.op_dst.as_ref().unwrap();
    let reg_dst = op_dst.reg_list[0].0;

    if inst.operation_length != OperationLength::QWord {
        let reg64 = reg_dst.to_reg64().unwrap_or(reg_dst);
        remove_reg_by_idx(reg64, reg_list);
        return true;
    }

    if Reg::Rsp != reg_dst {
        remove_reg_by_idx(reg_dst, reg_list);
    }

    if is_in_input(Reg::Rsp, reg_list) {
        let rsp_idx = get_reg_by_idx(Reg::Rsp, reg_list).unwrap();
        if reg_list[rsp_idx].contain_range(&(0, 8)) {
            let record = RECORD_MEM.with(|v| *v.borrow());
            let mut new_reg = Register::new(true);
            new_reg.name = reg_dst;

            let rsp_relation = reg_list[rsp_idx].get_input_relation();
            new_reg.set_input_relation(&rsp_relation, 0, true);

            if record {
                MEM_LIST.with(|list| list.borrow_mut().push(Rc::clone(&new_reg.mem)));
            }

            if record_flag {
                reg_list[rsp_idx].set_content(
                    0,
                    Value::new(ValueType::MemValue, new_reg.mem.borrow().mem_id as i64),
                    OperationLength::QWord,
                );
            }

            reg_list[rsp_idx].remove_range(&(0, 8));
            reg_list[rsp_idx].base_offset += 8;
            let old_relation = reg_list[rsp_idx].get_input_relation();
            reg_list[rsp_idx].set_input_relation(&old_relation, 8, false);

            if reg_dst == Reg::Rsp {
                remove_reg_by_idx(Reg::Rsp, reg_list);
            }
            reg_list.push(new_reg);
        }
    } else {
        remove_reg_by_idx(reg_dst, reg_list);
        if Reg::Rsp == reg_dst {
            return false;
        }
    }
    true
}

/// ADD/SUB instruction handler
pub fn add_sub_handler(inst: &Instruction, reg_list: &mut Vec<Register>) -> bool {
    let op_dst = inst.op_dst.as_ref().unwrap();
    let op_src = inst.op_src.as_ref().unwrap();

    if !op_dst.is_dereference {
        let mut reg_dst = op_dst.reg_list[0].0;
        let mut is_bh = false;
        if inst.operation_length != OperationLength::QWord {
            if reg_dst.is_reg8h() {
                is_bh = true;
            }
            reg_dst = reg_dst.to_reg64().unwrap_or(reg_dst);
        }

        let Some(reg_idx) = get_reg_by_idx(reg_dst, reg_list) else {
            return true;
        };

        if op_src.reg_num == 0 {
            // add/sub reg, imm
            let mut imm = op_src.imm;
            if inst.opcode == Opcode::Sub {
                imm = -imm;
            }

            reg_list[reg_idx].base_offset += if is_bh { imm * 0x100 } else { imm };
            let old_relation = reg_list[reg_idx].get_input_relation();
            reg_list[reg_idx].set_input_relation(&old_relation, imm, false);
        } else if !op_src.is_dereference {
            // add/sub reg, reg
            remove_reg_by_idx(reg_dst, reg_list);
        } else {
            // add/sub reg, []
            if op_src.reg_num != 1 {
                return false;
            }
            if op_src.contain_segment_reg() {
                remove_reg_by_idx(reg_dst, reg_list);
                return true;
            }
            let range = op_src.get_used_range();
            if let Some(src_idx) = get_reg_by_idx(range.0, reg_list) {
                if !reg_list[src_idx].contain_range(&range.1) {
                    remove_reg_by_idx(reg_dst, reg_list);
                } else {
                    reg_list[src_idx].remove_range(&range.1);
                    reg_list[src_idx].set_content(
                        range.1 .0,
                        Value::new(ValueType::ImmValue, 0),
                        inst.operation_length,
                    );
                }
            } else {
                return false;
            }
        }
    } else {
        if op_dst.reg_num != 1 {
            return false;
        }
        let reg_dst = op_dst.get_reg_op();
        let Some(reg_idx) = get_reg_by_idx(reg_dst, reg_list) else {
            return false;
        };
        let range = op_dst.get_used_range();
        reg_list[reg_idx].remove_range(&range.1);
    }
    true
}

/// PUSH instruction handler
pub fn push_handler(inst: &Instruction, reg_list: &mut Vec<Register>) -> bool {
    let Some(rsp_idx) = get_reg_by_idx(Reg::Rsp, reg_list) else {
        return true;
    };

    let range = inst.op_dst.as_ref().unwrap().get_used_range();
    if !reg_list[rsp_idx].contain_range(&range.1) {
        return false;
    }
    reg_list[rsp_idx].base_offset -= 8;
    let old_relation = reg_list[rsp_idx].get_input_relation();
    reg_list[rsp_idx].set_input_relation(&old_relation, -8, false);
    true
}

/// Bitwise instruction handler (XOR, AND, OR, SHR, ROR, SAR, SHL)
pub fn bitwise_handler(inst: &Instruction, reg_list: &mut Vec<Register>) -> bool {
    let op_dst = inst.op_dst.as_ref().unwrap();

    if op_dst.is_dereference {
        if op_dst.reg_num != 1 {
            return false;
        }
        let range = op_dst.get_used_range();
        let Some(reg_idx) = get_reg_by_idx(range.0, reg_list) else {
            return false;
        };
        reg_list[reg_idx].remove_range(&range.1);
    } else {
        let mut reg_dst = op_dst.reg_list[0].0;
        if inst.operation_length != OperationLength::QWord {
            reg_dst = reg_dst.to_reg64().unwrap_or(reg_dst);
        }
        remove_reg_by_idx(reg_dst, reg_list);
    }
    true
}

/// CALL/JMP instruction handler
pub fn branch_handler(inst: &Instruction, reg_list: &mut Vec<Register>, record_flag: bool) -> bool {
    let op_dst = inst.op_dst.as_ref().unwrap();

    if op_dst.is_dereference {
        // call/jmp []
        if op_dst.reg_num != 1 {
            for (reg, _) in &op_dst.reg_list {
                if !is_independent(*reg, reg_list) {
                    return false;
                }
            }
            let second_reg = op_dst.reg_list[1].0;
            if record_flag {
                if let Some(idx) = get_reg_by_idx(second_reg, reg_list) {
                    reg_list[idx].set_content(-1, Value::new(ValueType::ImmValue, 0xdeadbeef), OperationLength::QWord);
                }
            }
            remove_reg_by_idx(second_reg, reg_list);
        } else {
            if op_dst.reg_list[0].1 != 1 {
                return false;
            }

            let range = op_dst.get_used_range();
            let Some(dst_idx) = get_reg_by_idx(range.0, reg_list) else {
                return false;
            };

            if range.1 .1 - range.1 .0 != 8 {
                return false;
            }
            if !reg_list[dst_idx].contain_range(&range.1) {
                return false;
            }

            reg_list[dst_idx].remove_range(&range.1);

            if record_flag {
                reg_list[dst_idx].set_content(
                    range.1 .0,
                    Value::new(ValueType::CallValue, inst.offset as i64),
                    OperationLength::QWord,
                );
            }
        }
    } else {
        // call/jmp reg
        let reg = op_dst.reg_list[0].0;
        if !is_independent(reg, reg_list) {
            return false;
        }

        if record_flag {
            if let Some(idx) = get_reg_by_idx(reg, reg_list) {
                let mem_id = reg_list[idx].mem.borrow().mem_id;
                reg_list[idx].set_content(
                    -1,
                    Value::new(
                        ValueType::CallRegValue,
                        ((mem_id as i64) << 32) + inst.offset as i64,
                    ),
                    OperationLength::QWord,
                );
            }
        }
        remove_reg_by_idx(reg, reg_list);
    }

    // Handle stack pointer adjustment for call
    if let Some(rsp_idx) = get_reg_by_idx(Reg::Rsp, reg_list) {
        if inst.opcode == Opcode::Call {
            let tr = (-8, 0);
            reg_list[rsp_idx].remove_range(&tr);
            reg_list[rsp_idx].base_offset -= 8;
            let old_relation = reg_list[rsp_idx].get_input_relation();
            reg_list[rsp_idx].set_input_relation(&old_relation, -8, false);
        }
    }
    true
}

/// Execute a single instruction
pub fn execute_one_instruction(
    inst: &Instruction,
    reg_list: &mut Vec<Register>,
    record_flag: bool,
) -> bool {
    if contain_uncontrol_memory_access(inst, reg_list) {
        return false;
    }

    match inst.opcode {
        Opcode::Mov | Opcode::Movsxd => mov_handler(inst, reg_list, record_flag),
        Opcode::Lea => lea_handler(inst, reg_list),
        Opcode::Pop => pop_handler(inst, reg_list, record_flag),
        Opcode::Push => push_handler(inst, reg_list),
        Opcode::Add | Opcode::Sub => add_sub_handler(inst, reg_list),
        Opcode::Xor | Opcode::And | Opcode::Or | Opcode::Shr | Opcode::Ror | Opcode::Sar | Opcode::Shl => {
            bitwise_handler(inst, reg_list)
        }
        Opcode::Test | Opcode::Cmp | Opcode::Nop => true,
        Opcode::Call | Opcode::Jmp => branch_handler(inst, reg_list, record_flag),
        Opcode::Xchg => xchg_handler(inst, reg_list, record_flag),
        _ => false,
    }
}

/// Execute all instructions in a segment
pub fn execute_instructions(
    segment: &Segment,
    reg_list: &mut Vec<Register>,
    record_flag: bool,
) -> bool {
    for idx in segment.useful_inst_index..segment.inst_list.len() {
        if !execute_one_instruction(&segment.inst_list[idx], reg_list, record_flag) {
            if record_flag {
                println!("Inst: {}", segment.inst_list[idx].original_inst);
            }
            return false;
        }
        if reg_list.is_empty() {
            return false;
        }
    }
    true
}

/// Preprocessor for segment analysis
pub struct Preprocessor {
    pub result: HashMap<u64, Vec<SegmentPtr>>,
    pub test: HashMap<usize, u64>,
}

impl Preprocessor {
    pub fn new() -> Self {
        Self {
            result: HashMap::new(),
            test: HashMap::new(),
        }
    }

    pub fn process(&mut self, segments: &[SegmentPtr]) {
        for (i, seg) in segments.iter().enumerate() {
            let res = compute_constraint(seg);
            self.test.insert(i, res);
        }
    }

    pub fn get_constraint(&self, idx: usize) -> u64 {
        *self.test.get(&idx).unwrap_or(&0)
    }
}

impl Default for Preprocessor {
    fn default() -> Self {
        Self::new()
    }
}

/// Compute constraint hash for a segment
pub fn compute_constraint(segment: &SegmentPtr) -> u64 {
    let mut input_regs: HashSet<Reg> = HashSet::new();
    let mut output_regs: HashSet<Reg> = HashSet::new();
    collect_input_output_regs(segment, &mut input_regs, &mut output_regs);
    let input_hash = hash_reg_set(&input_regs);
    let output_hash = hash_reg_set(&output_regs);
    (input_hash << 32) | output_hash
}

/// Collect input and output registers from a segment
fn collect_input_output_regs(
    segment: &SegmentPtr,
    input_regs: &mut HashSet<Reg>,
    output_regs: &mut HashSet<Reg>,
) {
    fn opcode_dst_control(opcode: Opcode) -> bool {
        matches!(opcode, Opcode::Mov | Opcode::Lea | Opcode::Pop)
    }

    for idx in segment.useful_inst_index..segment.inst_list.len() {
        let inst = &segment.inst_list[idx];
        let op_src = &inst.op_src;
        let op_dst = &inst.op_dst;

        if let Some(src) = op_src {
            if src.is_dereference && src.reg_num == 1 {
                let reg = src.get_reg_op();
                if !input_regs.contains(&reg) && !output_regs.contains(&reg) {
                    input_regs.insert(reg);
                }
            }
        }

        if let Some(dst) = op_dst {
            if dst.is_dereference && dst.reg_num == 1 {
                let reg = dst.get_reg_op();
                if !input_regs.contains(&reg) && !output_regs.contains(&reg) {
                    input_regs.insert(reg);
                }
            } else if opcode_dst_control(inst.opcode)
                && dst.reg_num == 1
                && inst.operation_length == OperationLength::QWord
            {
                output_regs.insert(dst.get_reg_op());
            }
        }
    }
}

/// Hash a set of registers
fn hash_reg_set(reg_set: &HashSet<Reg>) -> u64 {
    let mut res: u64 = 0;
    for reg in reg_set {
        res |= 1u64 << (*reg as u64);
    }
    res
}

/// Hash a register list
pub fn hash_reg_list(reg_list: &[Register]) -> u64 {
    let mut res: u64 = 0;
    for reg in reg_list {
        res |= 1u64 << (reg.name as u64);
    }
    res
}

/// Check if hash matches for pruning
pub fn hash_match(needed: u64, src: u64) -> bool {
    let needed_input = (needed >> 32) as u32;
    let needed_output = (needed & 0xFFFFFFFF) as u32;
    // Check if segment's output registers overlap with current register list
    if ((src as u32) & needed_output) ^ needed_output == 0 {
        return false;
    }
    // Check if segment's required input registers are in current register list
    // Note: src is the hash of current registers (lower 32 bits only, not shifted)
    if needed_input != 0 && (needed_input & src as u32) == 0 {
        return false;
    }
    true
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::sync::Arc;

    #[test]
    fn test_is_in_input() {
        let regs = prepare_reg_list(&[Reg::Rax, Reg::Rbx]);
        assert!(is_in_input(Reg::Rax, &regs));
        assert!(is_in_input(Reg::Rbx, &regs));
        assert!(!is_in_input(Reg::Rcx, &regs));
    }

    #[test]
    fn test_get_reg_by_idx() {
        let regs = prepare_reg_list(&[Reg::Rax, Reg::Rbx]);
        assert_eq!(get_reg_by_idx(Reg::Rax, &regs), Some(0));
        assert_eq!(get_reg_by_idx(Reg::Rbx, &regs), Some(1));
        assert_eq!(get_reg_by_idx(Reg::Rcx, &regs), None);
    }

    #[test]
    fn test_remove_reg_by_idx() {
        let mut regs = prepare_reg_list(&[Reg::Rax, Reg::Rbx, Reg::Rcx]);
        remove_reg_by_idx(Reg::Rbx, &mut regs);
        assert_eq!(regs.len(), 2);
        assert!(!is_in_input(Reg::Rbx, &regs));
    }

    #[test]
    fn test_make_alias() {
        let regs = prepare_reg_list(&[Reg::Rax]);
        let alias = make_alias(Reg::Rbx, &regs[0], true);
        assert_eq!(alias.name, Reg::Rbx);
        assert_eq!(alias.mem.borrow().mem_id, regs[0].mem.borrow().mem_id);
    }

    #[test]
    fn test_copy_reg_list() {
        let regs = prepare_reg_list(&[Reg::Rax, Reg::Rbx]);
        let copied = copy_reg_list(&regs);
        assert_eq!(copied.len(), 2);
        assert_eq!(copied[0].name, Reg::Rax);
        assert_eq!(copied[1].name, Reg::Rbx);
    }

    #[test]
    fn test_hash_reg_list() {
        let regs = prepare_reg_list(&[Reg::Rax]);
        let hash = hash_reg_list(&regs);
        assert!(hash > 0);
    }

    #[test]
    fn test_preprocessor() {
        let mut prep = Preprocessor::new();
        let inst = Arc::new(Instruction::new(0x100, Opcode::Call));
        let seg = Arc::new(Segment::new(vec![inst]));
        prep.process(&[seg]);
        // Just verify it doesn't panic
    }

    #[test]
    fn test_stack_frame_usability() {
        set_rsp_usable(true);
        assert!(is_rsp_usable());
        set_rsp_usable(false);
        assert!(!is_rsp_usable());
        set_rsp_usable(true);
    }

    #[test]
    fn test_hash_match() {
        // When both needed_input and needed_output are 0, should fail
        assert!(!hash_match(0, 0));
        // needed = (input_hash << 32) | output_hash
        // src = hash of current registers (lower 32 bits)
        // This test: needed_input=0x1, needed_output=0x1, src=0x3 (has bits 0 and 1)
        // Check: output overlap (src & needed_output = 3 & 1 = 1, XOR 1 = 0) -> false
        assert!(!hash_match(0x100000001, 0x3));
        // This test: needed_input=0x1, needed_output=0x2, src=0x3 (has bits 0 and 1)
        // Check 1: (3 & 2) ^ 2 = 2 ^ 2 = 0 -> false, skip to next
        // But we want a case that passes:
        // needed_input=0x1, needed_output=0x2, src=0x1 (only bit 0)
        // Check 1: (1 & 2) ^ 2 = 0 ^ 2 = 2 != 0 -> continue
        // Check 2: (1 & 1) = 1 != 0 -> passes
        assert!(hash_match(0x100000002, 0x1));
    }

    // ===== Instruction Handler Tests =====

    fn create_mov_instruction(
        dst_reg: Reg,
        src_reg: Reg,
        src_is_deref: bool,
        src_imm: i64,
    ) -> Instruction {
        let mut inst = Instruction::new(0x100, Opcode::Mov);
        inst.op_dst = Some(Operand::new(
            false,
            false,
            vec![(dst_reg, 1)],
            false,
            0,
            OperationLength::QWord,
        ));
        let mut op_src = Operand::new(
            src_is_deref,
            false,
            if src_reg != Reg::None { vec![(src_reg, 1)] } else { vec![] },
            false,
            0,
            OperationLength::QWord,
        );
        op_src.imm = src_imm;
        inst.op_src = Some(op_src);
        inst.operation_length = OperationLength::QWord;
        inst.operand_num = 2;
        inst
    }

    #[test]
    fn test_mov_handler_reg_to_reg() {
        // mov rbx, rax  (creates alias)
        let mut regs = prepare_reg_list(&[Reg::Rax]);
        let inst = create_mov_instruction(Reg::Rbx, Reg::Rax, false, 0);
        
        let result = mov_handler(&inst, &mut regs, false);
        assert!(result);
        assert_eq!(regs.len(), 2);
        assert!(is_in_input(Reg::Rbx, &regs));
    }

    #[test]
    fn test_mov_handler_mem_to_reg() {
        // mov rbx, [rax+0x10]
        let mut regs = prepare_reg_list(&[Reg::Rax]);
        let inst = create_mov_instruction(Reg::Rbx, Reg::Rax, true, 0x10);
        
        let result = mov_handler(&inst, &mut regs, false);
        assert!(result);
        assert!(is_in_input(Reg::Rbx, &regs));
    }

    #[test]
    fn test_mov_handler_removes_dst_if_not_in_input() {
        // mov rax, 0x42 (immediate) - should remove rax if not controllable
        let mut regs = prepare_reg_list(&[Reg::Rax]);
        let mut inst = Instruction::new(0x100, Opcode::Mov);
        inst.op_dst = Some(Operand::new(
            false,
            false,
            vec![(Reg::Rax, 1)],
            false,
            0,
            OperationLength::QWord,
        ));
        // Literal source (immediate value)
        let mut src = Operand::new(false, false, vec![], false, 0x42, OperationLength::QWord);
        src.literal_num = 0x42;
        inst.op_src = Some(src);
        inst.operand_num = 2;
        
        let result = mov_handler(&inst, &mut regs, false);
        assert!(result);
        // Rax should be removed since it's overwritten with non-controlled value
        assert!(!is_in_input(Reg::Rax, &regs));
    }

    #[test]
    fn test_lea_handler() {
        // lea rbx, [rax+0x20]
        let mut regs = prepare_reg_list(&[Reg::Rax]);
        let mut inst = Instruction::new(0x100, Opcode::Lea);
        inst.op_dst = Some(Operand::new(
            false,
            false,
            vec![(Reg::Rbx, 1)],
            false,
            0,
            OperationLength::QWord,
        ));
        inst.op_src = Some(Operand::new(
            true,
            false,
            vec![(Reg::Rax, 1)],
            false,
            0x20,
            OperationLength::QWord,
        ));
        inst.operand_num = 2;
        
        let result = lea_handler(&inst, &mut regs);
        assert!(result);
        assert!(is_in_input(Reg::Rbx, &regs));
    }

    #[test]
    fn test_pop_handler() {
        // pop rbx - should create new controlled register if rsp is usable
        let mut regs = prepare_reg_list(&[Reg::Rsp]);
        set_rsp_usable(true);
        
        let mut inst = Instruction::new(0x100, Opcode::Pop);
        inst.op_dst = Some(Operand::new(
            false,
            false,
            vec![(Reg::Rbx, 1)],
            false,
            0,
            OperationLength::QWord,
        ));
        inst.operand_num = 1;
        
        let result = pop_handler(&inst, &mut regs, false);
        assert!(result);
        assert!(is_in_input(Reg::Rbx, &regs));
    }

    #[test]
    fn test_push_handler() {
        // push rax - adjusts rsp offset
        let mut regs = prepare_reg_list(&[Reg::Rsp]);
        set_rsp_usable(true);
        
        let mut inst = Instruction::new(0x100, Opcode::Push);
        inst.op_dst = Some(Operand::new(
            false,
            false,
            vec![(Reg::Rax, 1)],
            false,
            0,
            OperationLength::QWord,
        ));
        inst.operand_num = 1;
        
        let result = push_handler(&inst, &mut regs);
        assert!(result);
    }

    #[test]
    fn test_add_handler() {
        // add rax, 0x10
        let mut regs = prepare_reg_list(&[Reg::Rax]);
        let _initial_offset = regs[0].base_offset;
        
        let mut inst = Instruction::new(0x100, Opcode::Add);
        inst.op_dst = Some(Operand::new(
            false,
            false,
            vec![(Reg::Rax, 1)],
            false,
            0,
            OperationLength::QWord,
        ));
        let mut src = Operand::new(false, false, vec![], false, 0x10, OperationLength::QWord);
        src.literal_num = 0x10;
        inst.op_src = Some(src);
        inst.operand_num = 2;
        
        let result = add_sub_handler(&inst, &mut regs);
        assert!(result);
        // Rax should still be in input list
        assert!(is_in_input(Reg::Rax, &regs));
    }

    #[test]
    fn test_sub_handler() {
        // sub rax, 0x10
        let mut regs = prepare_reg_list(&[Reg::Rax]);
        let _initial_offset = regs[0].base_offset;
        
        let mut inst = Instruction::new(0x100, Opcode::Sub);
        inst.op_dst = Some(Operand::new(
            false,
            false,
            vec![(Reg::Rax, 1)],
            false,
            0,
            OperationLength::QWord,
        ));
        let mut src = Operand::new(false, false, vec![], false, 0x10, OperationLength::QWord);
        src.literal_num = 0x10;
        inst.op_src = Some(src);
        inst.operand_num = 2;
        
        let result = add_sub_handler(&inst, &mut regs);
        assert!(result);
        // Rax should still be in input list
        assert!(is_in_input(Reg::Rax, &regs));
    }

    #[test]
    fn test_xor_handler_clears_reg() {
        // xor rax, rax - clears register
        let mut regs = prepare_reg_list(&[Reg::Rax]);
        
        let mut inst = Instruction::new(0x100, Opcode::Xor);
        inst.op_dst = Some(Operand::new(
            false,
            false,
            vec![(Reg::Rax, 1)],
            false,
            0,
            OperationLength::QWord,
        ));
        inst.op_src = Some(Operand::new(
            false,
            false,
            vec![(Reg::Rax, 1)],
            false,
            0,
            OperationLength::QWord,
        ));
        inst.operand_num = 2;
        
        let result = bitwise_handler(&inst, &mut regs);
        assert!(result);
        // xor rax, rax removes rax from controlled list
        assert!(!is_in_input(Reg::Rax, &regs));
    }

    #[test]
    fn test_branch_handler_call() {
        // call [rax+0x10]
        let mut regs = prepare_reg_list(&[Reg::Rax]);
        
        let mut inst = Instruction::new(0x100, Opcode::Call);
        let mut op = Operand::new(
            true, // dereference
            false,
            vec![(Reg::Rax, 1)],
            false,
            0,
            OperationLength::QWord,
        );
        op.imm = 0x10;
        inst.op_dst = Some(op);
        inst.operand_num = 1;
        
        let result = branch_handler(&inst, &mut regs, false);
        assert!(result);
        // Memory range should be removed and content set
    }

    #[test]
    fn test_branch_handler_jmp() {
        // jmp [rax+0x8]
        let mut regs = prepare_reg_list(&[Reg::Rax]);
        
        let mut inst = Instruction::new(0x100, Opcode::Jmp);
        let mut op = Operand::new(
            true, // dereference
            false,
            vec![(Reg::Rax, 1)],
            false,
            0,
            OperationLength::QWord,
        );
        op.imm = 0x8;
        inst.op_dst = Some(op);
        inst.operand_num = 1;
        
        let result = branch_handler(&inst, &mut regs, false);
        assert!(result);
    }

    #[test]
    fn test_xchg_handler() {
        // xchg rax, rbx
        let mut regs = prepare_reg_list(&[Reg::Rax, Reg::Rbx]);
        let rax_mem_id = regs[0].mem.borrow().mem_id;
        let rbx_mem_id = regs[1].mem.borrow().mem_id;
        
        let mut inst = Instruction::new(0x100, Opcode::Xchg);
        inst.op_dst = Some(Operand::new(
            false,
            false,
            vec![(Reg::Rax, 1)],
            false,
            0,
            OperationLength::QWord,
        ));
        inst.op_src = Some(Operand::new(
            false,
            false,
            vec![(Reg::Rbx, 1)],
            false,
            0,
            OperationLength::QWord,
        ));
        inst.operand_num = 2;
        
        let result = xchg_handler(&inst, &mut regs, false);
        assert!(result);
        
        // Memory should be swapped
        let rax = get_reg_ref(Reg::Rax, &regs).unwrap();
        let rbx = get_reg_ref(Reg::Rbx, &regs).unwrap();
        assert_eq!(rax.mem.borrow().mem_id, rbx_mem_id);
        assert_eq!(rbx.mem.borrow().mem_id, rax_mem_id);
    }

    #[test]
    fn test_execute_one_instruction() {
        let mut regs = prepare_reg_list(&[Reg::Rax]);
        let inst = create_mov_instruction(Reg::Rbx, Reg::Rax, false, 0);
        
        let result = execute_one_instruction(&inst, &mut regs, false);
        assert!(result);
        assert!(is_in_input(Reg::Rbx, &regs));
    }

    #[test]
    fn test_execute_instructions() {
        let mut regs = prepare_reg_list(&[Reg::Rax]);
        let inst1 = Arc::new(create_mov_instruction(Reg::Rbx, Reg::Rax, false, 0));
        
        let mut inst2 = Instruction::new(0x200, Opcode::Jmp);
        let mut op = Operand::new(true, false, vec![(Reg::Rbx, 1)], false, 0, OperationLength::QWord);
        op.imm = 0x10;
        inst2.op_dst = Some(op);
        inst2.operand_num = 1;
        let inst2 = Arc::new(inst2);
        
        let mut seg = Segment::new(vec![inst1, inst2]);
        seg.useful_inst_index = 0;
        
        let result = execute_instructions(&seg, &mut regs, false);
        assert!(result);
    }

    #[test]
    fn test_is_alias() {
        let mut regs = prepare_reg_list(&[Reg::Rax]);
        let alias = make_alias(Reg::Rbx, &regs[0], true);
        regs.push(alias);
        
        // After aliasing, both should share memory
        assert!(is_alias(Reg::Rax, &regs));
        assert!(is_alias(Reg::Rbx, &regs));
    }

    #[test]
    fn test_is_independent() {
        let regs = prepare_reg_list(&[Reg::Rax]);
        assert!(is_independent(Reg::Rax, &regs));
        
        // After aliasing, neither is independent
        let mut regs2 = prepare_reg_list(&[Reg::Rax]);
        let alias = make_alias(Reg::Rbx, &regs2[0], true);
        regs2.push(alias);
        assert!(!is_independent(Reg::Rax, &regs2));
        assert!(!is_independent(Reg::Rbx, &regs2));
    }

    #[test]
    fn test_contain_uncontrol_memory_access() {
        let regs = prepare_reg_list(&[Reg::Rax]);
        
        // Access to controlled register - OK
        let inst1 = create_mov_instruction(Reg::Rbx, Reg::Rax, true, 0x10);
        assert!(!contain_uncontrol_memory_access(&inst1, &regs));
        
        // Access to uncontrolled register - not OK
        let inst2 = create_mov_instruction(Reg::Rbx, Reg::Rcx, true, 0x10);
        assert!(contain_uncontrol_memory_access(&inst2, &regs));
    }

    #[test]
    fn test_is_stack_frame_reg() {
        assert!(is_stack_frame_reg(Reg::Rsp));
        assert!(is_stack_frame_reg(Reg::Rbp));
        assert!(!is_stack_frame_reg(Reg::Rax));
    }

    #[test]
    fn test_set_stack_frame_reg() {
        set_stack_frame_reg(Reg::Rsp, true);
        assert!(is_stack_frame_reg_usable(Reg::Rsp));
        set_stack_frame_reg(Reg::Rsp, false);
        assert!(!is_stack_frame_reg_usable(Reg::Rsp));
        set_stack_frame_reg(Reg::Rsp, true);
        
        set_stack_frame_reg(Reg::Rbp, true);
        assert!(is_stack_frame_reg_usable(Reg::Rbp));
        set_stack_frame_reg(Reg::Rbp, false);
        assert!(!is_stack_frame_reg_usable(Reg::Rbp));
        set_stack_frame_reg(Reg::Rbp, true);
        
        // Non-stack frame reg
        set_stack_frame_reg(Reg::Rax, true);
        assert!(!is_stack_frame_reg_usable(Reg::Rax));
    }

    #[test]
    fn test_get_reg_ref_and_mut() {
        let mut regs = prepare_reg_list(&[Reg::Rax, Reg::Rbx]);
        
        // Test get_reg_ref
        let rax_ref = get_reg_ref(Reg::Rax, &regs);
        assert!(rax_ref.is_some());
        assert_eq!(rax_ref.unwrap().name, Reg::Rax);
        
        let none_ref = get_reg_ref(Reg::Rcx, &regs);
        assert!(none_ref.is_none());
        
        // Test get_reg_mut
        let rbx_mut = get_reg_mut(Reg::Rbx, &mut regs);
        assert!(rbx_mut.is_some());
        rbx_mut.unwrap().base_offset = 0x100;
        assert_eq!(regs[1].base_offset, 0x100);
    }
}

