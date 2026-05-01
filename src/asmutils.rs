//! Assembly utilities - disassembly, parsing, and register/opcode conversion

use crate::types::*;
use crate::utils::{str_split, str_trim};
use std::collections::HashMap;
use std::process::Command;
use std::sync::Arc;
use std::sync::OnceLock;

/// Get register enum from string
pub fn get_reg_by_str(s: &str) -> Option<Reg> {
    static MAP: OnceLock<HashMap<&'static str, Reg>> = OnceLock::new();
    let map = MAP.get_or_init(|| {
        let mut m = HashMap::new();
        // 64 bit
        m.insert("rax", Reg::Rax);
        m.insert("rbx", Reg::Rbx);
        m.insert("rcx", Reg::Rcx);
        m.insert("rdx", Reg::Rdx);
        m.insert("rdi", Reg::Rdi);
        m.insert("rsi", Reg::Rsi);
        m.insert("rsp", Reg::Rsp);
        m.insert("rbp", Reg::Rbp);
        m.insert("r8", Reg::R8);
        m.insert("r9", Reg::R9);
        m.insert("r10", Reg::R10);
        m.insert("r11", Reg::R11);
        m.insert("r12", Reg::R12);
        m.insert("r13", Reg::R13);
        m.insert("r14", Reg::R14);
        m.insert("r15", Reg::R15);
        m.insert("rip", Reg::Rip);
        m.insert("cr4", Reg::Cr4);
        m.insert("cr3", Reg::Cr3);
        // 32 bit
        m.insert("eax", Reg::Eax);
        m.insert("ebx", Reg::Ebx);
        m.insert("ecx", Reg::Ecx);
        m.insert("edx", Reg::Edx);
        m.insert("edi", Reg::Edi);
        m.insert("esi", Reg::Esi);
        m.insert("esp", Reg::Esp);
        m.insert("ebp", Reg::Ebp);
        m.insert("r8d", Reg::R8d);
        m.insert("r9d", Reg::R9d);
        m.insert("r10d", Reg::R10d);
        m.insert("r11d", Reg::R11d);
        m.insert("r12d", Reg::R12d);
        m.insert("r13d", Reg::R13d);
        m.insert("r14d", Reg::R14d);
        m.insert("r15d", Reg::R15d);
        m.insert("eip", Reg::Eip);
        // 16 bit
        m.insert("ax", Reg::Ax);
        m.insert("bx", Reg::Bx);
        m.insert("cx", Reg::Cx);
        m.insert("dx", Reg::Dx);
        m.insert("di", Reg::Di);
        m.insert("si", Reg::Si);
        m.insert("sp", Reg::Sp);
        m.insert("bp", Reg::Bp);
        m.insert("r8w", Reg::R8w);
        m.insert("r9w", Reg::R9w);
        m.insert("r10w", Reg::R10w);
        m.insert("r11w", Reg::R11w);
        m.insert("r12w", Reg::R12w);
        m.insert("r13w", Reg::R13w);
        m.insert("r14w", Reg::R14w);
        m.insert("r15w", Reg::R15w);
        m.insert("ip", Reg::Ip);
        // 8 bit low
        m.insert("al", Reg::Al);
        m.insert("bl", Reg::Bl);
        m.insert("cl", Reg::Cl);
        m.insert("dl", Reg::Dl);
        m.insert("dil", Reg::Dil);
        m.insert("sil", Reg::Sil);
        m.insert("spl", Reg::Spl);
        m.insert("bpl", Reg::Bpl);
        m.insert("r8b", Reg::R8b);
        m.insert("r9b", Reg::R9b);
        m.insert("r10b", Reg::R10b);
        m.insert("r11b", Reg::R11b);
        m.insert("r12b", Reg::R12b);
        m.insert("r13b", Reg::R13b);
        m.insert("r14b", Reg::R14b);
        m.insert("r15b", Reg::R15b);
        // 8 bit high
        m.insert("ah", Reg::Ah);
        m.insert("bh", Reg::Bh);
        m.insert("ch", Reg::Ch);
        m.insert("dh", Reg::Dh);
        m
    });
    map.get(s).copied()
}

/// Get string from register enum
pub fn get_reg_str_by_reg(reg: Reg) -> &'static str {
    static MAP: OnceLock<HashMap<Reg, &'static str>> = OnceLock::new();
    let map = MAP.get_or_init(|| {
        let mut m = HashMap::new();
        // 64 bit
        m.insert(Reg::Rax, "rax");
        m.insert(Reg::Rbx, "rbx");
        m.insert(Reg::Rcx, "rcx");
        m.insert(Reg::Rdx, "rdx");
        m.insert(Reg::Rdi, "rdi");
        m.insert(Reg::Rsi, "rsi");
        m.insert(Reg::Rsp, "rsp");
        m.insert(Reg::Rbp, "rbp");
        m.insert(Reg::R8, "r8");
        m.insert(Reg::R9, "r9");
        m.insert(Reg::R10, "r10");
        m.insert(Reg::R11, "r11");
        m.insert(Reg::R12, "r12");
        m.insert(Reg::R13, "r13");
        m.insert(Reg::R14, "r14");
        m.insert(Reg::R15, "r15");
        m.insert(Reg::Rip, "rip");
        m.insert(Reg::Cr4, "cr4");
        m.insert(Reg::Cr3, "cr3");
        // 32 bit
        m.insert(Reg::Eax, "eax");
        m.insert(Reg::Ebx, "ebx");
        m.insert(Reg::Ecx, "ecx");
        m.insert(Reg::Edx, "edx");
        m.insert(Reg::Edi, "edi");
        m.insert(Reg::Esi, "esi");
        m.insert(Reg::Esp, "esp");
        m.insert(Reg::Ebp, "ebp");
        m.insert(Reg::R8d, "r8d");
        m.insert(Reg::R9d, "r9d");
        m.insert(Reg::R10d, "r10d");
        m.insert(Reg::R11d, "r11d");
        m.insert(Reg::R12d, "r12d");
        m.insert(Reg::R13d, "r13d");
        m.insert(Reg::R14d, "r14d");
        m.insert(Reg::R15d, "r15d");
        m.insert(Reg::Eip, "eip");
        // 16 bit
        m.insert(Reg::Ax, "ax");
        m.insert(Reg::Bx, "bx");
        m.insert(Reg::Cx, "cx");
        m.insert(Reg::Dx, "dx");
        m.insert(Reg::Di, "di");
        m.insert(Reg::Si, "si");
        m.insert(Reg::Sp, "sp");
        m.insert(Reg::Bp, "bp");
        m.insert(Reg::R8w, "r8w");
        m.insert(Reg::R9w, "r9w");
        m.insert(Reg::R10w, "r10w");
        m.insert(Reg::R11w, "r11w");
        m.insert(Reg::R12w, "r12w");
        m.insert(Reg::R13w, "r13w");
        m.insert(Reg::R14w, "r14w");
        m.insert(Reg::R15w, "r15w");
        m.insert(Reg::Ip, "ip");
        // 8 bit low
        m.insert(Reg::Al, "al");
        m.insert(Reg::Bl, "bl");
        m.insert(Reg::Cl, "cl");
        m.insert(Reg::Dl, "dl");
        m.insert(Reg::Dil, "dil");
        m.insert(Reg::Sil, "sil");
        m.insert(Reg::Spl, "spl");
        m.insert(Reg::Bpl, "bpl");
        m.insert(Reg::R8b, "r8b");
        m.insert(Reg::R9b, "r9b");
        m.insert(Reg::R10b, "r10b");
        m.insert(Reg::R11b, "r11b");
        m.insert(Reg::R12b, "r12b");
        m.insert(Reg::R13b, "r13b");
        m.insert(Reg::R14b, "r14b");
        m.insert(Reg::R15b, "r15b");
        // 8 bit high
        m.insert(Reg::Ah, "ah");
        m.insert(Reg::Bh, "bh");
        m.insert(Reg::Ch, "ch");
        m.insert(Reg::Dh, "dh");
        m
    });
    map.get(&reg).copied().unwrap_or("None")
}

/// Convert string to opcode
pub fn transfer_str_to_op(s: &str) -> Opcode {
    static MAP: OnceLock<HashMap<&'static str, Opcode>> = OnceLock::new();
    let map = MAP.get_or_init(|| {
        let mut m = HashMap::new();
        m.insert("add", Opcode::Add);
        m.insert("or", Opcode::Or);
        m.insert("adc", Opcode::Add);
        m.insert("sbb", Opcode::Sub);
        m.insert("and", Opcode::And);
        m.insert("sub", Opcode::Sub);
        m.insert("xor", Opcode::Xor);
        m.insert("cmp", Opcode::Cmp);
        m.insert("push", Opcode::Push);
        m.insert("pop", Opcode::Pop);
        m.insert("movsxd", Opcode::Movsxd);
        m.insert("mul", Opcode::Mul);
        m.insert("imul", Opcode::Imul);
        // Jump conditions
        for jcc in [
            "jo", "jno", "jb", "jnae", "jc", "jnb", "jae", "jnc", "jz", "je", "jnz", "jne", "jbe",
            "jna", "jnbe", "ja", "js", "jns", "jp", "jpe", "jnp", "jpo", "jl", "jnge", "jnl",
            "jge", "jle", "jng", "jnle", "jg",
        ] {
            m.insert(jcc, Opcode::Jcc);
        }
        m.insert("test", Opcode::Test);
        m.insert("xchg", Opcode::Xchg);
        m.insert("lea", Opcode::Lea);
        m.insert("mov", Opcode::Mov);
        m.insert("movs", Opcode::Mov);
        m.insert("nop", Opcode::Nop);
        m.insert("cmps", Opcode::Cmp);
        m.insert("cmpsb", Opcode::Cmp);
        m.insert("cmpsw", Opcode::Cmp);
        m.insert("cmpsd", Opcode::Cmp);
        m.insert("cmpsq", Opcode::Cmp);
        m.insert("shl", Opcode::Shl);
        m.insert("shr", Opcode::Shr);
        m.insert("sar", Opcode::Sar);
        m.insert("ror", Opcode::Ror);
        m.insert("ret", Opcode::Ret);
        m.insert("retn", Opcode::Ret);
        m.insert("retf", Opcode::Ret);
        m.insert("iret", Opcode::Ret);
        m.insert("iretd", Opcode::Ret);
        m.insert("iretq", Opcode::Ret);
        m.insert("int", Opcode::Int3);
        m.insert("into", Opcode::Int3);
        m.insert("call", Opcode::Call);
        m.insert("jmp", Opcode::Jmp);
        m.insert("div", Opcode::Div);
        m.insert("syscall", Opcode::Syscall);
        m.insert("sfence", Opcode::Sfence);
        m.insert("bswap", Opcode::Bswap);
        m.insert("movaps", Opcode::Movaps);
        m.insert("movdqa", Opcode::Movdqa);
        m.insert("movntdq", Opcode::Movntdq);
        m
    });
    map.get(s).copied().unwrap_or(Opcode::None)
}

/// Convert opcode to string
pub fn transfer_op_to_str(opcode: Opcode) -> &'static str {
    match opcode {
        Opcode::None => "None",
        Opcode::Mov => "mov",
        Opcode::Lea => "lea",
        Opcode::Pop => "pop",
        Opcode::Add => "add",
        Opcode::Sub => "sub",
        Opcode::Imul => "imul",
        Opcode::Mul => "mul",
        Opcode::Div => "div",
        Opcode::Push => "push",
        Opcode::Xor => "xor",
        Opcode::Or => "or",
        Opcode::And => "and",
        Opcode::Shr => "shr",
        Opcode::Shl => "shl",
        Opcode::Ror => "ror",
        Opcode::Sar => "sar",
        Opcode::Test => "test",
        Opcode::Nop => "nop",
        Opcode::Cmp => "cmp",
        Opcode::Call => "call",
        Opcode::Jmp => "jmp",
        Opcode::Xchg => "xchg",
        Opcode::Jcc => "jcc",
        Opcode::Ret => "ret",
        Opcode::Syscall => "syscall",
        Opcode::Int3 => "int3",
        Opcode::Sfence => "sfence",
        Opcode::Bswap => "bswap",
        Opcode::Movaps => "movaps",
        Opcode::Movdqa => "movdqa",
        Opcode::Movntdq => "movntdq",
        Opcode::Movsxd => "movsxd",
    }
}

/// Check operation length from operand string
fn check_operation_length(operand: &str) -> OperationLength {
    if operand.contains("BYTE") {
        return OperationLength::Byte;
    }
    if operand.contains("DWORD") {
        return OperationLength::DWord;
    }
    if operand.contains("QWORD") {
        return OperationLength::QWord;
    }
    if operand.contains("WORD") {
        return OperationLength::Word;
    }

    let trimmed = str_trim(operand);
    if let Some(reg) = get_reg_by_str(trimmed) {
        if reg.is_reg64() {
            OperationLength::QWord
        } else {
            OperationLength::DWord
        }
    } else {
        OperationLength::QWord
    }
}

/// Refine a disassembly line (normalize spacing)
fn refine(mut line: String) -> String {
    let replacements = [
        ("  ", " "),
        (" + ", "+"),
        (" - ", "-"),
        (",", ", "),
    ];
    for (from, to) in replacements {
        let mut last = 0;
        while let Some(pos) = line[last..].find(from) {
            let abs_pos = last + pos;
            line.replace_range(abs_pos..abs_pos + from.len(), to);
            last = abs_pos + to.len();
        }
    }
    line
}

/// Parse operands from a string
fn parse_operands(operands: &str) -> (Vec<Operand>, OperationLength) {
    let operand_str_list = str_split(operands, ",");
    let mut operand_list = Vec::new();
    let mut operation_length = OperationLength::QWord;

    for each in operand_str_list {
        let each = str_trim(&each);
        let tmp_l = check_operation_length(each);
        if (tmp_l as u8) < (operation_length as u8) {
            operation_length = tmp_l;
        }

        let mut is_contain_seg_reg = false;
        let mut is_dereference = false;

        // Handle segment register prefix (e.g., fs:[...])
        let each = if let Some(seg_pos) = each.find(':') {
            is_contain_seg_reg = true;
            &each[seg_pos + 1..]
        } else {
            each
        };

        // Handle dereference
        let each = if let Some(bracket_start) = each.find('[') {
            if let Some(bracket_end) = each.find(']') {
                is_dereference = true;
                &each[bracket_start + 1..bracket_end]
            } else {
                each
            }
        } else {
            each
        };

        // Parse the operand expression
        let mut reg_list: Vec<(Reg, i32)> = Vec::new();
        let mut imm: i64 = 0;
        let mut imm_sym = true; // false for positive

        // Split by + first, then handle - within each part
        let plus_parts = str_split(each, "+");
        let plus_parts: Vec<_> = if plus_parts.is_empty() {
            vec![each.to_string()]
        } else {
            plus_parts
        };

        let mut reg_info: Vec<(String, bool)> = Vec::new(); // (token, is_positive)
        for part in &plus_parts {
            let minus_parts = str_split(part, "-");
            if minus_parts.is_empty() {
                reg_info.push((part.to_string(), true));
            } else {
                reg_info.push((minus_parts[0].clone(), true));
                for neg_part in minus_parts.iter().skip(1) {
                    reg_info.push((neg_part.clone(), false));
                }
            }
        }

        for (token, is_positive) in reg_info {
            let token = str_trim(&token);
            if token.is_empty() {
                continue;
            }

            let mul_parts = str_split(token, "*");
            let coefficient: i32 = if is_positive { 1 } else { -1 };

            if mul_parts.len() == 1 {
                if let Some(reg) = get_reg_by_str(token) {
                    reg_list.push((reg, coefficient));
                } else {
                    // It's an immediate value
                    imm_sym = coefficient != 1;
                    let tmp_str = if imm_sym {
                        format!("-{}", token)
                    } else {
                        token.to_string()
                    };
                    imm = i64::from_str_radix(
                        tmp_str.trim_start_matches("0x").trim_start_matches("-0x"),
                        16,
                    )
                    .unwrap_or(0);
                    if tmp_str.starts_with('-') {
                        imm = -imm;
                    }
                }
            } else if mul_parts.len() == 2 {
                if let Some(reg) = get_reg_by_str(str_trim(&mul_parts[0])) {
                    let num: i32 = i32::from_str_radix(
                        str_trim(&mul_parts[1]).trim_start_matches("0x"),
                        16,
                    )
                    .unwrap_or(1);
                    reg_list.push((reg, coefficient * num));
                }
            }
        }

        let literal_num = if imm >= 0 { imm as u64 } else { 0 };

        let mut operand = Operand::new(
            is_dereference,
            is_contain_seg_reg,
            reg_list,
            imm_sym,
            literal_num,
            operation_length,
        );
        operand.imm = imm;
        operand_list.push(operand);
    }

    (operand_list, operation_length)
}

/// Parse an instruction from offset, opcode, and operands
fn parse_instruction(offset: u64, opcode_str: &str, operands_str: &str, original: &str) -> Option<Instruction> {
    let opcode = transfer_str_to_op(opcode_str);
    if opcode == Opcode::Imul || opcode == Opcode::None {
        return None;
    }

    let mut inst = Instruction::new(offset, opcode);
    inst.original_inst = original.to_string();

    let (operand_list, operation_length) = parse_operands(operands_str);
    inst.operation_length = operation_length;

    if operand_list.len() == 2 {
        inst.op_dst = Some(operand_list[0].clone());
        inst.op_src = Some(operand_list[1].clone());
        inst.operand_num = 2;
    } else if operand_list.len() == 1 {
        inst.op_dst = Some(operand_list[0].clone());
        inst.operand_num = 1;
    }

    Some(inst)
}

/// Disassemble a binary file using objdump
pub fn get_disasm_code(filename: &str) -> Vec<InstrPtr> {
    let output = Command::new("objdump")
        .args(["-M", "intel", "--no-show-raw-insn", "-d", filename])
        .output()
        .expect("Failed to execute objdump");

    let disasm = String::from_utf8_lossy(&output.stdout);
    let mut result = Vec::new();

    for line in disasm.lines() {
        // Remove comments
        let line = if let Some(pos) = line.find('#') {
            &line[..pos]
        } else {
            line
        };
        let line = if let Some(pos) = line.find('<') {
            &line[..pos]
        } else {
            line
        };

        let line = refine(line.to_string());

        // Parse offset
        let Some(offset_pos) = line.find(":\t") else {
            continue;
        };
        let offset_str = &line[..offset_pos];
        let Ok(offset) = u64::from_str_radix(offset_str.trim(), 16) else {
            continue;
        };

        let inst_str = &line[offset_pos + 2..];
        let Some(opcode_pos) = inst_str.find(' ') else {
            continue;
        };
        let opcode_str = &inst_str[..opcode_pos];
        let operands_str = &inst_str[opcode_pos + 1..];

        if let Some(inst) = parse_instruction(offset, opcode_str, operands_str, &line) {
            result.push(Arc::new(inst));
        }
    }

    result
}

/// Check if an operand is "interesting" (contains 64-bit registers, not RIP)
fn is_interesting(operand: &Operand) -> bool {
    for (reg, _) in &operand.reg_list {
        if reg.is_reg64() {
            return *reg != Reg::Rip;
        }
    }
    false
}

/// Check if an opcode is harmful
fn is_harmful(opcode: Opcode) -> bool {
    matches!(
        opcode,
        Opcode::Imul | Opcode::Syscall | Opcode::Int3 | Opcode::None
    )
}

/// Check if an opcode is unwanted for gadget chains
fn is_unwanted_instruction(opcode: Opcode) -> bool {
    matches!(
        opcode,
        Opcode::Nop
            | Opcode::Sfence
            | Opcode::Sar
            | Opcode::Xor
            | Opcode::Add
            | Opcode::Sub
            | Opcode::Mul
            | Opcode::Div
            | Opcode::Ror
            | Opcode::Bswap
            | Opcode::Movaps
            | Opcode::Movdqa
            | Opcode::Movntdq
            | Opcode::Shl
            | Opcode::Shr
            | Opcode::None
    )
}

/// Extract call/jmp segments from instruction list
pub fn get_call_segment(insts: &[InstrPtr]) -> Vec<SegmentPtr> {
    let size = insts.len();
    let mut duplicate_helper: HashMap<String, usize> = HashMap::new();
    let mut result = Vec::new();

    for idx in 0..size {
        let inst = &insts[idx];
        if inst.opcode != Opcode::Call && inst.opcode != Opcode::Jmp {
            continue;
        }

        let Some(ref op_dst) = inst.op_dst else {
            continue;
        };
        if !is_interesting(op_dst) || op_dst.contain_reg(Reg::Rip) {
            continue;
        }

        // Find the start of the segment
        let mut start = idx as i32 - 1;
        while start >= 0 {
            let tmp_inst = &insts[start as usize];
            if matches!(
                tmp_inst.opcode,
                Opcode::Call | Opcode::Jcc | Opcode::Ret | Opcode::Jmp
            ) || is_harmful(tmp_inst.opcode)
            {
                break;
            }
            start -= 1;
        }
        start += 1;

        // Skip unwanted instructions at the start
        while (start as usize) < idx + 1 {
            let tmp_inst = &insts[start as usize];
            let should_skip = is_unwanted_instruction(tmp_inst.opcode)
                || tmp_inst.operation_length != OperationLength::QWord
                || tmp_inst
                    .op_dst
                    .as_ref()
                    .map(|o| {
                        o.contain_reg(Reg::Cr3)
                            || o.contain_reg(Reg::Cr4)
                            || o.contain_reg(Reg::Rip)
                            || o.contain_segment_reg()
                    })
                    .unwrap_or(false)
                || tmp_inst
                    .op_src
                    .as_ref()
                    .map(|o| {
                        o.contain_reg(Reg::Cr4)
                            || o.contain_reg(Reg::Rip)
                            || o.contain_segment_reg()
                    })
                    .unwrap_or(false);

            if !should_skip {
                break;
            }
            start += 1;
        }

        // Build the instruction list for this segment
        let inst_list: Vec<InstrPtr> = (start as usize..=idx)
            .map(|i| Arc::clone(&insts[i]))
            .collect();

        if inst_list.len() <= 1 {
            continue;
        }

        let seg = Segment::new(inst_list);
        let tmp_asm = seg.format_with_offset(false);

        if duplicate_helper.contains_key(&tmp_asm) {
            continue;
        }
        duplicate_helper.insert(tmp_asm, result.len());
        result.push(Arc::new(seg));
    }

    result
}

/// Locate the next instruction address after a given offset in segments
pub fn locate_next_inst_addr(offset: u64, code_segments: &[(SegmentPtr, usize)]) -> u64 {
    for (i, (seg, _)) in code_segments.iter().enumerate() {
        if let Some(last_inst) = seg.inst_list.last() {
            if offset == last_inst.offset {
                if i == code_segments.len() - 1 {
                    return u64::MAX;
                }
                let (next_seg, next_idx) = &code_segments[i + 1];
                return next_seg.inst_list[*next_idx].offset;
            }
        }
    }
    u64::MAX
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_get_reg_by_str() {
        assert_eq!(get_reg_by_str("rax"), Some(Reg::Rax));
        assert_eq!(get_reg_by_str("r15"), Some(Reg::R15));
        assert_eq!(get_reg_by_str("eax"), Some(Reg::Eax));
        assert_eq!(get_reg_by_str("invalid"), None);
    }

    #[test]
    fn test_get_reg_str_by_reg() {
        assert_eq!(get_reg_str_by_reg(Reg::Rax), "rax");
        assert_eq!(get_reg_str_by_reg(Reg::R15), "r15");
        assert_eq!(get_reg_str_by_reg(Reg::None), "None");
    }

    #[test]
    fn test_transfer_str_to_op() {
        assert_eq!(transfer_str_to_op("mov"), Opcode::Mov);
        assert_eq!(transfer_str_to_op("call"), Opcode::Call);
        assert_eq!(transfer_str_to_op("je"), Opcode::Jcc);
        assert_eq!(transfer_str_to_op("invalid"), Opcode::None);
    }

    #[test]
    fn test_transfer_op_to_str() {
        assert_eq!(transfer_op_to_str(Opcode::Mov), "mov");
        assert_eq!(transfer_op_to_str(Opcode::Call), "call");
    }

    #[test]
    fn test_check_operation_length() {
        assert_eq!(check_operation_length("QWORD PTR [rax]"), OperationLength::QWord);
        assert_eq!(check_operation_length("DWORD PTR [rax]"), OperationLength::DWord);
        assert_eq!(check_operation_length("rax"), OperationLength::QWord);
        assert_eq!(check_operation_length("eax"), OperationLength::DWord);
    }

    #[test]
    fn test_refine() {
        assert_eq!(refine("a  b".to_string()), "a b");
        assert_eq!(refine("a + b".to_string()), "a+b");
        assert_eq!(refine("a - b".to_string()), "a-b");
    }

    #[test]
    fn test_parse_operands_simple_reg() {
        let (ops, len) = parse_operands("rax");
        assert_eq!(ops.len(), 1);
        assert_eq!(ops[0].reg_list.len(), 1);
        assert_eq!(ops[0].reg_list[0].0, Reg::Rax);
        assert_eq!(len, OperationLength::QWord);
    }

    #[test]
    fn test_parse_operands_memory() {
        let (ops, _) = parse_operands("QWORD PTR [rax+0x10]");
        assert_eq!(ops.len(), 1);
        assert!(ops[0].is_dereference);
        assert_eq!(ops[0].reg_list[0].0, Reg::Rax);
    }

    #[test]
    fn test_parse_operands_two() {
        let (ops, _) = parse_operands("rax, rbx");
        assert_eq!(ops.len(), 2);
        assert_eq!(ops[0].reg_list[0].0, Reg::Rax);
        assert_eq!(ops[1].reg_list[0].0, Reg::Rbx);
    }

    #[test]
    fn test_is_harmful() {
        assert!(is_harmful(Opcode::Syscall));
        assert!(is_harmful(Opcode::Int3));
        assert!(!is_harmful(Opcode::Mov));
    }

    #[test]
    fn test_is_unwanted_instruction() {
        assert!(is_unwanted_instruction(Opcode::Nop));
        assert!(is_unwanted_instruction(Opcode::Sfence));
        assert!(!is_unwanted_instruction(Opcode::Mov));
    }

    #[test]
    fn test_locate_next_inst_addr_empty() {
        assert_eq!(locate_next_inst_addr(0x100, &[]), u64::MAX);
    }

    #[test]
    fn test_get_reg_by_str_all_64bit() {
        // Test all 64-bit registers
        for reg_name in ["rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"] {
            assert!(get_reg_by_str(reg_name).is_some());
        }
    }

    #[test]
    fn test_get_reg_by_str_32bit() {
        assert_eq!(get_reg_by_str("eax"), Some(Reg::Eax));
        assert_eq!(get_reg_by_str("r8d"), Some(Reg::R8d));
    }

    #[test]
    fn test_get_reg_by_str_16bit() {
        assert_eq!(get_reg_by_str("ax"), Some(Reg::Ax));
        assert_eq!(get_reg_by_str("r8w"), Some(Reg::R8w));
    }

    #[test]
    fn test_get_reg_by_str_8bit() {
        assert_eq!(get_reg_by_str("al"), Some(Reg::Al));
        assert_eq!(get_reg_by_str("ah"), Some(Reg::Ah));
        assert_eq!(get_reg_by_str("r8b"), Some(Reg::R8b));
    }

    #[test]
    fn test_transfer_str_to_op_more_opcodes() {
        assert_eq!(transfer_str_to_op("lea"), Opcode::Lea);
        assert_eq!(transfer_str_to_op("push"), Opcode::Push);
        assert_eq!(transfer_str_to_op("pop"), Opcode::Pop);
        assert_eq!(transfer_str_to_op("add"), Opcode::Add);
        assert_eq!(transfer_str_to_op("sub"), Opcode::Sub);
        assert_eq!(transfer_str_to_op("xor"), Opcode::Xor);
        assert_eq!(transfer_str_to_op("and"), Opcode::And);
        assert_eq!(transfer_str_to_op("or"), Opcode::Or);
        assert_eq!(transfer_str_to_op("jmp"), Opcode::Jmp);
        assert_eq!(transfer_str_to_op("ret"), Opcode::Ret);
    }

    #[test]
    fn test_transfer_str_to_op_conditional_jumps() {
        // All conditional jumps should map to Jcc
        for jcc in ["je", "jne", "jz", "jnz", "jg", "jge", "jl", "jle", "ja", "jae", "jb", "jbe"] {
            assert_eq!(transfer_str_to_op(jcc), Opcode::Jcc);
        }
    }

    #[test]
    fn test_check_operation_length_word_byte() {
        assert_eq!(check_operation_length("WORD PTR [rax]"), OperationLength::Word);
        assert_eq!(check_operation_length("BYTE PTR [rax]"), OperationLength::Byte);
    }

    #[test]
    fn test_parse_operands_negative_offset() {
        let (ops, _) = parse_operands("QWORD PTR [rax-0x10]");
        assert_eq!(ops.len(), 1);
        assert!(ops[0].is_dereference);
    }

    #[test]
    fn test_parse_operands_immediate() {
        let (ops, _) = parse_operands("0x1234");
        assert_eq!(ops.len(), 1);
        assert!(ops[0].reg_list.is_empty());
        assert_eq!(ops[0].literal_num, 0x1234);
    }

    #[test]
    fn test_parse_operands_rip_relative() {
        let (ops, _) = parse_operands("QWORD PTR [rip+0x12345]");
        assert_eq!(ops.len(), 1);
        assert!(ops[0].contain_reg(Reg::Rip));
    }

    #[test]
    fn test_is_harmful_more() {
        // Test harmful opcodes that exist in the enum
        assert!(is_harmful(Opcode::Int3));
        assert!(is_harmful(Opcode::Syscall));
        assert!(!is_harmful(Opcode::Mov));
        assert!(!is_harmful(Opcode::Lea));
    }

    #[test]
    fn test_is_unwanted_more() {
        // Test unwanted opcodes that exist in the enum
        assert!(is_unwanted_instruction(Opcode::Nop));
        assert!(is_unwanted_instruction(Opcode::Sfence));
        assert!(!is_unwanted_instruction(Opcode::Call));
    }
}

