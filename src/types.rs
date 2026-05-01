//! Core data types for OnePunch

use std::cell::RefCell;
use std::collections::BTreeMap;
use std::fmt;
use std::rc::Rc;
use std::sync::Arc;

/// Memory address infinity constant
pub const MEM_INF: i64 = 0x10000000;

/// A unique memory ID counter
static NEXT_MEM_ID: std::sync::atomic::AtomicU32 = std::sync::atomic::AtomicU32::new(0);

fn next_mem_id() -> u32 {
    NEXT_MEM_ID.fetch_add(1, std::sync::atomic::Ordering::SeqCst)
}

/// Reset memory ID counter (for testing)
pub fn reset_mem_id() {
    NEXT_MEM_ID.store(0, std::sync::atomic::Ordering::SeqCst);
}

/// Operation length in bytes
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
#[repr(u8)]
pub enum OperationLength {
    None = 0,
    Byte = 1,
    Word = 2,
    DWord = 4,
    QWord = 8,
}

impl Default for OperationLength {
    fn default() -> Self {
        Self::None
    }
}

/// x86_64 opcodes
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
pub enum Opcode {
    #[default]
    None,
    Mov,
    Lea,
    Pop,
    Add,
    Sub,
    Imul,
    Mul,
    Div,
    Push,
    Xor,
    Or,
    And,
    Shr,
    Shl,
    Ror,
    Sar,
    Test,
    Nop,
    Cmp,
    Call,
    Jmp,
    Xchg,
    Jcc,
    Ret,
    Syscall,
    Int3,
    Sfence,
    Bswap,
    Movaps,
    Movdqa,
    Movntdq,
    Movsxd,
}

/// x86_64 registers
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, PartialOrd, Ord, Default)]
#[repr(u8)]
pub enum Reg {
    #[default]
    None = 0,
    // 64-bit registers
    Reg64Start,
    Rax,
    Rcx,
    Rdx,
    Rbx,
    Rsi,
    Rdi,
    Rsp,
    Rbp,
    R8,
    R9,
    R10,
    R11,
    R12,
    R13,
    R14,
    R15,
    Rip,
    Reg64End,
    // 32-bit registers
    Reg32Start,
    Eax,
    Ecx,
    Edx,
    Ebx,
    Esi,
    Edi,
    Esp,
    Ebp,
    R8d,
    R9d,
    R10d,
    R11d,
    R12d,
    R13d,
    R14d,
    R15d,
    Eip,
    Reg32End,
    // 16-bit registers
    Reg16Start,
    Ax,
    Cx,
    Dx,
    Bx,
    Si,
    Di,
    Sp,
    Bp,
    R8w,
    R9w,
    R10w,
    R11w,
    R12w,
    R13w,
    R14w,
    R15w,
    Ip,
    Reg16End,
    // 8-bit low registers
    Reg8Start,
    Al,
    Cl,
    Dl,
    Bl,
    Sil,
    Dil,
    Spl,
    Bpl,
    R8b,
    R9b,
    R10b,
    R11b,
    R12b,
    R13b,
    R14b,
    R15b,
    Reg8End,
    // 8-bit high registers
    Reg8hStart,
    Ah,
    Ch,
    Dh,
    Bh,
    Sih,
    Dih,
    Sph,
    Bph,
    R8h,
    R9h,
    R10h,
    R11h,
    R12h,
    R13h,
    R14h,
    R15h,
    Reg8hEnd,
    // Control registers
    Cr4,
    Cr3,
}

impl Reg {
    /// Check if this is a 64-bit register
    pub fn is_reg64(self) -> bool {
        (self as u8) > (Reg::Reg64Start as u8) && (self as u8) < (Reg::Reg64End as u8)
    }

    /// Convert any register to its 64-bit equivalent
    pub fn to_reg64(self) -> Option<Reg> {
        let mut r = self as u8;
        while r > Reg::Reg64End as u8 {
            r = r.saturating_sub(19);
        }
        if r <= Reg::Reg64Start as u8 {
            return None;
        }
        // Safety: we know r is valid because we checked bounds
        Some(unsafe { std::mem::transmute(r) })
    }

    /// Check if this is a high 8-bit register
    pub fn is_reg8h(self) -> bool {
        (self as u8) > (Reg::Reg8hStart as u8) && (self as u8) < (Reg::Reg8hEnd as u8)
    }
}

/// Value types stored in memory
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
pub enum ValueType {
    CallValue,
    MemValue,
    CallRegValue,
    ImmValue,
    #[default]
    OtherValue,
}

/// A value stored in memory
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Value {
    pub value_type: ValueType,
    pub value: i64,
}

impl Value {
    pub fn new(value_type: ValueType, value: i64) -> Self {
        Self { value_type, value }
    }
}

impl Default for Value {
    fn default() -> Self {
        Self {
            value_type: ValueType::OtherValue,
            value: 0,
        }
    }
}

impl fmt::Display for Value {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self.value_type {
            ValueType::CallValue => {
                if self.value == -1 {
                    write!(f, "(CALL_VALUE,Target RIP)")
                } else {
                    write!(f, "(CALL_VALUE,0x{:x}(inst))", self.value)
                }
            }
            ValueType::MemValue => {
                write!(f, "(MEM_VALUE,0x{:x}(memid))", self.value)
            }
            ValueType::CallRegValue => {
                if self.value == -1 {
                    write!(f, "(CALL_REG_VALUE,Target RIP)")
                } else if (self.value >> 32) != 0 {
                    write!(f, "(CALL_REG_VALUE,0x{:x}(memid))", self.value >> 32)
                } else {
                    write!(f, "(CALL_REG_VALUE,0x{:x}(inst))", self.value)
                }
            }
            ValueType::ImmValue | ValueType::OtherValue => {
                write!(f, "(OTHER_VALUE,0x{:x})", self.value)
            }
        }
    }
}

/// Shared memory reference
pub type MemoryRef = Rc<RefCell<Memory>>;

/// Memory buffer that registers can point to
#[derive(Debug, Clone)]
pub struct Memory {
    pub ref_count: u32,
    pub mem_id: u32,
    pub range: Vec<(i64, i64)>,
    pub content: BTreeMap<i64, Value>,
    pub input_src: String,
    pub input_offset: i64,
    pub input_action: bool,
}

impl Memory {
    pub fn new() -> Self {
        Self {
            ref_count: 0,
            mem_id: next_mem_id(),
            range: vec![(-MEM_INF, MEM_INF)],
            content: BTreeMap::new(),
            input_src: String::new(),
            input_offset: 0,
            input_action: false,
        }
    }

    pub fn increase_ref_count(&mut self) {
        self.ref_count += 1;
    }

    pub fn decrease_ref_count(&mut self) {
        self.ref_count = self.ref_count.saturating_sub(1);
    }

    pub fn set_content(&mut self, offset: i64, val: Value, _len: OperationLength) {
        self.content.insert(offset, val);
    }

    pub fn contain_range(&self, range: &(i64, i64)) -> bool {
        for r in &self.range {
            if r.1 >= range.1 {
                return r.0 <= range.0;
            }
        }
        false
    }

    pub fn remove_range(&mut self, range: &(i64, i64)) -> bool {
        for i in 0..self.range.len() {
            if self.range[i].1 >= range.1 {
                if self.range[i].0 > range.0 {
                    return false;
                }
                let saved = self.range[i].0;
                self.range[i].0 = range.1;
                self.range.insert(i, (saved, range.0));
                return true;
            }
        }
        false
    }

    pub fn get_input_relation(&self) -> String {
        if self.input_offset == 0 {
            format!(
                "{}{}",
                if self.input_action { "*" } else { "" },
                self.input_src
            )
        } else {
            let sign = if self.input_offset < 0 { "-" } else { "+" };
            let abs_offset = self.input_offset.unsigned_abs();
            if self.input_action {
                format!("*({}{}0x{:x})", self.input_src, sign, abs_offset)
            } else {
                format!("{}{}0x{:x}", self.input_src, sign, abs_offset)
            }
        }
    }

    pub fn set_input_relation(&mut self, src: &str, offset: i64, action: bool) {
        self.input_src = src.to_string();
        self.input_offset = offset;
        self.input_action = action;
    }
}

impl Default for Memory {
    fn default() -> Self {
        Self::new()
    }
}

impl fmt::Display for Memory {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "memid:0x{:x}, relation: {}", self.mem_id, self.get_input_relation())?;
        
        // Build available range string
        write!(f, "\n\tAvailable:[ ")?;
        let mut first_range = true;
        for each in &self.range {
            if each.0 == each.1 {
                continue;
            }
            if !first_range {
                write!(f, ", ")?;
            }
            first_range = false;
            
            let left = if each.0 == -MEM_INF {
                "-INF".to_string()
            } else {
                format!(
                    "{}0x{:x}",
                    if each.0 < 0 { "-" } else { "" },
                    each.0.unsigned_abs()
                )
            };
            let right = if each.1 == MEM_INF {
                "INF".to_string()
            } else {
                format!(
                    "{}0x{:x}",
                    if each.1 < 0 { "-" } else { "" },
                    each.1.unsigned_abs()
                )
            };
            write!(f, "[{},{}]", left, right)?;
        }
        write!(f, " ]")?;

        // Build content string if not empty
        if !self.content.is_empty() {
            write!(f, "\n\tcontent:[ ")?;
            let mut first_content = true;
            for (key, val) in &self.content {
                if !first_content {
                    write!(f, ", ")?;
                }
                first_content = false;
                let key_str = format!(
                    "{}0x{:x}",
                    if *key < 0 { "-" } else { "" },
                    key.unsigned_abs()
                );
                write!(f, "[{}:{}]", key_str, val)?;
            }
            write!(f, " ]")?;
        }
        
        Ok(())
    }
}

/// A register with its memory state
#[derive(Debug, Clone)]
pub struct Register {
    pub name: Reg,
    pub mem: MemoryRef,
    pub base_offset: i64,
    pub input_src: String,
    pub input_offset: i64,
    pub input_action: bool,
}

impl Register {
    pub fn new(alloc_mem: bool) -> Self {
        let mem = if alloc_mem {
            let m = Rc::new(RefCell::new(Memory::new()));
            m.borrow_mut().ref_count = 1;
            m
        } else {
            Rc::new(RefCell::new(Memory {
                ref_count: 0,
                mem_id: 0,
                range: Vec::new(),
                content: BTreeMap::new(),
                input_src: String::new(),
                input_offset: 0,
                input_action: false,
            }))
        };

        Self {
            name: Reg::None,
            mem,
            base_offset: 0,
            input_src: String::new(),
            input_offset: 0,
            input_action: false,
        }
    }

    pub fn from_register(reg: &Register) -> Self {
        let new_mem = Rc::new(RefCell::new(Memory {
            ref_count: reg.mem.borrow().ref_count,
            mem_id: reg.mem.borrow().mem_id,
            range: reg.mem.borrow().range.clone(),
            content: reg.mem.borrow().content.clone(),
            input_src: reg.mem.borrow().input_src.clone(),
            input_offset: reg.mem.borrow().input_offset,
            input_action: reg.mem.borrow().input_action,
        }));

        Self {
            name: reg.name,
            mem: new_mem,
            base_offset: reg.base_offset,
            input_src: reg.input_src.clone(),
            input_offset: reg.input_offset,
            input_action: reg.input_action,
        }
    }

    pub fn alias(&mut self, reg: &Register, copy_mem: bool) {
        if copy_mem {
            self.mem = Rc::clone(&reg.mem);
        }
        self.base_offset = reg.base_offset;
        self.input_src = reg.input_src.clone();
        self.input_offset = reg.input_offset;
        self.input_action = reg.input_action;
        self.mem.borrow_mut().increase_ref_count();
    }

    pub fn contain_range(&self, range: &(i64, i64)) -> bool {
        let adjusted = (range.0 + self.base_offset, range.1 + self.base_offset);
        self.mem.borrow().contain_range(&adjusted)
    }

    pub fn remove_range(&mut self, range: &(i64, i64)) -> bool {
        let adjusted = (range.0 + self.base_offset, range.1 + self.base_offset);
        self.mem.borrow_mut().remove_range(&adjusted)
    }

    pub fn set_content(&mut self, offset: i64, val: Value, len: OperationLength) {
        self.mem
            .borrow_mut()
            .set_content(offset + self.base_offset, val, len);
    }

    pub fn get_input_relation(&self) -> String {
        if self.input_offset == 0 {
            format!(
                "{}{}",
                if self.input_action { "*" } else { "" },
                self.input_src
            )
        } else {
            let sign = if self.input_offset < 0 { "-" } else { "+" };
            let abs_offset = self.input_offset.unsigned_abs();
            if self.input_action {
                format!("*({}{}0x{:x})", self.input_src, sign, abs_offset)
            } else {
                format!("{}{}0x{:x}", self.input_src, sign, abs_offset)
            }
        }
    }

    pub fn set_input_relation(&mut self, src_relation: &str, offset: i64, action: bool) {
        self.input_src = src_relation.to_string();
        self.input_action = action;
        self.input_offset = offset;
        let mut mem = self.mem.borrow_mut();
        mem.input_src = src_relation.to_string();
        mem.input_offset = offset;
        mem.input_action = action;
    }
}

impl fmt::Display for Register {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(
            f,
            "{}:\t{}",
            crate::asmutils::get_reg_str_by_reg(self.name),
            self.mem.borrow()
        )
    }
}

/// Extension trait for Vec<Register> to provide idiomatic register list operations
pub trait RegListExt {
    /// Check if a register is in the list
    fn contains_reg(&self, reg: Reg) -> bool;
    
    /// Get the index of a register by name
    fn position_of(&self, reg: Reg) -> Option<usize>;
    
    /// Get a reference to a register by name
    fn get_by_name(&self, reg: Reg) -> Option<&Register>;
    
    /// Get a mutable reference to a register by name
    fn get_by_name_mut(&mut self, reg: Reg) -> Option<&mut Register>;
    
    /// Remove a register by name
    fn remove_by_name(&mut self, reg: Reg);
}

impl RegListExt for Vec<Register> {
    fn contains_reg(&self, reg: Reg) -> bool {
        self.iter().any(|r| r.name == reg)
    }
    
    fn position_of(&self, reg: Reg) -> Option<usize> {
        self.iter().position(|r| r.name == reg)
    }
    
    fn get_by_name(&self, reg: Reg) -> Option<&Register> {
        self.iter().find(|r| r.name == reg)
    }
    
    fn get_by_name_mut(&mut self, reg: Reg) -> Option<&mut Register> {
        self.iter_mut().find(|r| r.name == reg)
    }
    
    fn remove_by_name(&mut self, reg: Reg) {
        self.retain(|r| r.name != reg);
    }
}

impl RegListExt for [Register] {
    fn contains_reg(&self, reg: Reg) -> bool {
        self.iter().any(|r| r.name == reg)
    }
    
    fn position_of(&self, reg: Reg) -> Option<usize> {
        self.iter().position(|r| r.name == reg)
    }
    
    fn get_by_name(&self, reg: Reg) -> Option<&Register> {
        self.iter().find(|r| r.name == reg)
    }
    
    fn get_by_name_mut(&mut self, reg: Reg) -> Option<&mut Register> {
        self.iter_mut().find(|r| r.name == reg)
    }
    
    fn remove_by_name(&mut self, _reg: Reg) {
        // Note: Cannot remove from slice, only Vec
        panic!("Cannot remove from slice, use Vec<Register> instead")
    }
}

/// An operand in an instruction
#[derive(Debug, Clone)]
pub struct Operand {
    pub is_dereference: bool,
    pub contain_seg_reg: bool,
    pub reg_list: Vec<(Reg, i32)>,
    pub literal_sym: bool, // false for +, true for -
    pub literal_num: u64,
    pub reg_num: usize,
    pub operation_length: OperationLength,
    pub imm: i64,
}

impl Operand {
    pub fn new(
        is_dereference: bool,
        contain_seg_reg: bool,
        reg_list: Vec<(Reg, i32)>,
        literal_sym: bool,
        literal_num: u64,
        operation_length: OperationLength,
    ) -> Self {
        let reg_num = reg_list.len();
        Self {
            is_dereference,
            contain_seg_reg,
            reg_list,
            literal_sym,
            literal_num,
            reg_num,
            operation_length,
            imm: 0,
        }
    }

    pub fn is_literal_number(&self) -> bool {
        self.reg_num == 0 && !self.is_dereference && self.literal_num != 0
    }

    pub fn is_reg_operation(&self) -> bool {
        self.reg_num > 0 && !self.is_dereference
    }

    pub fn contain_reg(&self, reg: Reg) -> bool {
        self.reg_list.iter().any(|(r, _)| *r == reg)
    }

    pub fn is_reg64_operation(&self) -> bool {
        !self.reg_list.is_empty() && self.operation_length == OperationLength::QWord
    }

    pub fn is_memory_access(&self) -> bool {
        self.is_dereference
    }

    pub fn contain_segment_reg(&self) -> bool {
        self.contain_seg_reg
    }

    pub fn get_reg_op(&self) -> Reg {
        debug_assert!(self.reg_num == 1);
        self.reg_list[0].0
    }

    /// Returns (REG, (start, end)) range for memory access
    pub fn get_used_range(&self) -> (Reg, (i64, i64)) {
        if self.reg_num != 1 || !self.is_dereference || self.reg_list[0].1 != 1 {
            return (Reg::None, (0, 0));
        }
        let reg = self.reg_list[0].0;
        let start = self.imm;
        let end = start + (self.operation_length as i64);
        (reg, (start, end))
    }

    fn transfer_operation_len_to_str(&self) -> String {
        match self.operation_length {
            OperationLength::None => "0".to_string(),
            OperationLength::Byte => "1".to_string(),
            OperationLength::Word => "2".to_string(),
            OperationLength::DWord => "4".to_string(),
            OperationLength::QWord => "8".to_string(),
        }
    }
}

impl fmt::Display for Operand {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, " ")?;

        if self.is_dereference {
            write!(f, "{}", self.transfer_operation_len_to_str())?;
            write!(f, "[")?;
        }

        let mut first = true;
        for (reg, num) in &self.reg_list {
            let reg_str = crate::asmutils::get_reg_str_by_reg(*reg);
            if *num > 1 {
                if first {
                    write!(f, "{}*0x{:x}", reg_str, num)?;
                    first = false;
                } else {
                    write!(f, "+{}*0x{:x}", reg_str, num)?;
                }
            } else if *num == 1 {
                if first {
                    write!(f, "{}", reg_str)?;
                    first = false;
                } else {
                    write!(f, "+{}", reg_str)?;
                }
            } else if *num < 0 {
                write!(f, "-{}*0x{:x}", reg_str, (-num) as u32)?;
            }
        }

        if !self.literal_sym {
            if first {
                write!(f, "0x{:x}", self.literal_num)?;
            } else {
                write!(f, "+0x{:x}", self.literal_num)?;
            }
        } else if self.literal_num != 0 {
            write!(f, "-0x{:x}", self.literal_num)?;
        }

        if self.is_dereference {
            write!(f, "]")?;
        }

        Ok(())
    }
}

/// A single instruction
#[derive(Debug, Clone)]
pub struct Instruction {
    pub offset: u64,
    pub opcode: Opcode,
    pub op_src: Option<Operand>,
    pub op_dst: Option<Operand>,
    pub operand_num: u8,
    pub operation_length: OperationLength,
    pub original_inst: String,
}

impl Instruction {
    pub fn new(offset: u64, opcode: Opcode) -> Self {
        Self {
            offset,
            opcode,
            op_src: None,
            op_dst: None,
            operand_num: 0,
            operation_length: OperationLength::QWord,
            original_inst: String::new(),
        }
    }

    pub fn is_reg_operation(&self) -> bool {
        self.op_src
            .as_ref()
            .map(|o| o.is_reg_operation())
            .unwrap_or(false)
            || self
                .op_dst
                .as_ref()
                .map(|o| o.is_reg_operation())
                .unwrap_or(false)
    }

    pub fn format_with_offset(&self, display_offset: bool) -> String {
        let mut operand_str = String::new();
        if let Some(ref dst) = self.op_dst {
            operand_str.push_str(&dst.to_string());
            operand_str.push(',');
        }
        if let Some(ref src) = self.op_src {
            operand_str.push_str(&src.to_string());
            operand_str.push(',');
        }
        if operand_str.ends_with(',') {
            operand_str.pop();
        }

        let opcode_str = crate::asmutils::transfer_op_to_str(self.opcode);
        if display_offset {
            format!("0x{:x}:\t\t{} {}\n", self.offset, opcode_str, operand_str)
        } else {
            format!("{} {}\n", opcode_str, operand_str)
        }
    }
}

impl fmt::Display for Instruction {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let opcode_str = crate::asmutils::transfer_op_to_str(self.opcode);
        if let Some(ref dst) = self.op_dst {
            write!(f, "{} {}", opcode_str, dst)?;
            if let Some(ref src) = self.op_src {
                write!(f, ",{}", src)?;
            }
        } else {
            write!(f, "{}", opcode_str)?;
        }
        writeln!(f)
    }
}

/// Instruction pointer type
pub type InstrPtr = Arc<Instruction>;

/// A segment of instructions ending in call/jmp
#[derive(Debug, Clone)]
pub struct Segment {
    pub inst_list: Vec<InstrPtr>,
    pub useful_inst_index: usize,
}

impl Segment {
    pub fn new(inst_list: Vec<InstrPtr>) -> Self {
        Self {
            inst_list,
            useful_inst_index: 0,
        }
    }

    pub fn format_with_offset(&self, display_offset: bool) -> String {
        let mut res = String::new();
        for inst in &self.inst_list {
            res.push_str(&inst.format_with_offset(display_offset));
        }
        res
    }

    pub fn print_inst(&self) {
        for idx in self.useful_inst_index..self.inst_list.len() {
            println!("{}", self.inst_list[idx].original_inst);
        }
    }
}

impl fmt::Display for Segment {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        for inst in &self.inst_list {
            write!(f, "{}", inst)?;
        }
        Ok(())
    }
}

impl PartialEq for Segment {
    fn eq(&self, other: &Self) -> bool {
        self.format_with_offset(false) == other.format_with_offset(false)
    }
}

/// Segment pointer type
pub type SegmentPtr = Arc<Segment>;

/// A solution containing found gadget chain
#[derive(Debug, Clone, Default)]
pub struct Solution {
    pub found: bool,
    pub output_reg_list: Vec<Register>,
    pub output_segments: Vec<(SegmentPtr, usize)>,
    pub minimized_reg_list: Vec<Register>,
}

#[cfg(test)]
mod tests {
    use super::*;

    // ===== OperationLength tests =====
    #[test]
    fn test_operation_length_default() {
        assert_eq!(OperationLength::default(), OperationLength::None);
    }

    #[test]
    fn test_operation_length_ordering() {
        assert!(OperationLength::Byte < OperationLength::Word);
        assert!(OperationLength::Word < OperationLength::DWord);
        assert!(OperationLength::DWord < OperationLength::QWord);
    }

    // ===== Reg tests =====
    #[test]
    fn test_reg_is_reg64() {
        assert!(Reg::Rax.is_reg64());
        assert!(Reg::R15.is_reg64());
        assert!(Reg::Rip.is_reg64());
        assert!(!Reg::Eax.is_reg64());
        assert!(!Reg::Al.is_reg64());
        assert!(!Reg::None.is_reg64());
        assert!(!Reg::Reg64Start.is_reg64());
        assert!(!Reg::Reg64End.is_reg64());
    }

    #[test]
    fn test_reg_to_reg64() {
        // 64-bit stays 64-bit
        assert_eq!(Reg::Rax.to_reg64(), Some(Reg::Rax));
        assert_eq!(Reg::R15.to_reg64(), Some(Reg::R15));
        // 32-bit converts to 64-bit
        assert_eq!(Reg::Eax.to_reg64(), Some(Reg::Rax));
        assert_eq!(Reg::R15d.to_reg64(), Some(Reg::R15));
        // 16-bit converts
        assert_eq!(Reg::Ax.to_reg64(), Some(Reg::Rax));
        // 8-bit converts
        assert_eq!(Reg::Al.to_reg64(), Some(Reg::Rax));
        // None doesn't convert
        assert_eq!(Reg::None.to_reg64(), None);
    }

    #[test]
    fn test_reg_is_reg8h() {
        assert!(Reg::Ah.is_reg8h());
        assert!(Reg::Bh.is_reg8h());
        assert!(!Reg::Al.is_reg8h());
        assert!(!Reg::Rax.is_reg8h());
        assert!(!Reg::None.is_reg8h());
    }

    // ===== Value tests =====
    #[test]
    fn test_value_new() {
        let v = Value::new(ValueType::CallValue, 0x1234);
        assert_eq!(v.value_type, ValueType::CallValue);
        assert_eq!(v.value, 0x1234);
    }

    #[test]
    fn test_value_default() {
        let v = Value::default();
        assert_eq!(v.value_type, ValueType::OtherValue);
        assert_eq!(v.value, 0);
    }

    #[test]
    fn test_value_to_string_call_value() {
        // Target RIP case
        let v1 = Value::new(ValueType::CallValue, -1);
        assert!(v1.to_string().contains("Target RIP"));
        
        // Normal instruction case
        let v2 = Value::new(ValueType::CallValue, 0x14ace2);
        assert!(v2.to_string().contains("CALL_VALUE"));
        assert!(v2.to_string().contains("14ace2"));
    }

    #[test]
    fn test_value_to_string_mem_value() {
        let v = Value::new(ValueType::MemValue, 0x123);
        assert!(v.to_string().contains("MEM_VALUE"));
        assert!(v.to_string().contains("0x123"));
    }

    #[test]
    fn test_value_to_string_call_reg_value() {
        // Target RIP case
        let v1 = Value::new(ValueType::CallRegValue, -1);
        assert!(v1.to_string().contains("Target RIP"));
        
        // memid in upper 32 bits
        let v2 = Value::new(ValueType::CallRegValue, 0x100000000);
        assert!(v2.to_string().contains("CALL_REG_VALUE"));
        assert!(v2.to_string().contains("memid"));
        
        // inst offset in lower 32 bits
        let v3 = Value::new(ValueType::CallRegValue, 0x1234);
        assert!(v3.to_string().contains("inst"));
    }

    #[test]
    fn test_value_to_string_other() {
        let v1 = Value::new(ValueType::ImmValue, 0x42);
        assert!(v1.to_string().contains("OTHER_VALUE"));
        
        let v2 = Value::new(ValueType::OtherValue, 0x42);
        assert!(v2.to_string().contains("OTHER_VALUE"));
    }

    // ===== Memory tests =====
    #[test]
    fn test_memory_new() {
        let mem = Memory::new();
        assert_eq!(mem.ref_count, 0);
        assert_eq!(mem.range.len(), 1);
        assert_eq!(mem.range[0], (-MEM_INF, MEM_INF));
        assert!(mem.content.is_empty());
    }

    #[test]
    fn test_memory_default() {
        let mem = Memory::default();
        assert_eq!(mem.range.len(), 1);
    }

    #[test]
    fn test_memory_ref_count() {
        let mut mem = Memory::new();
        assert_eq!(mem.ref_count, 0);
        mem.increase_ref_count();
        assert_eq!(mem.ref_count, 1);
        mem.increase_ref_count();
        assert_eq!(mem.ref_count, 2);
        mem.decrease_ref_count();
        assert_eq!(mem.ref_count, 1);
        mem.decrease_ref_count();
        assert_eq!(mem.ref_count, 0);
        mem.decrease_ref_count(); // Should saturate at 0
        assert_eq!(mem.ref_count, 0);
    }

    #[test]
    fn test_memory_set_content() {
        let mut mem = Memory::new();
        mem.set_content(0x10, Value::new(ValueType::CallValue, 0x1234), OperationLength::QWord);
        assert!(mem.content.contains_key(&0x10));
        assert_eq!(mem.content.get(&0x10).unwrap().value, 0x1234);
    }

    #[test]
    fn test_memory_contain_range() {
        let mem = Memory::new();
        assert!(mem.contain_range(&(0, 8)));
        assert!(mem.contain_range(&(-100, 100)));
        assert!(mem.contain_range(&(-MEM_INF + 1, MEM_INF - 1)));
    }

    #[test]
    fn test_memory_contain_range_after_removal() {
        let mut mem = Memory::new();
        mem.remove_range(&(0, 8));
        assert!(!mem.contain_range(&(0, 8)));
        assert!(!mem.contain_range(&(2, 6)));
        assert!(mem.contain_range(&(8, 16)));
        assert!(mem.contain_range(&(-16, -8)));
    }

    #[test]
    fn test_memory_remove_range() {
        let mut mem = Memory::new();
        assert!(mem.remove_range(&(0, 8)));
        assert_eq!(mem.range.len(), 2);
        
        // Try to remove a range that doesn't exist (start > first available)
        let mut mem2 = Memory::new();
        mem2.remove_range(&(0, 100));
        assert!(!mem2.remove_range(&(50, 60))); // Range 50-60 was removed already
    }

    #[test]
    fn test_memory_get_input_relation() {
        let mut mem = Memory::new();
        
        // No offset, no action
        mem.input_src = "rdi".to_string();
        mem.input_offset = 0;
        mem.input_action = false;
        assert_eq!(mem.get_input_relation(), "rdi");
        
        // With action (dereference)
        mem.input_action = true;
        assert_eq!(mem.get_input_relation(), "*rdi");
        
        // With positive offset
        mem.input_action = false;
        mem.input_offset = 0x10;
        assert!(mem.get_input_relation().contains("+0x10"));
        
        // With negative offset
        mem.input_offset = -0x10;
        assert!(mem.get_input_relation().contains("-0x10"));
        
        // With action and offset
        mem.input_action = true;
        mem.input_offset = 0x20;
        let rel = mem.get_input_relation();
        assert!(rel.contains("*"));
        assert!(rel.contains("+0x20"));
    }

    #[test]
    fn test_memory_set_input_relation() {
        let mut mem = Memory::new();
        mem.set_input_relation("rax", 0x10, true);
        assert_eq!(mem.input_src, "rax");
        assert_eq!(mem.input_offset, 0x10);
        assert!(mem.input_action);
    }

    #[test]
    fn test_memory_to_string() {
        let mut mem = Memory::new();
        mem.input_src = "rdi".to_string();
        let s = mem.to_string();
        assert!(s.contains("memid"));
        assert!(s.contains("relation"));
        assert!(s.contains("Available"));
    }

    #[test]
    fn test_memory_to_string_with_content() {
        let mut mem = Memory::new();
        mem.input_src = "rdi".to_string();
        mem.set_content(0x8, Value::new(ValueType::MemValue, 0x123), OperationLength::QWord);
        let s = mem.to_string();
        assert!(s.contains("content"));
    }

    #[test]
    fn test_memory_to_string_negative_range() {
        let mut mem = Memory::new();
        mem.input_src = "rdi".to_string();
        mem.range = vec![(-100, -50), (50, 100)];
        let s = mem.to_string();
        assert!(s.contains("-0x64")); // -100
        assert!(s.contains("-0x32")); // -50
    }

    // ===== Register tests =====
    #[test]
    fn test_register_new_with_alloc() {
        let reg = Register::new(true);
        assert_eq!(reg.name, Reg::None);
        assert_eq!(reg.base_offset, 0);
        assert_eq!(reg.mem.borrow().ref_count, 1);
        assert_eq!(reg.mem.borrow().range.len(), 1);
    }

    #[test]
    fn test_register_new_without_alloc() {
        let reg = Register::new(false);
        assert_eq!(reg.mem.borrow().ref_count, 0);
        assert_eq!(reg.mem.borrow().range.len(), 0);
    }

    #[test]
    fn test_register_from_register() {
        let mut reg1 = Register::new(true);
        reg1.name = Reg::Rax;
        reg1.base_offset = 0x10;
        reg1.input_src = "test".to_string();
        
        let reg2 = Register::from_register(&reg1);
        assert_eq!(reg2.name, Reg::Rax);
        assert_eq!(reg2.base_offset, 0x10);
        assert_eq!(reg2.input_src, "test");
        // ref_count starts at 1 for both (independent allocations)
        assert_eq!(reg1.mem.borrow().ref_count, 1);
        assert_eq!(reg2.mem.borrow().ref_count, 1);
        // Memory is not shared - modifying one doesn't affect the other
        reg1.mem.borrow_mut().ref_count = 5;
        assert_eq!(reg2.mem.borrow().ref_count, 1);
    }

    #[test]
    fn test_register_alias() {
        let mut reg1 = Register::new(true);
        reg1.name = Reg::Rax;
        reg1.input_src = "src".to_string();
        reg1.base_offset = 0x10;
        
        let mut reg2 = Register::new(false);
        reg2.alias(&reg1, true);
        
        assert_eq!(reg2.base_offset, 0x10);
        assert_eq!(reg2.input_src, "src");
        // Memory should be shared
        assert_eq!(reg1.mem.borrow().mem_id, reg2.mem.borrow().mem_id);
        assert_eq!(reg1.mem.borrow().ref_count, 2);
    }

    #[test]
    fn test_register_alias_without_copy_mem() {
        let mut reg1 = Register::new(true);
        reg1.name = Reg::Rax;
        
        let mut reg2 = Register::new(true); // Has its own memory
        let orig_mem_id = reg2.mem.borrow().mem_id;
        reg2.alias(&reg1, false);
        
        // Memory should NOT be shared
        assert_eq!(reg2.mem.borrow().mem_id, orig_mem_id);
    }

    #[test]
    fn test_register_contain_range() {
        let mut reg = Register::new(true);
        reg.base_offset = 0x100;
        
        // Range adjusted by base_offset
        assert!(reg.contain_range(&(-0x100, -0x92))); // Actual: 0, 8
    }

    #[test]
    fn test_register_remove_range() {
        let mut reg = Register::new(true);
        reg.base_offset = 0x10;
        
        assert!(reg.remove_range(&(0, 8))); // Actual: 0x10, 0x18
        assert!(!reg.contain_range(&(0, 8)));
    }

    #[test]
    fn test_register_set_content() {
        let mut reg = Register::new(true);
        reg.base_offset = 0x10;
        reg.set_content(0x8, Value::new(ValueType::CallValue, 0x42), OperationLength::QWord);
        
        // Content stored at offset + base_offset = 0x18
        assert!(reg.mem.borrow().content.contains_key(&0x18));
    }

    #[test]
    fn test_register_get_input_relation() {
        let mut reg = Register::new(true);
        
        // No offset
        reg.input_src = "rdi".to_string();
        reg.input_offset = 0;
        reg.input_action = false;
        assert_eq!(reg.get_input_relation(), "rdi");
        
        // With action
        reg.input_action = true;
        assert_eq!(reg.get_input_relation(), "*rdi");
        
        // With offset
        reg.input_action = false;
        reg.input_offset = 0x20;
        assert!(reg.get_input_relation().contains("+0x20"));
        
        // Negative offset
        reg.input_offset = -0x20;
        assert!(reg.get_input_relation().contains("-0x20"));
    }

    #[test]
    fn test_register_set_input_relation() {
        let mut reg = Register::new(true);
        reg.set_input_relation("*(rdi+0x8)", 0x10, true);
        
        assert_eq!(reg.input_src, "*(rdi+0x8)");
        assert_eq!(reg.input_offset, 0x10);
        assert!(reg.input_action);
        
        // Memory should also be updated
        assert_eq!(reg.mem.borrow().input_src, "*(rdi+0x8)");
    }

    #[test]
    fn test_register_to_string() {
        let mut reg = Register::new(true);
        reg.name = Reg::Rdi;
        reg.input_src = "test".to_string();
        let s = reg.to_string();
        assert!(s.contains("rdi"));
    }

    // ===== Operand tests =====
    #[test]
    fn test_operand_new() {
        let op = Operand::new(
            true,
            false,
            vec![(Reg::Rax, 1), (Reg::Rbx, 2)],
            false,
            0x10,
            OperationLength::QWord,
        );
        assert!(op.is_dereference);
        assert!(!op.contain_seg_reg);
        assert_eq!(op.reg_num, 2);
        assert_eq!(op.literal_num, 0x10);
    }

    #[test]
    fn test_operand_is_literal_number() {
        let op1 = Operand::new(false, false, vec![], false, 0x42, OperationLength::QWord);
        assert!(op1.is_literal_number());
        
        let op2 = Operand::new(false, false, vec![(Reg::Rax, 1)], false, 0x42, OperationLength::QWord);
        assert!(!op2.is_literal_number()); // Has register
        
        let op3 = Operand::new(true, false, vec![], false, 0x42, OperationLength::QWord);
        assert!(!op3.is_literal_number()); // Is dereference
        
        let op4 = Operand::new(false, false, vec![], false, 0, OperationLength::QWord);
        assert!(!op4.is_literal_number()); // Zero literal
    }

    #[test]
    fn test_operand_is_reg_operation() {
        let op1 = Operand::new(false, false, vec![(Reg::Rax, 1)], false, 0, OperationLength::QWord);
        assert!(op1.is_reg_operation());
        
        let op2 = Operand::new(true, false, vec![(Reg::Rax, 1)], false, 0, OperationLength::QWord);
        assert!(!op2.is_reg_operation()); // Is dereference
        
        let op3 = Operand::new(false, false, vec![], false, 0x42, OperationLength::QWord);
        assert!(!op3.is_reg_operation()); // No registers
    }

    #[test]
    fn test_operand_contain_reg() {
        let op = Operand::new(false, false, vec![(Reg::Rax, 1), (Reg::Rbx, 2)], false, 0, OperationLength::QWord);
        assert!(op.contain_reg(Reg::Rax));
        assert!(op.contain_reg(Reg::Rbx));
        assert!(!op.contain_reg(Reg::Rcx));
    }

    #[test]
    fn test_operand_is_reg64_operation() {
        let op1 = Operand::new(false, false, vec![(Reg::Rax, 1)], false, 0, OperationLength::QWord);
        assert!(op1.is_reg64_operation());
        
        let op2 = Operand::new(false, false, vec![(Reg::Rax, 1)], false, 0, OperationLength::DWord);
        assert!(!op2.is_reg64_operation());
        
        let op3 = Operand::new(false, false, vec![], false, 0, OperationLength::QWord);
        assert!(!op3.is_reg64_operation()); // No registers
    }

    #[test]
    fn test_operand_is_memory_access() {
        let op1 = Operand::new(true, false, vec![(Reg::Rax, 1)], false, 0, OperationLength::QWord);
        assert!(op1.is_memory_access());
        
        let op2 = Operand::new(false, false, vec![(Reg::Rax, 1)], false, 0, OperationLength::QWord);
        assert!(!op2.is_memory_access());
    }

    #[test]
    fn test_operand_contain_segment_reg() {
        let op1 = Operand::new(false, true, vec![], false, 0, OperationLength::QWord);
        assert!(op1.contain_segment_reg());
        
        let op2 = Operand::new(false, false, vec![], false, 0, OperationLength::QWord);
        assert!(!op2.contain_segment_reg());
    }

    #[test]
    fn test_operand_get_reg_op() {
        let op = Operand::new(false, false, vec![(Reg::Rax, 1)], false, 0, OperationLength::QWord);
        assert_eq!(op.get_reg_op(), Reg::Rax);
    }

    #[test]
    fn test_operand_get_used_range() {
        // Valid memory access
        let mut op = Operand::new(true, false, vec![(Reg::Rax, 1)], false, 0, OperationLength::QWord);
        op.imm = 0x10;
        let (reg, range) = op.get_used_range();
        assert_eq!(reg, Reg::Rax);
        assert_eq!(range, (0x10, 0x18));
        
        // Not a dereference
        let op2 = Operand::new(false, false, vec![(Reg::Rax, 1)], false, 0, OperationLength::QWord);
        let (reg2, _) = op2.get_used_range();
        assert_eq!(reg2, Reg::None);
        
        // Multiple registers
        let op3 = Operand::new(true, false, vec![(Reg::Rax, 1), (Reg::Rbx, 1)], false, 0, OperationLength::QWord);
        let (reg3, _) = op3.get_used_range();
        assert_eq!(reg3, Reg::None);
        
        // Coefficient != 1
        let op4 = Operand::new(true, false, vec![(Reg::Rax, 2)], false, 0, OperationLength::QWord);
        let (reg4, _) = op4.get_used_range();
        assert_eq!(reg4, Reg::None);
    }

    #[test]
    fn test_operand_to_string() {
        // Register only
        let op1 = Operand::new(false, false, vec![(Reg::Rax, 1)], false, 0, OperationLength::QWord);
        assert!(op1.to_string().contains("rax"));
        
        // Register with multiplier
        let op2 = Operand::new(false, false, vec![(Reg::Rax, 4)], false, 0, OperationLength::QWord);
        assert!(op2.to_string().contains("rax*0x4"));
        
        // Memory access
        let op3 = Operand::new(true, false, vec![(Reg::Rax, 1)], false, 0x10, OperationLength::QWord);
        assert!(op3.to_string().contains("["));
        assert!(op3.to_string().contains("]"));
        assert!(op3.to_string().contains("0x10"));
        
        // Multiple registers
        let op4 = Operand::new(false, false, vec![(Reg::Rax, 1), (Reg::Rbx, 1)], false, 0, OperationLength::QWord);
        let s4 = op4.to_string();
        assert!(s4.contains("rax"));
        assert!(s4.contains("rbx"));
        
        // Negative multiplier
        let op5 = Operand::new(false, false, vec![(Reg::Rax, -1)], false, 0, OperationLength::QWord);
        assert!(op5.to_string().contains("-"));
        
        // Literal only (positive)
        let op6 = Operand::new(false, false, vec![], false, 0x42, OperationLength::QWord);
        assert!(op6.to_string().contains("0x42"));
        
        // Literal with negative sym
        let mut op7 = Operand::new(false, false, vec![], true, 0x42, OperationLength::QWord);
        op7.literal_num = 0x42;
        assert!(op7.to_string().contains("-0x42"));
    }

    // ===== Instruction tests =====
    #[test]
    fn test_instruction_new() {
        let inst = Instruction::new(0x1234, Opcode::Mov);
        assert_eq!(inst.offset, 0x1234);
        assert_eq!(inst.opcode, Opcode::Mov);
        assert_eq!(inst.operand_num, 0);
        assert_eq!(inst.operation_length, OperationLength::QWord);
    }

    #[test]
    fn test_instruction_is_reg_operation() {
        let mut inst = Instruction::new(0, Opcode::Mov);
        assert!(!inst.is_reg_operation());
        
        inst.op_dst = Some(Operand::new(false, false, vec![(Reg::Rax, 1)], false, 0, OperationLength::QWord));
        assert!(inst.is_reg_operation());
    }

    #[test]
    fn test_instruction_to_string() {
        let mut inst = Instruction::new(0x1234, Opcode::Mov);
        inst.op_dst = Some(Operand::new(false, false, vec![(Reg::Rax, 1)], false, 0, OperationLength::QWord));
        inst.op_src = Some(Operand::new(false, false, vec![(Reg::Rbx, 1)], false, 0, OperationLength::QWord));
        
        let with_offset = inst.format_with_offset(true);
        assert!(with_offset.contains("0x1234"));
        assert!(with_offset.contains("mov"));
        
        let without_offset = inst.format_with_offset(false);
        assert!(!without_offset.contains("0x1234"));
    }

    // ===== Segment tests =====
    #[test]
    fn test_segment_new() {
        let inst = Arc::new(Instruction::new(0x100, Opcode::Mov));
        let seg = Segment::new(vec![inst]);
        assert_eq!(seg.inst_list.len(), 1);
        assert_eq!(seg.useful_inst_index, 0);
    }

    #[test]
    fn test_segment_to_string() {
        let inst = Arc::new(Instruction::new(0x100, Opcode::Mov));
        let seg = Segment::new(vec![inst]);
        let s = seg.format_with_offset(true);
        assert!(s.contains("0x100"));
    }

    #[test]
    fn test_segment_equality() {
        let inst1 = Arc::new(Instruction::new(0x100, Opcode::Mov));
        let inst2 = Arc::new(Instruction::new(0x100, Opcode::Mov));
        let seg1 = Segment::new(vec![inst1]);
        let seg2 = Segment::new(vec![inst2]);
        assert_eq!(seg1, seg2);
    }

    #[test]
    fn test_segment_inequality() {
        // Segment equality is based on to_string(false) which compares instruction strings
        // Use different opcodes to ensure inequality
        let inst1 = Arc::new(Instruction::new(0x100, Opcode::Mov));
        let inst2 = Arc::new(Instruction::new(0x100, Opcode::Lea));
        let seg1 = Segment::new(vec![inst1]);
        let seg2 = Segment::new(vec![inst2]);
        assert_ne!(seg1, seg2);
    }

    // ===== Solution tests =====
    #[test]
    fn test_solution_default() {
        let sol = Solution::default();
        assert!(!sol.found);
        assert!(sol.output_reg_list.is_empty());
        assert!(sol.output_segments.is_empty());
    }
}

