use std::os::raw::{c_uchar, c_uint, c_ulong};
use crate::{RustSolver, RustSegment, RustRegister, RustPreprocessor, RustReg};

#[no_mangle]
pub extern "C" fn rust_solver_new(
    code_segments_ptr: *const *mut RustSegment,
    code_segments_len: c_uint,
    must_control_list_ptr: *const (RustReg, c_uint),
    must_control_list_len: c_uint,
    reg_list_ptr: *const *const RustRegister,
    reg_list_len: c_uint,
    search_level: c_ulong,
    preprocessor_ptr: *const RustPreprocessor,
) -> *mut RustSolver {
    if code_segments_ptr.is_null() || must_control_list_ptr.is_null() || reg_list_ptr.is_null() {
        return std::ptr::null_mut();
    }
    
    unsafe {
        let code_segments = std::slice::from_raw_parts(code_segments_ptr, code_segments_len as usize);
        let must_control_list = std::slice::from_raw_parts(must_control_list_ptr, must_control_list_len as usize);
        let reg_list = std::slice::from_raw_parts(reg_list_ptr, reg_list_len as usize);
        
        let solver = RustSolver::new(
            code_segments,
            must_control_list,
            reg_list,
            search_level,
            preprocessor_ptr,
        );
        
        Box::into_raw(Box::new(solver))
    }
}

#[no_mangle]
pub extern "C" fn rust_solver_free(solver: *mut RustSolver) {
    if !solver.is_null() {
        unsafe {
            drop(Box::from_raw(solver));
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_solver_dfs(
    solver: *mut RustSolver,
    output_register_ptr: *mut *mut RustRegister,
    output_register_capacity: c_uint,
    output_register_len: *mut c_uint,
    output_segments_ptr: *mut (*mut RustSegment, c_uint),
    output_segments_capacity: c_uint,
    output_segments_len: *mut c_uint,
) -> c_uchar {
    if solver.is_null() || output_register_ptr.is_null() || output_segments_ptr.is_null() {
        return 0;
    }
    
    unsafe {
        let solver_ref = &mut *solver;
        let mut output_register = Vec::new();
        let mut output_segments = Vec::new();
        
        let result = solver_ref.dfs(&mut output_register, &mut output_segments);
        
        // Copy output registers to the provided buffer
        if !output_register_len.is_null() {
            let reg_count = std::cmp::min(output_register.len(), output_register_capacity as usize);
            *output_register_len = reg_count as c_uint;
            
            let output_reg_slice = std::slice::from_raw_parts_mut(output_register_ptr, reg_count);
            for (i, &reg_ptr) in output_register.iter().take(reg_count).enumerate() {
                output_reg_slice[i] = reg_ptr;
            }
        }
        
        // Copy output segments to the provided buffer
        if !output_segments_len.is_null() {
            let seg_count = std::cmp::min(output_segments.len(), output_segments_capacity as usize);
            *output_segments_len = seg_count as c_uint;
            
            let output_seg_slice = std::slice::from_raw_parts_mut(output_segments_ptr, seg_count);
            for (i, &(seg_ptr, start_idx)) in output_segments.iter().take(seg_count).enumerate() {
                output_seg_slice[i] = (seg_ptr, start_idx);
            }
        }
        
        if result { 1 } else { 0 }
    }
}

#[no_mangle]
pub extern "C" fn rust_solver_hash_reg_list(
    reg_list_ptr: *const *const RustRegister,
    reg_list_len: c_uint,
) -> c_ulong {
    if reg_list_ptr.is_null() {
        return 0;
    }
    
    unsafe {
        let reg_list = std::slice::from_raw_parts(reg_list_ptr, reg_list_len as usize);
        
        // Simple hash implementation
        let mut hash = 0u64;
        for &reg_ptr in reg_list {
            if !reg_ptr.is_null() {
                hash ^= reg_ptr as u64;
            }
        }
        
        hash
    }
}

#[no_mangle]
pub extern "C" fn rust_solver_is_solution(
    must_control_list_ptr: *const (RustReg, c_uint),
    must_control_list_len: c_uint,
    reg_list_ptr: *const *const RustRegister,
    reg_list_len: c_uint,
) -> c_uchar {
    if must_control_list_ptr.is_null() || reg_list_ptr.is_null() {
        return 0;
    }
    
    unsafe {
        let must_control_list = std::slice::from_raw_parts(must_control_list_ptr, must_control_list_len as usize);
        let reg_list = std::slice::from_raw_parts(reg_list_ptr, reg_list_len as usize);
        
        for &(target_reg, min_control) in must_control_list {
            let mut found = false;
            
            for &reg_ptr in reg_list {
                if !reg_ptr.is_null() {
                    if (*reg_ptr).name as u32 == target_reg as u32 {
                        // Check control level - simplified for now
                        if min_control <= 1 {
                            found = true;
                            break;
                        }
                    }
                }
            }
            
            if !found {
                return 0;
            }
        }
        
        1
    }
}