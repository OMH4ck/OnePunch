#include "../include/asmutils.h"

#include <array>
#include <cstdio>
#include <cstdlib>

#include "../include/utils.h"

//#define DBG
using namespace std;
Segment::Segment(vector<InstrPtr> &inst_list) {
  this->inst_list_ = inst_list;
  this->useful_inst_index_ = 0;
}

void Segment::set_inst_list(vector<InstrPtr> &inst_list) { this->inst_list_ = inst_list; }

vector<InstrPtr> Segment::get_inst_list() { return this->inst_list_; }

string Segment::to_string(bool display_offset = true) const {
  string res;
  for (auto inst : this->inst_list_) {
    res += inst->to_string(display_offset) + "\n";
  }

  return res;
}

Segment::~Segment() {
  // for (auto i : inst_list_) delete i;
  // delete this;
}

Instruction::Instruction(unsigned long offset, string opcode, string operands) : offset_(offset) {
  this->opcode_ = transfer_op(opcode);
  this->op_src_ = std::nullopt;
  this->op_dst_ = std::nullopt;
  // assert(this->opcode_ != OP_NONE);
  if (this->opcode_ == OP_IMUL || this->opcode_ == OP_NONE) return;
  vector<Operand> operand_list;
#ifdef DBG
  cout << opcode;
#endif
  parse_operands(operands, operand_list);
  this->operand_num_ = 0;
  // this->operation_length_ = kNONE;
  if (operand_list.size() == 2) {
    this->op_src_ = operand_list[1];
    this->op_dst_ = operand_list[0];
    this->operand_num_ = 2;
  } else if (operand_list.size() == 1) {
    this->op_dst_ = operand_list[0];
    this->operand_num_ = 1;
  }
}

OPCODE Instruction::transfer_op(string &op_str) {
  OPCODE res = OP_NONE;

  // to do
  res = transfer_str_to_op(op_str);

  return res;
}

bool is_reg64(REG r) { return r > REG_64_START && r < REG_64_END; }

OPERATION_LENGTH check_operation_length(const string &operand) {
  // case 1: dereference
  if (operand.find("BYTE") != string::npos) return kBYTE;
  if (operand.find("DWORD") != string::npos) return kDWORD;
  if (operand.find("QWORD") != string::npos) return kQWORD;
  if (operand.find("WORD") != string::npos) return kWORD;

  string trimmed_str = str_trim(operand);
  auto reg = get_reg_by_str(trimmed_str);
  if (reg != REG_NONE) {
    // case 2: reg
    if (is_reg64(reg))
      return kQWORD;
    else
      return kDWORD;  // this doesn't matter much;
  } else {
    // case 3: imm
    return kQWORD;  // this might be incorrect
  }
}

void Instruction::parse_operands(string &operands, vector<Operand> &operand_list) {
  // to do
  // fill into operand_list
  std::stringstream ss(operands);
  string line;
  vector<string> operand_str_list = str_split(operands, ",");

  operation_length_ = kQWORD;
#ifdef DBG
  cout << operands << endl;
#endif
  assert(operand_str_list.size() < 3);
  for (auto each : operand_str_list) {
    each = str_trim(each);
    auto seg_reg_pos = each.find(":");
    auto deference_pos = each.find("[");
    auto tmp_l = check_operation_length(each);
    if (tmp_l < operation_length_) operation_length_ = tmp_l;

    auto is_contain_seg_reg = false;
    auto is_dereference = false;

    if (seg_reg_pos != string::npos) {
      is_contain_seg_reg = true;
      if (deference_pos != string::npos) deference_pos = deference_pos - seg_reg_pos - 1;
      each = each.substr(seg_reg_pos + 1);
    }

    if (deference_pos != string::npos) {
      auto end_deference = each.find("]");
      assert(end_deference != string::npos);
      each = each.substr(deference_pos + 1, end_deference - deference_pos - 1);
      is_dereference = true;
    }

    vector<pair<REG, int>> reg_list;
    vector<pair<string, bool>> reg_info;
    vector<string> reg_info_plus = str_split(each, "+");
    if (reg_info_plus.size() == 0) {
      reg_info_plus.push_back(each);
    }
    for (auto &s : reg_info_plus) {
      vector<string> reg_info_minus = str_split(s, "-");
      if (reg_info_minus.size() == 0)
        reg_info.push_back(make_pair(s, true));
      else {
        reg_info.push_back(make_pair(reg_info_minus[0], true));
        for (auto itr = reg_info_minus.begin() + 1; itr != reg_info_minus.end(); itr++) {
          reg_info.push_back(make_pair(*itr, false));
        }
      }
    }

    long imm = 0;
    bool imm_sym = true;
    for (auto &reg_pair : reg_info) {
      auto &reg_str = reg_pair.first;
      auto is_positive = reg_pair.second;
      auto reg_info_mul = str_split(reg_str, "*");

      auto coefficient = 1;
      if (!is_positive) coefficient = -1;
      if (reg_info_mul.size() == 1) {
        auto reg = get_reg_by_str(reg_str);
        if (reg != REG_NONE) {
          reg_list.push_back(make_pair(reg, coefficient));
        } else {
          char *endptr;
          imm_sym = coefficient == 1 ? false : true;  // false (0) for positive
          string tmp_str;
          if (imm_sym)
            tmp_str = "-" + reg_str;
          else
            tmp_str = reg_str;
          imm = strtoul(tmp_str.c_str(), &endptr, 16);
        }
      } else {
        assert(reg_info_mul.size() == 2);
        char *endptr;
        auto reg = get_reg_by_str(reg_info_mul[0]);
        // assert(reg != REG_NONE);
        if (reg == REG_NONE) return;
        int num = coefficient * strtol(reg_info_mul[1].c_str(), &endptr, 16);
        reg_list.push_back(make_pair(reg, num));
      }
    }
#ifdef DBG
    cout << "operand: "
         << "is_deference=" << is_dereference << ", is_contain_seg_reg=" << is_contain_seg_reg
         << ", reg_list=";
    for (auto &i : reg_list) {
      cout << "<" << i.first << "," << i.second << "> ";
    }
    cout << ", imm_sym=" << imm_sym << ", imm=" << imm
         << ", operation_length_=" << operation_length_ << endl;
#endif
    Operand operand(is_dereference, is_contain_seg_reg, reg_list, imm_sym, imm, operation_length_);
    operand.imm_ = imm;
    operand_list.push_back(operand);
  }

  return;
}

bool Instruction::is_reg_operation() {
  return op_src_->is_reg_operation() || op_dst_->is_reg_operation();
}

string Instruction::to_string(bool display_offset = true) const {
  char res[100] = {0};
  string operand_str;

  if (op_dst_.has_value()) operand_str += op_dst_.value().to_string() + ",";  // BUG
  if (op_src_.has_value()) operand_str += op_src_.value().to_string() + ",";  // BUG

  auto sz = operand_str.size();
  if (sz != 0) operand_str = operand_str.substr(0, sz - 1);

  auto opcode = transfer_op_to_str(this->opcode_);
  if (display_offset) {
    sprintf(res, "0x%lx:\t\t%s %s\n", this->offset_, opcode.c_str(), operand_str.c_str());
  } else {
    sprintf(res, "%s %s\n", opcode.c_str(), operand_str.c_str());
  }

  return string(res);
}

string Operand::to_string() const {
  string res = " ";

  if (this->is_dereference_) res += this->transfer_operation_len_to_str(operation_length_) + "[";
  bool flag = true;
  for (auto &reg_info : this->reg_list_) {
    auto reg = get_reg_str_by_reg(reg_info.first);
    auto num = reg_info.second;
    char tmp[30] = {0};
    if (num > 1) {
      if (flag == true) {
        sprintf(tmp, "%s*0x%x", reg.c_str(), num);
        flag = false;
      } else {
        sprintf(tmp, "+%s*0x%x", reg.c_str(), num);
      }
    } else if (num == 1) {
      if (flag == true) {
        sprintf(tmp, "%s", reg.c_str());  // BUG
        flag = false;
      } else {
        sprintf(tmp, "+%s", reg.c_str());  // BUG
      }
    } else if (num < 0) {
      sprintf(tmp, "-%s*0x%x", reg.c_str(), -num);
    } else {
      assert(0);
    }
    res += string(tmp);
  }

  if (this->literal_sym_ == 0) {
    char tmp[30] = {0};
    if (flag == true) {
      sprintf(tmp, "0x%lx", this->literal_num_);
      res += string(tmp);
      flag = false;
    } else {
      sprintf(tmp, "+0x%lx", this->literal_num_);
      res += string(tmp);
    }
  } else {
    char tmp[30] = {0};
    sprintf(tmp, "-0x%lx", this->literal_num_);
    res += string(tmp);
  }

  if (this->is_dereference_) res += "]";

  return res;
}

bool Operand::contain_reg(REG reg) {
  for (const auto &each : reg_list_) {
    if (each.first == reg) {
      return true;
    }
  }
  return false;
}

REG Operand::get_reg_op() const {
  assert(reg_num_ == 1);
  return reg_list_[0].first;
}

pair<REG, pair<long, long>> Operand::get_used_range() {
  pair<REG, pair<long, long>> res(
      make_pair<REG, pair<long, long>>(REG_NONE, make_pair<long, long>(0, 0)));

  if (this->reg_num_ != 1 || is_dereference_ == false || reg_list_[0].second != 1) return res;

  auto reg = reg_list_[0].first;
  // auto start = this->literal_sym_ == 0 ? this->literal_num_ : -this->literal_num_;
  auto start = imm_;
  auto end = start + int(operation_length_);
  res.first = reg;
  res.second.first = start;
  res.second.second = end;

  return res;
}

string refine(string line) {
  string res;
  map<string, string> replace_table;

  replace_table["  "] = " ";
  replace_table[" + "] = "+";
  replace_table[" - "] = "-";
  replace_table[","] = ", ";
  for (auto &each : replace_table) {
    int last = 0;
    while (true) {
      auto pos = line.find(each.first, last + 1);
      if (pos == string::npos) break;
      line.replace(pos, each.first.size(), each.second);
      last = pos;
    }
  }

  return line;
}

bool Operand::is_reg64_operation() {
  /*
  for (auto &reg_info : reg_list_) {
      auto reg = reg_info.first;
      if (is_reg64(reg)) return true;
  }
  */

  return this->reg_list_.size() != 0 && this->operation_length_ == OPERATION_LENGTH::kQWORD;
}

bool Operand::contain_segment_reg() { return contain_seg_reg_; }

bool Operand::is_memory_access() { return is_dereference_; }

bool Operand::is_reg_operation() { return reg_num_ && (!is_dereference_); }

bool Operand::is_literal_number() {
  return reg_num_ == 0 && is_dereference_ == false && literal_num_;
}

vector<InstrPtr> get_disasm_code(string filename) {
  array<char, 256> buf;
  string cmd("objdump -M intel --no-show-raw-insn -d " + filename);
  vector<InstrPtr> res;
  string disasm;

  auto pipe = popen(cmd.c_str(), "r");
  if (!pipe) {
    cout << "error in disasm" << endl;
    exit(-1);
  }

  while (fgets(buf.data(), 256, pipe) != NULL) {
    disasm += buf.data();
  }

  pclose(pipe);

  std::stringstream ss(disasm);
  string line;
  char useless[2] = {'#', '<'};
  while (getline(ss, line, '\n')) {
    for (int i = 0; i < 2; i++) {
      auto pos = line.find(useless[i]);
      if (pos == string::npos) continue;
      line = line.substr(0, pos);
    }

    line = refine(line);

    auto offset_pos = line.find(":\t");
    if (offset_pos == string::npos) continue;
    auto offset_str = line.substr(0, offset_pos);
    auto inst_str = line.substr(offset_pos + 2);
    char *endptr;
    unsigned long offset = strtol(offset_str.c_str(), &endptr, 16);

    auto opcode_pos = inst_str.find(" ");
    if (opcode_pos == string::npos) continue;
    auto opcode_str = inst_str.substr(0, opcode_pos);
    auto operand_str = inst_str.substr(opcode_pos + 1);

    auto inst_ptr = std::make_shared<Instruction>(offset, opcode_str, operand_str);
    inst_ptr->original_inst_ = line;
    res.push_back(inst_ptr);
  }

  return res;
}

bool is_interesting(const Operand &operand) {
  // const vector<string> regs_64 = {"rax", "rbx", "rsi", "rdi", "rdx", "rcx", "r8",
  //                               "r9", "r10", "r11", "r12", "r13", "r14", "r15", "rbp", "rsp"};
  for (const auto &each : operand.reg_list_) {
    if (each.first > REG_64_START && each.first < REG_64_END) {
      return each.first != REG_RIP;
    }
  }
  return false;
}

bool is_harmful(OPCODE opcode) {
  // const vector<string> harmful_instructions = {"syscall", "int3"};
  return opcode == OP_IMUL || opcode == OP_SYSCALL || opcode == OP_INT3 || opcode == OP_NONE;
}

bool is_unwanted_instructions(OPCODE opcode) {
  // const vector<string> unwant_instructions = {"nop", "sfence", "sar", "xor", "add", "sub", "mul",
  // "div", "ror", "bswap", "movaps", "movdqa", "movntdq", "shl", "shr"};
  const OPCODE unwanted[]
      = {OP_NOP, OP_SFENCE, OP_SAR,    OP_XOR,    OP_ADD,     OP_SUB, OP_MUL, OP_DIV,
         OP_ROR, OP_BSWAP,  OP_MOVAPS, OP_MOVDQA, OP_MOVNTDQ, OP_SHL, OP_SHR, OP_NONE};
  for (const auto &each : unwanted) {
    if (opcode == each) {
      return true;
    }
  }
  return false;
}

vector<SegmentPtr> get_call_segment(vector<InstrPtr> &insts) {
  int start = 0;
  const int size = insts.size();
  std::unordered_map<std::string, size_t> duplicate_helper = {};

  vector<SegmentPtr> result;

  for (int idx = 0; idx < size; idx++) {
    auto inst = insts[idx];
    if (inst->opcode_ == OPCODE::OP_CALL || inst->opcode_ == OPCODE::OP_JMP) {
      if (!inst->op_dst_.has_value() || is_interesting(inst->op_dst_.value()) == false
          || inst->op_dst_->contain_reg(REG_RIP)) {
        continue;
      }
      start = idx - 1;
      while (start >= 0) {
        auto tmp_inst = insts[start];
        if (tmp_inst->opcode_ == OP_CALL || tmp_inst->opcode_ == OP_JCC
            || tmp_inst->opcode_ == OP_RET || tmp_inst->opcode_ == OP_JMP
            || is_harmful(tmp_inst->opcode_)) {
          break;
        }
        start -= 1;
      }
      start += 1;
      while (start < idx + 1) {
        auto tmp_inst = insts[start];
        if (is_unwanted_instructions(tmp_inst->opcode_) || (tmp_inst->operation_length_ != kQWORD)
            || (tmp_inst->op_dst_
                && (tmp_inst->op_dst_->contain_reg(REG_CR3)
                    || tmp_inst->op_dst_->contain_reg(REG_CR4)
                    || tmp_inst->op_dst_->contain_reg(REG_RIP)
                    || tmp_inst->op_dst_->contain_segment_reg()))
            || (tmp_inst->op_src_
                && (tmp_inst->op_src_->contain_reg(REG_CR4)
                    || tmp_inst->op_src_->contain_reg(REG_CR4)
                    || tmp_inst->op_src_->contain_reg(REG_RIP)
                    || tmp_inst->op_src_->contain_segment_reg()))) {
          // remove this instruction
          // cout << "Removing: " << tmp_inst->original_inst_ << endl;
          start += 1;
          continue;
        }
        break;
      }

      // for (auto del = 0; del < start; del++)
      //     delete insts[del];

      vector<InstrPtr> inst_list;
      for (int tmp_idx = start; tmp_idx < idx + 1; tmp_idx++) inst_list.push_back(insts[tmp_idx]);

      if (inst_list.size() <= 1) {
        continue;
      }

      SegmentPtr seg = std::make_shared<Segment>(inst_list);

      std::string tmp_asm = seg->to_string(false);

      if (duplicate_helper.count(tmp_asm)) {  // contains
        // duplicate segment
        // delete seg;
        continue;
      }
      duplicate_helper[tmp_asm] = result.size();
      result.push_back(seg);
    }
  }
  return result;
}

unsigned long locate_next_inst_addr(unsigned long offset,
                                    const vector<pair<SegmentPtr, unsigned>> &code_segments) {
  /*InstrPtrres = NULL;
  bool is_find = false;
  for (auto inst : segment->inst_list_) {
      if (offset == inst->offset_) {
          is_find = true;
      }
      if (is_find == true) {
          res = inst;
          break;
      }
  }

  if (is_find == false) assert(0);
  return res;*/
  for (size_t i = 0; i < code_segments.size(); i++) {
    auto seg = code_segments[i].first;
    if (offset == seg->inst_list_.back()->offset_) {
      if (i == code_segments.size() - 1) {
        return -1;
      }
      return code_segments[i + 1].first->inst_list_[code_segments[i + 1].second]->offset_;
    }
  }
  assert(false && "This should not happen");
  return -1;
}

REG get_reg_by_str(const string &str) {
  static map<string, REG> m;
  static bool intialized = false;

  if (intialized == false) {
    // 64 bit
    m["rax"] = REG_RAX;
    m["rbx"] = REG_RBX;
    m["rcx"] = REG_RCX;
    m["rdx"] = REG_RDX;
    m["rdi"] = REG_RDI;
    m["rsi"] = REG_RSI;
    m["rsp"] = REG_RSP;
    m["rbp"] = REG_RBP;
    m["r8"] = REG_R8;
    m["r9"] = REG_R9;
    m["r10"] = REG_R10;
    m["r11"] = REG_R11;
    m["r12"] = REG_R12;
    m["r13"] = REG_R13;
    m["r14"] = REG_R14;
    m["r15"] = REG_R15;
    m["rip"] = REG_RIP;
    m["cr4"] = REG_CR4;
    m["cr3"] = REG_CR3;
    // 32 bit
    m["eax"] = REG_EAX;
    m["ebx"] = REG_EBX;
    m["ecx"] = REG_ECX;
    m["edx"] = REG_EDX;
    m["edi"] = REG_EDI;
    m["esi"] = REG_ESI;
    m["esp"] = REG_ESP;
    m["ebp"] = REG_EBP;
    m["r8d"] = REG_R8D;
    m["r9d"] = REG_R9D;
    m["r10d"] = REG_R10D;
    m["r11d"] = REG_R11D;
    m["r12d"] = REG_R12D;
    m["r13d"] = REG_R13D;
    m["r14d"] = REG_R14D;
    m["r15d"] = REG_R15D;
    m["eip"] = REG_EIP;

    // 16 bit
    m["ax"] = REG_AX;
    m["bx"] = REG_BX;
    m["cx"] = REG_CX;
    m["dx"] = REG_DX;
    m["di"] = REG_DI;
    m["si"] = REG_SI;
    m["sp"] = REG_SP;
    m["bp"] = REG_BP;
    m["r8w"] = REG_R8W;
    m["r9w"] = REG_R9W;
    m["r10w"] = REG_R10W;
    m["r11w"] = REG_R11W;
    m["r12w"] = REG_R12W;
    m["r13w"] = REG_R13W;
    m["r14w"] = REG_R14W;
    m["r15w"] = REG_R15W;
    m["ip"] = REG_IP;

    // 8 bit
    m["al"] = REG_AL;
    m["bl"] = REG_BL;
    m["cl"] = REG_CL;
    m["dl"] = REG_DL;
    m["dil"] = REG_DIL;
    m["sil"] = REG_SIL;
    m["spl"] = REG_SPL;
    m["bpl"] = REG_BPL;
    m["r8b"] = REG_R8B;
    m["r9b"] = REG_R9B;
    m["r10b"] = REG_R10B;
    m["r11b"] = REG_R11B;
    m["r12b"] = REG_R12B;
    m["r13b"] = REG_R13B;
    m["r14b"] = REG_R14B;
    m["r15b"] = REG_R15B;

    m["ah"] = REG_AH;
    m["bh"] = REG_BH;
    m["ch"] = REG_CH;
    m["dh"] = REG_DH;
    intialized = true;
  }

  if (m.find(str) != m.end()) return m[str];
  return REG_NONE;
}

string get_reg_str_by_reg(REG reg) {
  static map<REG, string> m;
  static bool intialized = false;

  if (intialized == false) {
    // 64 bit
    m[REG_RAX] = "rax";
    m[REG_RBX] = "rbx";
    m[REG_RCX] = "rcx";
    m[REG_RDX] = "rdx";
    m[REG_RDI] = "rdi";
    m[REG_RSI] = "rsi";
    m[REG_RSP] = "rsp";
    m[REG_RBP] = "rbp";
    m[REG_R8] = "r8";
    m[REG_R9] = "r9";
    m[REG_R10] = "r10";
    m[REG_R11] = "r11";
    m[REG_R12] = "r12";
    m[REG_R13] = "r13";
    m[REG_R14] = "r14";
    m[REG_R15] = "r15";
    m[REG_RIP] = "rip";
    m[REG_CR4] = "cr4";
    m[REG_CR3] = "cr3";
    // 32 bit
    m[REG_EAX] = "eax";
    m[REG_EBX] = "ebx";
    m[REG_ECX] = "ecx";
    m[REG_EDX] = "edx";
    m[REG_EDI] = "edi";
    m[REG_ESI] = "esi";
    m[REG_ESP] = "esp";
    m[REG_EBP] = "ebp";
    m[REG_R8D] = "r8d";
    m[REG_R9D] = "r9d";
    m[REG_R10D] = "r10d";
    m[REG_R11D] = "r11d";
    m[REG_R12D] = "r12d";
    m[REG_R13D] = "r13d";
    m[REG_R14D] = "r14d";
    m[REG_R15D] = "r15d";
    m[REG_EIP] = "eip";

    // 16 bit
    m[REG_AX] = "ax";
    m[REG_BX] = "bx";
    m[REG_CX] = "cx";
    m[REG_DX] = "dx";
    m[REG_DI] = "di";
    m[REG_SI] = "si";
    m[REG_SP] = "sp";
    m[REG_BP] = "bp";
    m[REG_R8W] = "r8w";
    m[REG_R9W] = "r9w";
    m[REG_R10W] = "r10w";
    m[REG_R11W] = "r11w";
    m[REG_R12W] = "r12w";
    m[REG_R13W] = "r13w";
    m[REG_R14W] = "r14w";
    m[REG_R15W] = "r15w";
    m[REG_IP] = "ip";

    // 8 bit
    m[REG_AL] = "al";
    m[REG_BL] = "bl";
    m[REG_CL] = "cl";
    m[REG_DL] = "dl";
    m[REG_DIL] = "dil";
    m[REG_SIL] = "sil";
    m[REG_SPL] = "spl";
    m[REG_BPL] = "bpl";
    m[REG_R8B] = "r8b";
    m[REG_R9B] = "r9b";
    m[REG_R10B] = "r10b";
    m[REG_R11B] = "r11b";
    m[REG_R12B] = "r12b";
    m[REG_R13B] = "r13b";
    m[REG_R14B] = "r14b";
    m[REG_R15B] = "r15b";

    m[REG_AH] = "ah";
    m[REG_BH] = "bh";
    m[REG_CH] = "ch";
    m[REG_DH] = "dh";
    intialized = true;
  }

  if (m.find(reg) != m.end()) return m[reg];
  return "None";
}

REG find_reg64(REG r) {
  while (r > REG_64_END) {
    r = (REG)((int)r - 19);
  }
  if (r <= REG_64_START) return REG_NONE;
  return r;
}

string Operand::transfer_operation_len_to_str(OPERATION_LENGTH length) const {
  string res;
  switch (length) {
    case kNONE:
      res = "0";
      break;
    case kBYTE:
      res = "1";
      break;
    case kWORD:
      res = "2";
      break;
    case kDWORD:
      res = "4";
      break;
    case kQWORD:
      res = "8";
      break;
    default:
      res = "Unexpected type";
      break;
  }
  return res;
}

string transfer_op_to_str(OPCODE opcode) {
  static map<OPCODE, string> m;
  static bool transfer_op_to_str_init = false;
  if (transfer_op_to_str_init == false) {
    m[OP_MOV] = "mov";
    m[OP_LEA] = "lea";
    m[OP_POP] = "pop";
    m[OP_ADD] = "add";
    m[OP_SUB] = "sub";
    m[OP_IMUL] = "imul";
    m[OP_MUL] = "mul";
    m[OP_DIV] = "div";
    m[OP_PUSH] = "push";
    m[OP_XOR] = "xor";
    m[OP_OR] = "or";
    m[OP_AND] = "and";
    m[OP_SHR] = "shr";
    m[OP_SHL] = "shl";
    m[OP_ROR] = "ror";
    m[OP_SAR] = "sar";
    m[OP_TEST] = "test";
    m[OP_NOP] = "nop";
    m[OP_CMP] = "cmp";
    m[OP_CALL] = "call";
    m[OP_JMP] = "jmp";
    m[OP_XCHG] = "xchg";
    m[OP_JCC] = "jcc";
    m[OP_RET] = "ret";
    m[OP_SYSCALL] = "syscall";
    m[OP_INT3] = "int3";
    m[OP_SFENCE] = "sfence";
    m[OP_BSWAP] = "bswap";
    m[OP_MOVAPS] = "movaps";
    m[OP_MOVDQA] = "movdqa";
    m[OP_MOVNTDQ] = "movntdq";
    transfer_op_to_str_init = true;
  }

  if (m.find(opcode) != m.end()) return m[opcode];
  return "None";
}

void Segment::print_inst() const {
  for (auto idx = useful_inst_index_; idx < inst_list_.size(); idx++) {
    cout << inst_list_[idx]->original_inst_ << endl;
  }
}

OPCODE transfer_str_to_op(string op_str) {
  static map<string, OPCODE> str_op_map;
  static bool str_op_inited = false;

  if (str_op_inited == false) {
    str_op_map["add"] = OP_ADD;
    str_op_map["or"] = OP_OR;
    str_op_map["adc"] = OP_ADD;
    str_op_map["sbb"] = OP_SUB;
    str_op_map["and"] = OP_AND;
    str_op_map["sub"] = OP_SUB;
    str_op_map["xor"] = OP_XOR;
    str_op_map["cmp"] = OP_CMP;
    str_op_map["push"] = OP_PUSH;
    str_op_map["pop"] = OP_POP;
    str_op_map["rex"] = OP_NONE;
    str_op_map["rex.b"] = OP_NONE;
    str_op_map["rex.x"] = OP_NONE;
    str_op_map["rex.xb"] = OP_NONE;
    str_op_map["rex.r"] = OP_NONE;
    str_op_map["rex.rb"] = OP_NONE;
    str_op_map["rex.rx"] = OP_NONE;
    str_op_map["rex.rxb"] = OP_NONE;
    str_op_map["rex.w"] = OP_NONE;
    str_op_map["rex.wb"] = OP_NONE;
    str_op_map["rex.wx"] = OP_NONE;
    str_op_map["rex.wxb"] = OP_NONE;
    str_op_map["rex.wr"] = OP_NONE;
    str_op_map["rex.wrb"] = OP_NONE;
    str_op_map["rex.wrx"] = OP_NONE;
    str_op_map["rex.wrxb"] = OP_NONE;
    str_op_map["movsxd"] = OP_MOVSXD;
    str_op_map["mul"] = OP_MUL;
    str_op_map["imul"] = OP_IMUL;
    str_op_map["ins"] = OP_NONE;
    str_op_map["insb"] = OP_NONE;
    str_op_map["insw"] = OP_NONE;
    str_op_map["insd"] = OP_NONE;
    str_op_map["outs"] = OP_NONE;
    str_op_map["outsd"] = OP_NONE;
    str_op_map["outsb"] = OP_NONE;
    str_op_map["outsw"] = OP_NONE;
    str_op_map["jo"] = OP_JCC;
    str_op_map["jno"] = OP_JCC;
    str_op_map["jb"] = OP_JCC;
    str_op_map["jnae"] = OP_JCC;
    str_op_map["jc"] = OP_JCC;
    str_op_map["jnb"] = OP_JCC;
    str_op_map["jae"] = OP_JCC;
    str_op_map["jnc"] = OP_JCC;
    str_op_map["jz"] = OP_JCC;
    str_op_map["je"] = OP_JCC;
    str_op_map["jnz"] = OP_JCC;
    str_op_map["jne"] = OP_JCC;
    str_op_map["jbe"] = OP_JCC;
    str_op_map["jna"] = OP_JCC;
    str_op_map["jnbe"] = OP_JCC;
    str_op_map["ja"] = OP_JCC;
    str_op_map["js"] = OP_JCC;
    str_op_map["jns"] = OP_JCC;
    str_op_map["jp"] = OP_JCC;
    str_op_map["jpe"] = OP_JCC;
    str_op_map["jnp"] = OP_JCC;
    str_op_map["jpo"] = OP_JCC;
    str_op_map["jl"] = OP_JCC;
    str_op_map["jnge"] = OP_JCC;
    str_op_map["jnl"] = OP_JCC;
    str_op_map["jge"] = OP_JCC;
    str_op_map["jle"] = OP_JCC;
    str_op_map["jng"] = OP_JCC;
    str_op_map["jnle"] = OP_JCC;
    str_op_map["jg"] = OP_JCC;
    str_op_map["test"] = OP_TEST;
    str_op_map["xchg"] = OP_XCHG;
    str_op_map["lea"] = OP_LEA;
    str_op_map["mov"] = OP_MOV;
    str_op_map["nop"] = OP_NOP;
    str_op_map["pause"] = OP_NONE;
    str_op_map["cbw"] = OP_NONE;
    str_op_map["cwde"] = OP_NONE;
    str_op_map["cdqe"] = OP_NONE;
    str_op_map["cwd"] = OP_NONE;
    str_op_map["cdq"] = OP_NONE;
    str_op_map["cqo"] = OP_NONE;
    str_op_map["fwait"] = OP_NONE;
    str_op_map["wait"] = OP_NONE;
    str_op_map["pushf"] = OP_NONE;
    str_op_map["pushfq"] = OP_NONE;
    str_op_map["popf"] = OP_NONE;
    str_op_map["popfq"] = OP_NONE;
    str_op_map["cbw"] = OP_NONE;
    str_op_map["sahf"] = OP_NONE;
    str_op_map["lahf"] = OP_NONE;
    str_op_map["cbw"] = OP_NONE;
    str_op_map["movs"] = OP_MOV;
    str_op_map["movsb"] = OP_NONE;
    str_op_map["movsw"] = OP_NONE;
    str_op_map["movsd"] = OP_NONE;
    str_op_map["movsq"] = OP_NONE;
    str_op_map["movs"] = OP_MOV;
    str_op_map["cmps"] = OP_CMP;
    str_op_map["cmpsb"] = OP_CMP;
    str_op_map["cmpsw"] = OP_CMP;
    str_op_map["cmpsd"] = OP_CMP;
    str_op_map["cmpsq"] = OP_CMP;
    str_op_map["stos"] = OP_NONE;
    str_op_map["stosb"] = OP_NONE;
    str_op_map["stosw"] = OP_NONE;
    str_op_map["stosd"] = OP_NONE;
    str_op_map["stosq"] = OP_NONE;
    str_op_map["lods"] = OP_NONE;
    str_op_map["lodsb"] = OP_NONE;
    str_op_map["lodsw"] = OP_NONE;
    str_op_map["lodsd"] = OP_NONE;
    str_op_map["lodsq"] = OP_NONE;
    str_op_map["scas"] = OP_NONE;
    str_op_map["scasw"] = OP_NONE;
    str_op_map["scasd"] = OP_NONE;
    str_op_map["scaq"] = OP_NONE;
    str_op_map["rol"] = OP_NONE;
    str_op_map["ror"] = OP_ROR;
    str_op_map["rcl"] = OP_NONE;
    str_op_map["rcr"] = OP_NONE;
    str_op_map["shl"] = OP_SHL;
    str_op_map["sal"] = OP_NONE;
    str_op_map["shr"] = OP_SHR;
    str_op_map["sar"] = OP_SAR;
    str_op_map["ret"] = OP_RET;
    str_op_map["retn"] = OP_RET;
    str_op_map["enter"] = OP_NONE;
    str_op_map["leave"] = OP_NONE;
    str_op_map["retf"] = OP_RET;
    str_op_map["int"] = OP_INT3;
    str_op_map["into"] = OP_INT3;
    str_op_map["iret"] = OP_RET;
    str_op_map["iretd"] = OP_RET;
    str_op_map["iretq"] = OP_RET;
    str_op_map["iretq"] = OP_RET;
    str_op_map["call"] = OP_CALL;
    str_op_map["jmp"] = OP_JMP;
    str_op_map["div"] = OP_DIV;
    str_op_map["xor"] = OP_XOR;
    str_op_map["syscall"] = OP_SYSCALL;
    str_op_map["sfence"] = OP_SFENCE;
    str_op_map["bswap"] = OP_BSWAP;
    str_op_map["movaps"] = OP_MOVAPS;
    str_op_map["movdqa"] = OP_MOVDQA;
    str_op_map["movntdq"] = OP_MOVNTDQ;
    str_op_inited = true;
  }
  if (str_op_map.find(op_str) != str_op_map.end()) return str_op_map[op_str];
  return OP_NONE;
}
