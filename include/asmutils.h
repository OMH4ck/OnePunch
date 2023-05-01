#ifndef __HEADER_ASMUTILS__
#define __HEADER_ASMUTILS__

#include "common.h"
#include "utils.h"

/* for Instruction */
enum OPCODE {
  OP_NONE = 0,
  OP_MOV,
  OP_LEA,
  OP_POP,
  OP_ADD,
  OP_SUB,
  OP_IMUL,
  OP_MUL,
  OP_DIV,
  OP_PUSH,
  OP_XOR,
  OP_OR,
  OP_AND,
  OP_SHR,
  OP_SHL,
  OP_ROR,
  OP_SAR,
  OP_TEST,
  OP_NOP,
  OP_CMP,
  OP_CALL,
  OP_JMP,
  OP_XCHG,
  OP_JCC,  // opcodes starting with 'j', except 'jmp'
  OP_RET,
  OP_SYSCALL,
  OP_INT3,
  OP_SFENCE,
  OP_BSWAP,
  OP_MOVAPS,
  OP_MOVDQA,
  OP_MOVNTDQ,
  OP_MOVSXD
};

string transfer_op_to_str(OPCODE opcode);
OPCODE transfer_str_to_op(string op_str);
enum OPERATION_LENGTH { kNONE = 0, kBYTE = 1, kWORD = 2, kDWORD = 4, kQWORD = 8 };

/* for reg */
enum REG {
  REG_NONE = 0,
  REG_64_START,
  REG_RAX,
  REG_RCX,
  REG_RDX,
  REG_RBX,
  REG_RSI,
  REG_RDI,
  REG_RSP,
  REG_RBP,
  REG_R8,
  REG_R9,
  REG_R10,
  REG_R11,
  REG_R12,
  REG_R13,
  REG_R14,
  REG_R15,
  REG_RIP,
  REG_64_END,
  REG_32_START,
  REG_EAX,
  REG_ECX,
  REG_EDX,
  REG_EBX,
  REG_ESI,
  REG_EDI,
  REG_ESP,
  REG_EBP,
  REG_R8D,
  REG_R9D,
  REG_R10D,
  REG_R11D,
  REG_R12D,
  REG_R13D,
  REG_R14D,
  REG_R15D,
  REG_EIP,
  REG_32_END,
  REG_16_START,
  REG_AX,
  REG_CX,
  REG_DX,
  REG_BX,
  REG_SI,
  REG_DI,
  REG_SP,
  REG_BP,
  REG_R8W,
  REG_R9W,
  REG_R10W,
  REG_R11W,
  REG_R12W,
  REG_R13W,
  REG_R14W,
  REG_R15W,
  REG_IP,
  REG_16_END,
  REG_8_START,
  REG_AL,
  REG_CL,
  REG_DL,
  REG_BL,
  REG_SIL,
  REG_DIL,
  REG_SPL,
  REG_BPL,
  REG_R8B,
  REG_R9B,
  REG_R10B,
  REG_R11B,
  REG_R12B,
  REG_R13B,
  REG_R14B,
  REG_R15B,
  REG_8_END,
  REG_8H_START,
  REG_AH,
  REG_CH,
  REG_DH,
  REG_BH,
  REG_SIH,
  REG_DIH,
  REG_SPH,
  REG_BPH,
  REG_R8H,
  REG_R9H,
  REG_R10H,
  REG_R11H,
  REG_R12H,
  REG_R13H,
  REG_R14H,
  REG_R15H,
  REG_8H_END,
  REG_CR4,
  REG_CR3
};

class Segment;
class Instruction;
class Operand;

class Segment {
public:
  Segment(vector<Instruction *> &inst_list);
  bool operator==(const Segment &rhs) const {
    return this->to_string(false) == rhs.to_string(false);
  }

  void set_inst_list(vector<Instruction *> &inst_list);
  vector<Instruction *> get_inst_list();
  string to_string(bool display_offset) const;
  void set_useful_inst_index(unsigned int idx);

  unsigned int useful_inst_index_ = 0;
  // void remove_prefix_insns(vector<string> &remove_list); //should be done in get_call_segment
  vector<Instruction *> inst_list_;
  const string asm_text;
  void print_inst() const;
  ~Segment();
};

class Instruction {
public:
  Instruction(unsigned long offset, string opcode, string operands);

  bool is_reg_operation();

  string to_string(bool display_offset) const;

  unsigned long offset_;
  OPCODE opcode_;
  Operand *op_src_;
  Operand *op_dst_;
  unsigned int operand_num_;
  OPERATION_LENGTH operation_length_;

  string original_inst_;

  OPCODE transfer_op(string &op_str);  // transfer string to OPCODE. see utils.h
  void parse_operands(string &operands, vector<Operand *> &operand_list);
  ~Instruction();
};

/* e.g. DWORD PTR [rax-4*rbx+0x1000]
 * is_deference_ = True
 * reg_list_ = [<rax, 1>, <rbx, 4>]    (the int field never being 0)
 * reg_num_ = 2
 * literal_sym_ = 0  (0 for +, 1 for -)
 * literal_num_ = 0x1000
 */
class Operand {
public:
  Operand(bool is_dereference, bool is_contain_seg_reg, vector<pair<REG, int>> &regs_list,
          bool literal_sym, unsigned long literal_num = 0,
          OPERATION_LENGTH operation_length = kNONE)
      : is_dereference_(is_dereference),
        contain_seg_reg_(is_contain_seg_reg),
        reg_list_(regs_list),
        literal_sym_(literal_sym),
        literal_num_(literal_num),
        reg_num_(regs_list.size()),
        operation_length_(operation_length) {}

  bool is_literal_number();
  bool is_reg_operation();
  bool contain_reg(REG reg);
  bool is_reg64_operation();
  bool is_memory_access();

  bool contain_segment_reg();  // contains special segment register e.g. 'fs:[xxx]'

  REG get_reg_op();
  /* return <REG, <start, end>>. For no range need to be removed , return <REG_NONE, pair<0, 0>> */
  pair<REG, pair<long, long>> get_used_range();

  string to_string();
  string transfer_operation_len_to_str(OPERATION_LENGTH length);

  bool is_dereference_;
  bool contain_seg_reg_;
  vector<pair<REG, int>> reg_list_;

  bool literal_sym_;
  unsigned long literal_num_;
  unsigned int reg_num_;
  OPERATION_LENGTH operation_length_;
  long imm_;
};

/* this is not using
vector<vector<string>> ASMUTILS_REGS = {
    {"rax", "eax", "ax", "al"},
    {"rcx", "ecx", "cx", "cl"},
    {"rdx", "edx", "dx", "dl"},
    {"rbx", "ebx", "bx", "bl"},
    {"rsi", "esi", "si", "sil"},
    {"rdi", "edi", "di", "dil"},
    {"rsp", "esp", "sp", "spl"},
    {"rbp", "ebp", "bp", "bpl"},
    {"r8", "r8d", "r8w", "r8b"},
    {"r9", "r9d", "r9w", "r9b"},
    {"r10", "r10d", "r10w", "r10b"},
    {"r11", "r11d", "r11w", "r11b"},
    {"r12", "r12d", "r12w", "r12b"},
    {"r13", "r13d", "r13w", "r13b"},
    {"r14", "r14d", "r14w", "r14b"},
    {"r15", "r15d", "r15w", "r15b"}};
*/

REG get_reg_by_str(const string &str);
string get_reg_str_by_reg(REG);

string refine(string line);
vector<Instruction *> get_disasm_code(string filename);
vector<Segment *> get_call_segment(vector<Instruction *> &insts);

REG find_reg64(REG r);  // e.g., transfer eax to rax. return REG_NONE if cannot find
bool is_reg64(REG r);
bool is_reg64_operation(Instruction *inst);

unsigned long locate_next_inst_addr(unsigned long offset,
                                    const vector<pair<Segment *, unsigned>> &code_segments);
OPERATION_LENGTH check_operation_length(const string &operand);
#endif
