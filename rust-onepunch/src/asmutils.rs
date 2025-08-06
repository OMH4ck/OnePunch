use std::os::raw::{c_char, c_long, c_uchar, c_uint, c_ulong};
use std::collections::HashMap;

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum RustOpcode {
    OpNone = 0,
    OpMov,
    OpLea,
    OpPop,
    OpAdd,
    OpSub,
    OpImul,
    OpMul,
    OpDiv,
    OpPush,
    OpXor,
    OpOr,
    OpAnd,
    OpShr,
    OpShl,
    OpRor,
    OpSar,
    OpTest,
    OpNop,
    OpCmp,
    OpCall,
    OpJmp,
    OpXchg,
    OpJcc,
    OpRet,
    OpSyscall,
    OpInt3,
    OpSfence,
    OpBswap,
    OpMovaps,
    OpMovdqa,
    OpMovntdq,
    OpMovsxd,
}

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RustOperationLength {
    None = 0,
    Byte = 1,
    Word = 2,
    Dword = 4,
    Qword = 8,
}

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum RustReg {
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
    Reg64End,
    Reg32Start,
    RegEax,
    RegEcx,
    RegEdx,
    RegEbx,
    RegEsi,
    RegEdi,
    RegEsp,
    RegEbp,
    RegR8d,
    RegR9d,
    RegR10d,
    RegR11d,
    RegR12d,
    RegR13d,
    RegR14d,
    RegR15d,
    RegEip,
    Reg32End,
    Reg16Start,
    RegAx,
    RegCx,
    RegDx,
    RegBx,
    RegSi,
    RegDi,
    RegSp,
    RegBp,
    RegR8w,
    RegR9w,
    RegR10w,
    RegR11w,
    RegR12w,
    RegR13w,
    RegR14w,
    RegR15w,
    RegIp,
    Reg16End,
    Reg8Start,
    RegAl,
    RegCl,
    RegDl,
    RegBl,
    RegSil,
    RegDil,
    RegSpl,
    RegBpl,
    RegR8b,
    RegR9b,
    RegR10b,
    RegR11b,
    RegR12b,
    RegR13b,
    RegR14b,
    RegR15b,
    Reg8End,
    Reg8hStart,
    RegAh,
    RegCh,
    RegDh,
    RegBh,
    RegSih,
    RegDih,
    RegSph,
    RegBph,
    RegR8h,
    RegR9h,
    RegR10h,
    RegR11h,
    RegR12h,
    RegR13h,
    RegR14h,
    RegR15h,
    Reg8hEnd,
    RegCr4,
    RegCr3,
}

#[repr(C)]
#[derive(Clone)]
pub struct RustOperand {
    pub is_dereference: c_uchar,
    pub contain_seg_reg: c_uchar,
    pub reg_list_ptr: *mut (RustReg, c_uint),
    pub reg_list_len: c_uint,
    pub literal_sym: c_uchar,
    pub literal_num: c_ulong,
    pub reg_num: c_uint,
    pub operation_length: RustOperationLength,
    pub imm: c_long,
}

impl RustOperand {
    pub fn new(
        is_dereference: bool,
        is_contain_seg_reg: bool,
        reg_list: Vec<(RustReg, c_uint)>,
        literal_sym: bool,
        literal_num: c_ulong,
        operation_length: RustOperationLength,
    ) -> Self {
        let reg_list_len = reg_list.len() as c_uint;
        let reg_list_ptr = if reg_list.is_empty() {
            std::ptr::null_mut()
        } else {
            let boxed_slice = reg_list.into_boxed_slice();
            Box::into_raw(boxed_slice) as *mut (RustReg, c_uint)
        };

        Self {
            is_dereference: if is_dereference { 1 } else { 0 },
            contain_seg_reg: if is_contain_seg_reg { 1 } else { 0 },
            reg_list_ptr,
            reg_list_len,
            literal_sym: if literal_sym { 1 } else { 0 },
            literal_num,
            reg_num: reg_list_len,
            operation_length,
            imm: 0,
        }
    }

    pub fn is_literal_number(&self) -> bool {
        self.reg_num == 0 && !self.is_dereference()
    }

    pub fn is_reg_operation(&self) -> bool {
        self.reg_num == 1 && !self.is_dereference()
    }

    pub fn contain_reg(&self, reg: RustReg) -> bool {
        if self.reg_list_ptr.is_null() || self.reg_list_len == 0 {
            return false;
        }
        
        unsafe {
            let slice = std::slice::from_raw_parts(self.reg_list_ptr, self.reg_list_len as usize);
            slice.iter().any(|(r, _)| *r == reg)
        }
    }

    pub fn is_reg64_operation(&self) -> bool {
        if !self.is_reg_operation() {
            return false;
        }
        
        if self.reg_list_ptr.is_null() {
            return false;
        }
        
        unsafe {
            let slice = std::slice::from_raw_parts(self.reg_list_ptr, 1);
            let reg_val = slice[0].0 as u32;
            reg_val > RustReg::Reg64Start as u32 && reg_val < RustReg::Reg64End as u32
        }
    }

    pub fn is_memory_access(&self) -> bool {
        self.is_dereference()
    }

    pub fn contain_segment_reg(&self) -> bool {
        self.contain_seg_reg != 0
    }

    pub fn get_reg_op(&self) -> RustReg {
        if self.is_reg_operation() && !self.reg_list_ptr.is_null() {
            unsafe {
                let slice = std::slice::from_raw_parts(self.reg_list_ptr, 1);
                slice[0].0
            }
        } else {
            RustReg::RegNone
        }
    }

    pub fn is_dereference(&self) -> bool {
        self.is_dereference != 0
    }
}

impl Drop for RustOperand {
    fn drop(&mut self) {
        if !self.reg_list_ptr.is_null() && self.reg_list_len > 0 {
            unsafe {
                let slice = Box::from_raw(std::slice::from_raw_parts_mut(
                    self.reg_list_ptr, 
                    self.reg_list_len as usize
                ));
                drop(slice);
            }
        }
    }
}

#[repr(C)]
pub struct RustInstruction {
    pub offset: c_ulong,
    pub opcode: RustOpcode,
    pub op_src: *mut RustOperand,
    pub op_dst: *mut RustOperand,
    pub operand_num: c_uint,
    pub operation_length: RustOperationLength,
    pub original_inst: *mut c_char,
}

impl RustInstruction {
    pub fn new(offset: c_ulong, opcode: RustOpcode) -> Self {
        Self {
            offset,
            opcode,
            op_src: std::ptr::null_mut(),
            op_dst: std::ptr::null_mut(),
            operand_num: 0,
            operation_length: RustOperationLength::None,
            original_inst: std::ptr::null_mut(),
        }
    }

    pub fn is_reg_operation(&self) -> bool {
        if self.op_dst.is_null() {
            return false;
        }
        unsafe {
            (*self.op_dst).is_reg_operation()
        }
    }
    
    pub fn set_operands(&mut self, src: Option<RustOperand>, dst: Option<RustOperand>) {
        // Clean up existing operands
        if !self.op_src.is_null() {
            unsafe {
                drop(Box::from_raw(self.op_src));
            }
        }
        if !self.op_dst.is_null() {
            unsafe {
                drop(Box::from_raw(self.op_dst));
            }
        }
        
        // Set new operands
        self.op_src = src.map(|op| Box::into_raw(Box::new(op))).unwrap_or(std::ptr::null_mut());
        self.op_dst = dst.map(|op| Box::into_raw(Box::new(op))).unwrap_or(std::ptr::null_mut());
        
        self.operand_num = (if !self.op_src.is_null() { 1 } else { 0 }) + 
                          (if !self.op_dst.is_null() { 1 } else { 0 });
    }
}

impl Drop for RustInstruction {
    fn drop(&mut self) {
        if !self.op_src.is_null() {
            unsafe {
                drop(Box::from_raw(self.op_src));
            }
        }
        if !self.op_dst.is_null() {
            unsafe {
                drop(Box::from_raw(self.op_dst));
            }
        }
        if !self.original_inst.is_null() {
            unsafe {
                drop(std::ffi::CString::from_raw(self.original_inst));
            }
        }
    }
}

#[repr(C)]
pub struct RustSegment {
    pub inst_list_ptr: *mut *mut RustInstruction,
    pub inst_list_len: c_uint,
    pub useful_inst_index: c_uint,
}

impl RustSegment {
    pub fn new(inst_list: Vec<Box<RustInstruction>>) -> Self {
        let inst_list_len = inst_list.len() as c_uint;
        let inst_list_ptr = if inst_list.is_empty() {
            std::ptr::null_mut()
        } else {
            let ptrs: Vec<*mut RustInstruction> = inst_list.into_iter()
                .map(|boxed_inst| Box::into_raw(boxed_inst))
                .collect();
            let boxed_slice = ptrs.into_boxed_slice();
            Box::into_raw(boxed_slice) as *mut *mut RustInstruction
        };

        Self {
            inst_list_ptr,
            inst_list_len,
            useful_inst_index: 0,
        }
    }

    pub fn set_useful_inst_index(&mut self, idx: c_uint) {
        self.useful_inst_index = idx;
    }
    
    pub fn get_instruction(&self, index: c_uint) -> Option<&RustInstruction> {
        if index >= self.inst_list_len || self.inst_list_ptr.is_null() {
            return None;
        }
        
        unsafe {
            let slice = std::slice::from_raw_parts(self.inst_list_ptr, self.inst_list_len as usize);
            if slice[index as usize].is_null() {
                None
            } else {
                Some(&*slice[index as usize])
            }
        }
    }
}

impl Drop for RustSegment {
    fn drop(&mut self) {
        if !self.inst_list_ptr.is_null() && self.inst_list_len > 0 {
            unsafe {
                let slice = std::slice::from_raw_parts_mut(self.inst_list_ptr, self.inst_list_len as usize);
                for ptr in slice.iter_mut() {
                    if !ptr.is_null() {
                        drop(Box::from_raw(*ptr));
                    }
                }
                let boxed_slice = Box::from_raw(slice);
                drop(boxed_slice);
            }
        }
    }
}

pub fn transfer_op_to_str(opcode: RustOpcode) -> &'static [u8] {
    match opcode {
        RustOpcode::OpNone => b"none\0",
        RustOpcode::OpMov => b"mov\0",
        RustOpcode::OpLea => b"lea\0",
        RustOpcode::OpPop => b"pop\0",
        RustOpcode::OpAdd => b"add\0",
        RustOpcode::OpSub => b"sub\0",
        RustOpcode::OpImul => b"imul\0",
        RustOpcode::OpMul => b"mul\0",
        RustOpcode::OpDiv => b"div\0",
        RustOpcode::OpPush => b"push\0",
        RustOpcode::OpXor => b"xor\0",
        RustOpcode::OpOr => b"or\0",
        RustOpcode::OpAnd => b"and\0",
        RustOpcode::OpShr => b"shr\0",
        RustOpcode::OpShl => b"shl\0",
        RustOpcode::OpRor => b"ror\0",
        RustOpcode::OpSar => b"sar\0",
        RustOpcode::OpTest => b"test\0",
        RustOpcode::OpNop => b"nop\0",
        RustOpcode::OpCmp => b"cmp\0",
        RustOpcode::OpCall => b"call\0",
        RustOpcode::OpJmp => b"jmp\0",
        RustOpcode::OpXchg => b"xchg\0",
        RustOpcode::OpJcc => b"jcc\0",
        RustOpcode::OpRet => b"ret\0",
        RustOpcode::OpSyscall => b"syscall\0",
        RustOpcode::OpInt3 => b"int3\0",
        RustOpcode::OpSfence => b"sfence\0",
        RustOpcode::OpBswap => b"bswap\0",
        RustOpcode::OpMovaps => b"movaps\0",
        RustOpcode::OpMovdqa => b"movdqa\0",
        RustOpcode::OpMovntdq => b"movntdq\0",
        RustOpcode::OpMovsxd => b"movsxd\0",
    }
}

pub fn transfer_str_to_op(op_str: &str) -> RustOpcode {
    match op_str {
        "mov" => RustOpcode::OpMov,
        "lea" => RustOpcode::OpLea,
        "pop" => RustOpcode::OpPop,
        "add" => RustOpcode::OpAdd,
        "sub" => RustOpcode::OpSub,
        "imul" => RustOpcode::OpImul,
        "mul" => RustOpcode::OpMul,
        "div" => RustOpcode::OpDiv,
        "push" => RustOpcode::OpPush,
        "xor" => RustOpcode::OpXor,
        "or" => RustOpcode::OpOr,
        "and" => RustOpcode::OpAnd,
        "shr" => RustOpcode::OpShr,
        "shl" => RustOpcode::OpShl,
        "ror" => RustOpcode::OpRor,
        "sar" => RustOpcode::OpSar,
        "test" => RustOpcode::OpTest,
        "nop" => RustOpcode::OpNop,
        "cmp" => RustOpcode::OpCmp,
        "call" => RustOpcode::OpCall,
        "jmp" => RustOpcode::OpJmp,
        "xchg" => RustOpcode::OpXchg,
        "ret" => RustOpcode::OpRet,
        "syscall" => RustOpcode::OpSyscall,
        "int3" => RustOpcode::OpInt3,
        "sfence" => RustOpcode::OpSfence,
        "bswap" => RustOpcode::OpBswap,
        "movaps" => RustOpcode::OpMovaps,
        "movdqa" => RustOpcode::OpMovdqa,
        "movntdq" => RustOpcode::OpMovntdq,
        "movsxd" => RustOpcode::OpMovsxd,
        _ => {
            if op_str.starts_with('j') && op_str != "jmp" {
                RustOpcode::OpJcc
            } else {
                RustOpcode::OpNone
            }
        }
    }
}

pub fn get_reg_by_str(reg_str: &str) -> RustReg {
    match reg_str {
        "rax" => RustReg::RegRax,
        "rcx" => RustReg::RegRcx,
        "rdx" => RustReg::RegRdx,
        "rbx" => RustReg::RegRbx,
        "rsi" => RustReg::RegRsi,
        "rdi" => RustReg::RegRdi,
        "rsp" => RustReg::RegRsp,
        "rbp" => RustReg::RegRbp,
        "r8" => RustReg::RegR8,
        "r9" => RustReg::RegR9,
        "r10" => RustReg::RegR10,
        "r11" => RustReg::RegR11,
        "r12" => RustReg::RegR12,
        "r13" => RustReg::RegR13,
        "r14" => RustReg::RegR14,
        "r15" => RustReg::RegR15,
        "rip" => RustReg::RegRip,
        "eax" => RustReg::RegEax,
        "ecx" => RustReg::RegEcx,
        "edx" => RustReg::RegEdx,
        "ebx" => RustReg::RegEbx,
        "esi" => RustReg::RegEsi,
        "edi" => RustReg::RegEdi,
        "esp" => RustReg::RegEsp,
        "ebp" => RustReg::RegEbp,
        "r8d" => RustReg::RegR8d,
        "r9d" => RustReg::RegR9d,
        "r10d" => RustReg::RegR10d,
        "r11d" => RustReg::RegR11d,
        "r12d" => RustReg::RegR12d,
        "r13d" => RustReg::RegR13d,
        "r14d" => RustReg::RegR14d,
        "r15d" => RustReg::RegR15d,
        "eip" => RustReg::RegEip,
        "ax" => RustReg::RegAx,
        "cx" => RustReg::RegCx,
        "dx" => RustReg::RegDx,
        "bx" => RustReg::RegBx,
        "si" => RustReg::RegSi,
        "di" => RustReg::RegDi,
        "sp" => RustReg::RegSp,
        "bp" => RustReg::RegBp,
        "r8w" => RustReg::RegR8w,
        "r9w" => RustReg::RegR9w,
        "r10w" => RustReg::RegR10w,
        "r11w" => RustReg::RegR11w,
        "r12w" => RustReg::RegR12w,
        "r13w" => RustReg::RegR13w,
        "r14w" => RustReg::RegR14w,
        "r15w" => RustReg::RegR15w,
        "ip" => RustReg::RegIp,
        "al" => RustReg::RegAl,
        "cl" => RustReg::RegCl,
        "dl" => RustReg::RegDl,
        "bl" => RustReg::RegBl,
        "sil" => RustReg::RegSil,
        "dil" => RustReg::RegDil,
        "spl" => RustReg::RegSpl,
        "bpl" => RustReg::RegBpl,
        "r8b" => RustReg::RegR8b,
        "r9b" => RustReg::RegR9b,
        "r10b" => RustReg::RegR10b,
        "r11b" => RustReg::RegR11b,
        "r12b" => RustReg::RegR12b,
        "r13b" => RustReg::RegR13b,
        "r14b" => RustReg::RegR14b,
        "r15b" => RustReg::RegR15b,
        "ah" => RustReg::RegAh,
        "ch" => RustReg::RegCh,
        "dh" => RustReg::RegDh,
        "bh" => RustReg::RegBh,
        "sih" => RustReg::RegSih,
        "dih" => RustReg::RegDih,
        "sph" => RustReg::RegSph,
        "bph" => RustReg::RegBph,
        "r8h" => RustReg::RegR8h,
        "r9h" => RustReg::RegR9h,
        "r10h" => RustReg::RegR10h,
        "r11h" => RustReg::RegR11h,
        "r12h" => RustReg::RegR12h,
        "r13h" => RustReg::RegR13h,
        "r14h" => RustReg::RegR14h,
        "r15h" => RustReg::RegR15h,
        "cr4" => RustReg::RegCr4,
        "cr3" => RustReg::RegCr3,
        _ => RustReg::RegNone,
    }
}

pub fn get_reg_str_by_reg(reg: RustReg) -> &'static [u8] {
    match reg {
        RustReg::RegRax => b"rax\0",
        RustReg::RegRcx => b"rcx\0",
        RustReg::RegRdx => b"rdx\0",
        RustReg::RegRbx => b"rbx\0",
        RustReg::RegRsi => b"rsi\0",
        RustReg::RegRdi => b"rdi\0",
        RustReg::RegRsp => b"rsp\0",
        RustReg::RegRbp => b"rbp\0",
        RustReg::RegR8 => b"r8\0",
        RustReg::RegR9 => b"r9\0",
        RustReg::RegR10 => b"r10\0",
        RustReg::RegR11 => b"r11\0",
        RustReg::RegR12 => b"r12\0",
        RustReg::RegR13 => b"r13\0",
        RustReg::RegR14 => b"r14\0",
        RustReg::RegR15 => b"r15\0",
        RustReg::RegRip => b"rip\0",
        RustReg::RegEax => b"eax\0",
        RustReg::RegEcx => b"ecx\0",
        RustReg::RegEdx => b"edx\0",
        RustReg::RegEbx => b"ebx\0",
        RustReg::RegEsi => b"esi\0",
        RustReg::RegEdi => b"edi\0",
        RustReg::RegEsp => b"esp\0",
        RustReg::RegEbp => b"ebp\0",
        RustReg::RegR8d => b"r8d\0",
        RustReg::RegR9d => b"r9d\0",
        RustReg::RegR10d => b"r10d\0",
        RustReg::RegR11d => b"r11d\0",
        RustReg::RegR12d => b"r12d\0",
        RustReg::RegR13d => b"r13d\0",
        RustReg::RegR14d => b"r14d\0",
        RustReg::RegR15d => b"r15d\0",
        RustReg::RegEip => b"eip\0",
        RustReg::RegAx => b"ax\0",
        RustReg::RegCx => b"cx\0",
        RustReg::RegDx => b"dx\0",
        RustReg::RegBx => b"bx\0",
        RustReg::RegSi => b"si\0",
        RustReg::RegDi => b"di\0",
        RustReg::RegSp => b"sp\0",
        RustReg::RegBp => b"bp\0",
        RustReg::RegR8w => b"r8w\0",
        RustReg::RegR9w => b"r9w\0",
        RustReg::RegR10w => b"r10w\0",
        RustReg::RegR11w => b"r11w\0",
        RustReg::RegR12w => b"r12w\0",
        RustReg::RegR13w => b"r13w\0",
        RustReg::RegR14w => b"r14w\0",
        RustReg::RegR15w => b"r15w\0",
        RustReg::RegIp => b"ip\0",
        RustReg::RegAl => b"al\0",
        RustReg::RegCl => b"cl\0",
        RustReg::RegDl => b"dl\0",
        RustReg::RegBl => b"bl\0",
        RustReg::RegSil => b"sil\0",
        RustReg::RegDil => b"dil\0",
        RustReg::RegSpl => b"spl\0",
        RustReg::RegBpl => b"bpl\0",
        RustReg::RegR8b => b"r8b\0",
        RustReg::RegR9b => b"r9b\0",
        RustReg::RegR10b => b"r10b\0",
        RustReg::RegR11b => b"r11b\0",
        RustReg::RegR12b => b"r12b\0",
        RustReg::RegR13b => b"r13b\0",
        RustReg::RegR14b => b"r14b\0",
        RustReg::RegR15b => b"r15b\0",
        RustReg::RegAh => b"ah\0",
        RustReg::RegCh => b"ch\0",
        RustReg::RegDh => b"dh\0",
        RustReg::RegBh => b"bh\0",
        RustReg::RegSih => b"sih\0",
        RustReg::RegDih => b"dih\0",
        RustReg::RegSph => b"sph\0",
        RustReg::RegBph => b"bph\0",
        RustReg::RegR8h => b"r8h\0",
        RustReg::RegR9h => b"r9h\0",
        RustReg::RegR10h => b"r10h\0",
        RustReg::RegR11h => b"r11h\0",
        RustReg::RegR12h => b"r12h\0",
        RustReg::RegR13h => b"r13h\0",
        RustReg::RegR14h => b"r14h\0",
        RustReg::RegR15h => b"r15h\0",
        RustReg::RegCr4 => b"cr4\0",
        RustReg::RegCr3 => b"cr3\0",
        _ => b"none\0",
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_opcode_conversion() {
        assert_eq!(transfer_str_to_op("mov"), RustOpcode::OpMov);
        assert_eq!(transfer_str_to_op("call"), RustOpcode::OpCall);
        assert_eq!(transfer_str_to_op("jne"), RustOpcode::OpJcc);
        assert_eq!(transfer_op_to_str(RustOpcode::OpMov), b"mov\0");
    }

    #[test]
    fn test_register_conversion() {
        assert_eq!(get_reg_by_str("rax"), RustReg::RegRax);
        assert_eq!(get_reg_by_str("r8"), RustReg::RegR8);
        assert_eq!(get_reg_str_by_reg(RustReg::RegRdi), b"rdi\0");
    }

    // Additional tests can be added once we implement the complex structures
}