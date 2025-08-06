use std::os::raw::{c_char, c_uchar, c_uint, c_ulong};
use std::ptr;

use crate::{RustSymbolicExecutor, RustSegment, RustRegister, RustReg};

#[no_mangle]
pub extern "C" fn rust_symbolic_executor_new() -> *mut RustSymbolicExecutor {
    Box::into_raw(Box::new(RustSymbolicExecutor::new()))
}

#[no_mangle]
pub extern "C" fn rust_symbolic_executor_free(executor: *mut RustSymbolicExecutor) {
    if !executor.is_null() {
        unsafe {
            drop(Box::from_raw(executor));
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_symbolic_executor_execute_instructions(
    executor: *mut RustSymbolicExecutor,
    segment: *const RustSegment,
    reg_list_ptr: *mut *mut RustRegister,
    reg_list_len: c_uint,
    record_flag: c_uchar,
) -> c_uchar {
    if executor.is_null() || segment.is_null() || reg_list_ptr.is_null() {
        return 0;
    }

    unsafe {
        let executor_ref = &mut *executor;
        let segment_ref = &*segment;
        let mut reg_vec = Vec::from_raw_parts(reg_list_ptr, reg_list_len as usize, reg_list_len as usize);
        
        let result = executor_ref.execute_instructions(segment_ref, &mut reg_vec, record_flag != 0);
        
        // Don't drop the vec - we're just borrowing it
        std::mem::forget(reg_vec);
        
        if result { 1 } else { 0 }
    }
}

#[no_mangle]
pub extern "C" fn rust_symbolic_executor_is_in_input(
    reg: RustReg,
    reg_list_ptr: *const *const RustRegister,
    reg_list_len: c_uint,
) -> c_uchar {
    if reg_list_ptr.is_null() {
        return 0;
    }

    unsafe {
        let reg_slice = std::slice::from_raw_parts(reg_list_ptr, reg_list_len as usize);
        if RustSymbolicExecutor::is_in_input(reg, reg_slice) { 1 } else { 0 }
    }
}

#[no_mangle]
pub extern "C" fn rust_symbolic_executor_get_reg_by_idx(
    reg: RustReg,
    reg_list_ptr: *mut *mut RustRegister,
    reg_list_len: c_uint,
) -> *mut RustRegister {
    if reg_list_ptr.is_null() {
        return ptr::null_mut();
    }

    unsafe {
        let reg_slice = std::slice::from_raw_parts_mut(reg_list_ptr, reg_list_len as usize);
        RustSymbolicExecutor::get_reg_by_idx(reg, reg_slice).unwrap_or(ptr::null_mut())
    }
}

#[no_mangle]
pub extern "C" fn rust_symbolic_executor_is_solution(
    must_control_list_ptr: *const (RustReg, c_uint),
    must_control_list_len: c_uint,
    reg_list_ptr: *const *const RustRegister,
    reg_list_len: c_uint,
) -> c_uchar {
    if must_control_list_ptr.is_null() || reg_list_ptr.is_null() {
        return 0;
    }

    unsafe {
        let must_control_slice = std::slice::from_raw_parts(must_control_list_ptr, must_control_list_len as usize);
        let reg_slice = std::slice::from_raw_parts(reg_list_ptr, reg_list_len as usize);
        
        if RustSymbolicExecutor::is_solution(must_control_slice, reg_slice) { 1 } else { 0 }
    }
}

#[no_mangle]
pub extern "C" fn rust_symbolic_executor_prepare_reg_list(
    reg_names_ptr: *const RustReg,
    reg_names_len: c_uint,
    result_ptr: *mut *mut RustRegister,
    max_results: c_uint,
) -> c_uint {
    if reg_names_ptr.is_null() || result_ptr.is_null() {
        return 0;
    }

    unsafe {
        let reg_names_slice = std::slice::from_raw_parts(reg_names_ptr, reg_names_len as usize);
        let reg_list = RustSymbolicExecutor::prepare_reg_list(reg_names_slice);
        
        let count = std::cmp::min(reg_list.len(), max_results as usize);
        let result_slice = std::slice::from_raw_parts_mut(result_ptr, count);
        
        for (i, &reg_ptr) in reg_list.iter().take(count).enumerate() {
            result_slice[i] = reg_ptr;
        }
        
        count as c_uint
    }
}