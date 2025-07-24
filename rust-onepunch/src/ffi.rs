use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_long, c_uchar, c_uint};
use std::ptr;

use crate::{RustMemory, RustRegister, RustValue, RustValueType, RustRegType};

#[no_mangle]
pub extern "C" fn rust_value_new(value_type: RustValueType, value: c_long) -> *mut RustValue {
    Box::into_raw(Box::new(RustValue::new(value_type, value)))
}

#[no_mangle]
pub extern "C" fn rust_value_free(value: *mut RustValue) {
    if !value.is_null() {
        unsafe {
            drop(Box::from_raw(value));
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_memory_new() -> *mut RustMemory {
    Box::into_raw(Box::new(RustMemory::new()))
}

#[no_mangle]
pub extern "C" fn rust_memory_free(memory: *mut RustMemory) {
    if !memory.is_null() {
        unsafe {
            drop(Box::from_raw(memory));
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_memory_increase_ref_count(memory: *mut RustMemory) {
    if !memory.is_null() {
        unsafe {
            (*memory).increase_ref_count();
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_memory_decrease_ref_count(memory: *mut RustMemory) {
    if !memory.is_null() {
        unsafe {
            (*memory).decrease_ref_count();
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_memory_contain_range(
    memory: *const RustMemory,
    start: c_long,
    end: c_long,
) -> c_uchar {
    if memory.is_null() {
        return 0;
    }
    unsafe {
        if (*memory).contain_range((start, end)) {
            1
        } else {
            0
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_memory_remove_range(
    memory: *mut RustMemory,
    start: c_long,
    end: c_long,
) -> c_uchar {
    if memory.is_null() {
        return 0;
    }
    unsafe {
        if (*memory).remove_range((start, end)) {
            1
        } else {
            0
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_memory_set_content(
    memory: *mut RustMemory,
    offset: c_long,
    value: *const RustValue,
) {
    if memory.is_null() || value.is_null() {
        return;
    }
    unsafe {
        let val = (*value).clone();
        (*memory).set_content(offset, val);
    }
}

#[no_mangle]
pub extern "C" fn rust_register_new(alloc_mem: c_uchar) -> *mut RustRegister {
    Box::into_raw(Box::new(RustRegister::new(alloc_mem != 0)))
}

#[no_mangle]
pub extern "C" fn rust_register_free(register: *mut RustRegister) {
    if !register.is_null() {
        unsafe {
            drop(Box::from_raw(register));
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_register_alias(
    register: *mut RustRegister,
    other: *const RustRegister,
    copy_mem: c_uchar,
) {
    if register.is_null() || other.is_null() {
        return;
    }
    unsafe {
        (*register).alias(&(*other), copy_mem != 0);
    }
}

#[no_mangle]
pub extern "C" fn rust_register_set_name(register: *mut RustRegister, name: RustRegType) {
    if !register.is_null() {
        unsafe {
            (*register).name = name;
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_register_get_name(register: *const RustRegister) -> RustRegType {
    if register.is_null() {
        return RustRegType::RegNone;
    }
    unsafe { (*register).name }
}