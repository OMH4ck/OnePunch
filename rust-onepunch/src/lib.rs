use std::collections::HashMap;
use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_int, c_long, c_uchar, c_uint};
use std::ptr;

pub mod register;
pub mod ffi;
pub mod asmutils;
pub mod asmutils_ffi;
pub mod utils;
pub mod utils_ffi;
pub mod symbolic_executor;
pub mod symbolic_executor_ffi;
pub mod preprocessor;
pub mod preprocessor_ffi;

pub use register::*;
pub use ffi::*;
pub use asmutils::*;
pub use asmutils_ffi::*;
pub use utils::*;
pub use utils_ffi::*;
pub use symbolic_executor::*;
pub use symbolic_executor_ffi::*;
pub use preprocessor::*;
pub use preprocessor_ffi::*;

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub enum RustValueType {
    CallValue = 0,
    MemValue,
    CallRegValue,
    ImmValue,
    OtherValue,
}

#[repr(C)]
#[derive(Debug, Clone)]
pub struct RustValue {
    pub value_type: RustValueType,
    pub value: c_long,
}

impl RustValue {
    pub fn new(value_type: RustValueType, value: c_long) -> Self {
        Self { value_type, value }
    }
}

#[repr(C)]
#[derive(Clone)]
pub struct RustMemory {
    pub ref_count: c_uint,
    pub mem_id: c_uint,
    ranges: Vec<(c_long, c_long)>,
    content: HashMap<c_long, RustValue>,
    input_src: *mut c_char,
    pub input_offset: c_long,
    pub input_action: c_uchar,
}

impl RustMemory {
    pub fn new() -> Self {
        static mut NEXT_MEM_ID: c_uint = 0;
        let mem_id = unsafe {
            let id = NEXT_MEM_ID;
            NEXT_MEM_ID += 1;
            id
        };

        Self {
            ref_count: 0,
            mem_id,
            ranges: Vec::new(),
            content: HashMap::new(),
            input_src: std::ptr::null_mut(),
            input_offset: 0,
            input_action: 0,
        }
    }

    pub fn increase_ref_count(&mut self) {
        self.ref_count += 1;
    }

    pub fn decrease_ref_count(&mut self) {
        if self.ref_count > 0 {
            self.ref_count -= 1;
        }
    }

    pub fn contain_range(&self, range: (c_long, c_long)) -> bool {
        self.ranges.contains(&range)
    }

    pub fn remove_range(&mut self, range: (c_long, c_long)) -> bool {
        if let Some(pos) = self.ranges.iter().position(|&r| r == range) {
            self.ranges.remove(pos);
            true
        } else {
            false
        }
    }

    pub fn set_content(&mut self, offset: c_long, val: RustValue) {
        self.content.insert(offset, val);
    }
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub enum RustRegType {
    RegNone = 0,
    Reg64Start,
    RegRax,
    RegRcx,
    RegRdx,
    RegRbx,
    RegRsi,
    RegRdi,
    RegRsp,
    RegRbp,
    RegR8,
    RegR9,
    RegR10,
    RegR11,
    RegR12,
    RegR13,
    RegR14,
    RegR15,
    RegRip,
}

#[repr(C)]
pub struct RustRegister {
    pub name: RustRegType,
    memory: Option<Box<RustMemory>>,
    pub base_offset: c_long,
    input_src: *mut c_char,
    pub input_offset: c_long,
    pub input_action: c_uchar,
}

impl RustRegister {
    pub fn new(alloc_mem: bool) -> Self {
        Self {
            name: RustRegType::RegNone,
            memory: if alloc_mem { Some(Box::new(RustMemory::new())) } else { None },
            base_offset: 0,
            input_src: std::ptr::null_mut(),
            input_offset: 0,
            input_action: 0,
        }
    }

    pub fn alias(&mut self, other: &RustRegister, copy_mem: bool) {
        if copy_mem {
            if let Some(ref other_mem) = other.memory {
                self.memory = Some((*other_mem).clone());
            }
        }
        self.name = other.name;
        self.base_offset = other.base_offset;
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_value_creation() {
        let val = RustValue::new(RustValueType::ImmValue, 42);
        assert_eq!(val.value, 42);
    }

    #[test]
    fn test_memory_operations() {
        let mut mem = RustMemory::new();
        mem.increase_ref_count();
        assert_eq!(mem.ref_count, 1);
        
        let val = RustValue::new(RustValueType::ImmValue, 100);
        mem.set_content(0x1000, val);
        assert!(mem.content.contains_key(&0x1000));
    }

    #[test]
    fn test_register_creation() {
        let reg = RustRegister::new(true);
        assert!(reg.memory.is_some());
        assert_eq!(reg.base_offset, 0);
    }
}