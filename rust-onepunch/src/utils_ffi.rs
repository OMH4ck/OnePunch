use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_double, c_uchar, c_ulong, c_uint};
use std::ptr;

use crate::utils::*;

#[no_mangle]
pub extern "C" fn rust_transfer_operation_len_to_str(dtype: c_uint) -> *const c_char {
    let byte_slice = transfer_operation_len_to_str(dtype);
    byte_slice.as_ptr() as *const c_char
}

#[no_mangle]
pub extern "C" fn rust_string_hash(input: *const c_char) -> c_ulong {
    if input.is_null() {
        return 0;
    }
    
    unsafe {
        let c_str = CStr::from_ptr(input);
        if let Ok(str_slice) = c_str.to_str() {
            string_hash(str_slice)
        } else {
            0
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_gen_id() -> c_ulong {
    gen_id()
}

#[no_mangle]
pub extern "C" fn rust_is_imm(input: *const c_char) -> c_uchar {
    if input.is_null() {
        return 0;
    }
    
    unsafe {
        let c_str = CStr::from_ptr(input);
        if let Ok(str_slice) = c_str.to_str() {
            if is_imm(str_slice) { 1 } else { 0 }
        } else {
            0
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_str_split(
    input: *const c_char, 
    delimiter: *const c_char,
    result_array: *mut *mut c_char,
    max_results: c_uint
) -> c_uint {
    if input.is_null() || delimiter.is_null() || result_array.is_null() {
        return 0;
    }
    
    unsafe {
        let input_str = match CStr::from_ptr(input).to_str() {
            Ok(s) => s,
            Err(_) => return 0,
        };
        
        let delimiter_str = match CStr::from_ptr(delimiter).to_str() {
            Ok(s) => s,
            Err(_) => return 0,
        };
        
        let parts = str_split(input_str, delimiter_str);
        let count = std::cmp::min(parts.len(), max_results as usize);
        
        for (i, part) in parts.iter().take(count).enumerate() {
            if let Ok(c_string) = CString::new(part.as_str()) {
                *result_array.add(i) = c_string.into_raw();
            }
        }
        
        count as c_uint
    }
}

#[no_mangle]
pub extern "C" fn rust_str_trim(input: *const c_char) -> *mut c_char {
    if input.is_null() {
        return ptr::null_mut();
    }
    
    unsafe {
        let c_str = CStr::from_ptr(input);
        if let Ok(str_slice) = c_str.to_str() {
            let trimmed = str_trim(str_slice);
            if let Ok(c_string) = CString::new(trimmed) {
                c_string.into_raw()
            } else {
                ptr::null_mut()
            }
        } else {
            ptr::null_mut()
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_get_cur_time() -> c_double {
    get_cur_time()
}

#[no_mangle]
pub extern "C" fn rust_free_string(s: *mut c_char) {
    if !s.is_null() {
        unsafe {
            drop(CString::from_raw(s));
        }
    }
}