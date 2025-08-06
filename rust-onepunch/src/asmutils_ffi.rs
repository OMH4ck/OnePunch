use std::ffi::{CStr};
use std::os::raw::{c_char, c_uchar, c_uint, c_ulong};

use crate::asmutils::*;

#[no_mangle]
pub extern "C" fn rust_transfer_str_to_op(op_str: *const c_char) -> RustOpcode {
    if op_str.is_null() {
        return RustOpcode::OpNone;
    }
    
    unsafe {
        let c_str = CStr::from_ptr(op_str);
        if let Ok(str_slice) = c_str.to_str() {
            transfer_str_to_op(str_slice)
        } else {
            RustOpcode::OpNone
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_transfer_op_to_str(opcode: RustOpcode) -> *const c_char {
    let byte_slice = transfer_op_to_str(opcode);
    byte_slice.as_ptr() as *const c_char
}

#[no_mangle]
pub extern "C" fn rust_get_reg_by_str(reg_str: *const c_char) -> RustReg {
    if reg_str.is_null() {
        return RustReg::RegNone;
    }
    
    unsafe {
        let c_str = CStr::from_ptr(reg_str);
        if let Ok(str_slice) = c_str.to_str() {
            get_reg_by_str(str_slice)
        } else {
            RustReg::RegNone
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_get_reg_str_by_reg(reg: RustReg) -> *const c_char {
    let byte_slice = get_reg_str_by_reg(reg);
    byte_slice.as_ptr() as *const c_char
}

// Complex structure FFI functions

#[no_mangle]
pub extern "C" fn rust_operand_new(
    is_dereference: c_uchar,
    is_contain_seg_reg: c_uchar,
    reg_list_ptr: *const (RustReg, c_uint),
    reg_list_len: c_uint,
    literal_sym: c_uchar,
    literal_num: c_ulong,
    operation_length: RustOperationLength,
) -> *mut RustOperand {
    if reg_list_ptr.is_null() && reg_list_len > 0 {
        return std::ptr::null_mut();
    }

    let reg_list = if reg_list_len == 0 {
        Vec::new()
    } else {
        unsafe {
            std::slice::from_raw_parts(reg_list_ptr, reg_list_len as usize).to_vec()
        }
    };

    let operand = RustOperand::new(
        is_dereference != 0,
        is_contain_seg_reg != 0,
        reg_list,
        literal_sym != 0,
        literal_num,
        operation_length,
    );

    Box::into_raw(Box::new(operand))
}

#[no_mangle]
pub extern "C" fn rust_operand_free(operand: *mut RustOperand) {
    if !operand.is_null() {
        unsafe {
            drop(Box::from_raw(operand));
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_operand_is_literal_number(operand: *const RustOperand) -> c_uchar {
    if operand.is_null() {
        return 0;
    }
    unsafe {
        if (*operand).is_literal_number() { 1 } else { 0 }
    }
}

#[no_mangle]
pub extern "C" fn rust_operand_is_reg_operation(operand: *const RustOperand) -> c_uchar {
    if operand.is_null() {
        return 0;
    }
    unsafe {
        if (*operand).is_reg_operation() { 1 } else { 0 }
    }
}

#[no_mangle]
pub extern "C" fn rust_operand_contain_reg(operand: *const RustOperand, reg: RustReg) -> c_uchar {
    if operand.is_null() {
        return 0;
    }
    unsafe {
        if (*operand).contain_reg(reg) { 1 } else { 0 }
    }
}

#[no_mangle]
pub extern "C" fn rust_operand_is_reg64_operation(operand: *const RustOperand) -> c_uchar {
    if operand.is_null() {
        return 0;
    }
    unsafe {
        if (*operand).is_reg64_operation() { 1 } else { 0 }
    }
}

#[no_mangle]
pub extern "C" fn rust_operand_get_reg_op(operand: *const RustOperand) -> RustReg {
    if operand.is_null() {
        return RustReg::RegNone;
    }
    unsafe {
        (*operand).get_reg_op()
    }
}

#[no_mangle]
pub extern "C" fn rust_instruction_new(offset: c_ulong, opcode: RustOpcode) -> *mut RustInstruction {
    let instruction = RustInstruction::new(offset, opcode);
    Box::into_raw(Box::new(instruction))
}

#[no_mangle]
pub extern "C" fn rust_instruction_free(instruction: *mut RustInstruction) {
    if !instruction.is_null() {
        unsafe {
            drop(Box::from_raw(instruction));
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_instruction_is_reg_operation(instruction: *const RustInstruction) -> c_uchar {
    if instruction.is_null() {
        return 0;
    }
    unsafe {
        if (*instruction).is_reg_operation() { 1 } else { 0 }
    }
}

#[no_mangle]
pub extern "C" fn rust_instruction_set_operands(
    instruction: *mut RustInstruction,
    src_operand: *mut RustOperand,
    dst_operand: *mut RustOperand,
) {
    if instruction.is_null() {
        return;
    }
    
    unsafe {
        let src = if src_operand.is_null() {
            None
        } else {
            Some(*Box::from_raw(src_operand))
        };
        
        let dst = if dst_operand.is_null() {
            None
        } else {
            Some(*Box::from_raw(dst_operand))
        };
        
        (*instruction).set_operands(src, dst);
    }
}

#[no_mangle]
pub extern "C" fn rust_segment_new(
    inst_list_ptr: *const *mut RustInstruction,
    inst_list_len: c_uint,
) -> *mut RustSegment {
    if inst_list_ptr.is_null() && inst_list_len > 0 {
        return std::ptr::null_mut();
    }

    let inst_list = if inst_list_len == 0 {
        Vec::new()
    } else {
        unsafe {
            let slice = std::slice::from_raw_parts(inst_list_ptr, inst_list_len as usize);
            slice.iter().map(|&ptr| {
                if ptr.is_null() {
                    // Create a default instruction for null pointers
                    Box::new(RustInstruction::new(0, RustOpcode::OpNone))
                } else {
                    Box::from_raw(ptr)
                }
            }).collect()
        }
    };

    let segment = RustSegment::new(inst_list);
    Box::into_raw(Box::new(segment))
}

#[no_mangle]
pub extern "C" fn rust_segment_free(segment: *mut RustSegment) {
    if !segment.is_null() {
        unsafe {
            drop(Box::from_raw(segment));
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_segment_set_useful_inst_index(segment: *mut RustSegment, idx: c_uint) {
    if !segment.is_null() {
        unsafe {
            (*segment).set_useful_inst_index(idx);
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_segment_get_instruction(
    segment: *const RustSegment,
    index: c_uint,
) -> *const RustInstruction {
    if segment.is_null() {
        return std::ptr::null();
    }
    
    unsafe {
        match (*segment).get_instruction(index) {
            Some(inst) => inst as *const RustInstruction,
            None => std::ptr::null(),
        }
    }
}