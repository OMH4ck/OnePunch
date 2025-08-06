use std::ffi::{CStr};
use std::os::raw::{c_char};

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