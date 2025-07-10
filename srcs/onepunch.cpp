#include "onepunch.h"

#include <algorithm>

long MEM_INF = 0x10000000;

// static const char *g_type_strings[] = {"CallValue", "MemValue", "CallRegValue", "OtherValue"};

map<unsigned long, vector<SegmentPtr>> Preprocessor::result_;
map<SegmentPtr, unsigned long> Preprocessor::test_;
vector<shared_ptr<Memory>> MEM_LIST;
bool RECORD_MEM = false;

void collect_input_output_regs(const SegmentPtr segment, set<REG> &input_regs,
                               set<REG> &output_regs);

static bool g_is_rsp_usable = true;
static bool g_is_rbp_usable = true;

static inline bool is_rsp_usable() { return g_is_rsp_usable; }

static inline bool is_rbp_usable() { return g_is_rbp_usable; }

static inline bool is_stack_frame_reg(REG r) { return r == REG_RSP || r == REG_RBP; }

static inline bool is_stack_frame_reg_usable(REG r) {
  if (r == REG_RSP)
    return g_is_rsp_usable;
  else if (r == REG_RBP)
    return g_is_rbp_usable;
  else {
    assert(false && "This should never happen");
    return false;
  }
}

static inline void set_stack_frame_reg(REG r, bool flag) {
  if (r == REG_RSP)
    g_is_rsp_usable = flag;
  else if (r == REG_RBP)
    g_is_rsp_usable = flag;
  else {
    assert(0);
  }
}

inline void set_rsp_usable(bool flag) { g_is_rsp_usable = flag; }

inline void set_rbp_usable(bool flag) { g_is_rbp_usable = flag; }

unsigned long compute_constraint(const SegmentPtr segment) {
  set<REG> input_regs, output_regs;
  collect_input_output_regs(segment, input_regs, output_regs);
  /*
  cout << "----" << endl;
  for(auto i: segment->inst_list_){
      cout << i->original_inst_ << endl;
  }
  cout << "Input: " << endl;
  for(auto r: input_regs){
      cout << get_reg_str_by_reg(r) << endl;
  }
  cout << "Output: " << endl;
  for(auto r: output_regs){
      cout << get_reg_str_by_reg(r) << endl;
  }
  cout <<"----" << endl;
  char a;
  */
  auto input_hash = hash_reg_list(input_regs);
  auto output_hash = hash_reg_list(output_regs);
  /*
  cout << std:: hex << "Input Hash: "<<  input_hash << endl;
  cout << std:: hex << "Output Hash: "<<  output_hash << endl;
  cout << "Total: " << res << endl;
  */
  auto res = (input_hash << 32) | output_hash;
  return res;
}

void Preprocessor::process(const vector<SegmentPtr> &segments) {
  for (const auto &i : segments) {
    auto res = compute_constraint(i);
    // cout << std::hex << res << endl;
    test_[i] = res;
  }
}

bool opcode_dst_control(OPCODE opcode) {
  static set<OPCODE> m_opcode = {OP_MOV, OP_LEA, OP_POP};
  return m_opcode.find(opcode) != m_opcode.end();
}

bool hash_match(unsigned long needed, unsigned long src) {
  unsigned int needed_input = needed >> 32;
  unsigned int needed_output = needed & 0xFFFFFFFF;
  if (((src & needed_output) ^ needed_output) == 0) {
    return false;
  }
  if (needed_input && !(needed_input & src)) {
    return false;
  }
  return true;
}

/*
bool opcode_src_control(OPCODE opcode){
    static set<OPCODE> m_opcode = {OP_MOV, OP_LEA, OP_POP};
    return m_opcode.find(opcode) != m_opcode.end();
}
*/

void collect_input_output_regs(const SegmentPtr segment, set<REG> &input_regs,
                               set<REG> &output_regs) {
  for (auto idx = segment->useful_inst_index_; idx != segment->inst_list_.size(); ++idx) {
    const auto &inst = segment->inst_list_[idx];
    const auto &op_src = inst->op_src_;
    const auto &op_dst = inst->op_dst_;

    if (op_src && op_src->is_dereference_ && op_src->reg_num_ == 1) {
      const auto &reg_src = op_src->get_reg_op();
      if (input_regs.find(reg_src) == input_regs.end()
          && output_regs.find(reg_src) == output_regs.end()) {
        input_regs.insert(reg_src);
      }
    }

    if (op_dst) {
      if (op_dst->is_dereference_ && op_dst->reg_num_ == 1) {
        const auto &reg_dst = op_dst->get_reg_op();
        if (input_regs.find(reg_dst) == input_regs.end()
            && output_regs.find(reg_dst) == output_regs.end()) {
          input_regs.insert(reg_dst);
        }
      } else if (opcode_dst_control(inst->opcode_) && op_dst->reg_num_ == 1
                 && inst->operation_length_ == kQWORD) {
        output_regs.insert(op_dst->get_reg_op());
      }
    }
  }
}

unsigned long hash_reg_list(const set<REG> &reg_list) {
  unsigned long res = 0;
  for (auto i : reg_list) {
    res |= (1 << ((unsigned long)i));
  }
  return res;
}

unsigned long hash_reg_list(const list<RegisterPtr> &reg_list) {
  unsigned long res = 0;
  for (auto i : reg_list) {
    res |= 1 << ((unsigned long)(i->name_));
  }
  return res;
}

void Memory::increase_ref_count() { this->ref_count_++; }

void Memory::decrease_ref_count() { this->ref_count_--; }

void Memory::set_content(long offset, const Value &val, OPERATION_LENGTH len) {
  (void)len;
  content_[offset] = val;
}

bool Memory::contain_range(const pair<long, long> &range) {
  for (auto &r : range_) {
    if (r.second >= range.second) {
      return r.first <= range.first;
    }
  }
  return false;
}

string Value::to_string() {
  string type_str, value_str;
  char tmp[20];
  if (this->type_ == kCallValue) {
    type_str = "CALL_VALUE";
    if (this->value_ == -1) {
      strcpy(tmp, "Target RIP");
    } else {
      sprintf(tmp, "0x%lx(inst)", this->value_);
    }
    value_str = tmp;
  } else if (this->type_ == kMemValue) {
    type_str = "MEM_VALUE";
    sprintf(tmp, "0x%lx(memid)", this->value_);
    value_str = tmp;
  } else if (this->type_ == kCallRegValue) {
    type_str = "CALL_REG_VALUE";
    if (this->value_ == -1) {
      strcpy(tmp, "Target RIP");
    } else if (this->value_ >> 32) {
      sprintf(tmp, "0x%lx(memid)", this->value_ >> 32);
    } else {
      sprintf(tmp, "0x%lx(inst)", this->value_);
    }
    value_str = tmp;
  } else if (this->type_ == kOtherValue) {
    type_str = "OTHER_VALUE";
    sprintf(tmp, "0x%lx", this->value_);
    value_str = tmp;
  }
  return type_str + "," + value_str;
}

Memory::Memory() {
  static unsigned long id = 0;
  mem_id_ = id++;
}

Memory::~Memory() {
  this->content_.clear();
  this->range_.clear();
}

bool Memory::remove_range(const pair<long, long> &range) {
  for (auto start = range_.begin(); start != range_.end(); start++) {
    if (start->second >= range.second) {
      if (start->first > range.first) return false;
      auto saved = start->first;
      start->first = range.second;
      range_.insert(start, make_pair(saved, range.first));
      return true;
    }
  }
  return false;
}

string Memory::to_string() {
  string info_range = "Available:[ ";
  for (const auto &each : this->range_) {
    if (each.first == each.second) continue;
    char left[20];
    if (each.first == -MEM_INF) {
      strcpy(left, "-INF");
    } else {
      sprintf(left, "%s0x%lx", each.first < 0 ? "-" : "",
              each.first < 0 ? -each.first : each.first);
    }
    char right[20];
    if (each.second == MEM_INF) {
      strcpy(right, "INF");
    } else {
      sprintf(right, "%s0x%lx", each.second < 0 ? "-" : "",
              each.second < 0 ? -each.second : each.second);
    }
    auto tmp_range = string_format("[%s,%s], ", left, right);
    info_range += tmp_range;
  }
  info_range.pop_back();
  info_range.pop_back();
  info_range += " ]";

  string info_content = "content:[ ";
  for (auto &kv : this->content_) {
    auto tmp_content
        = string_format("[%s0x%lx:(%s)], ", kv.first < 0 ? "-" : "",
                        kv.first < 0 ? -kv.first : kv.first, kv.second.to_string().c_str());
    info_content += tmp_content;
  }

  string result;
  char memid[10];
  sprintf(memid, "0x%x", this->mem_id_);
  result = "memid:";
  result += memid;
  result += ", relation: " + this->get_input_relation();
  if (this->content_.size()) {
    info_content.pop_back();
    info_content.pop_back();
    info_content += " ]";
    result += "\n\t" + info_range + "\n\t" + info_content;
  } else {
    result += "\n\t" + info_range;
  }

  return result;
}

Register::Register(bool alloc_mem) {
  this->base_offset_ = 0;
  this->input_src_ = "";
  this->input_action_ = false;
  this->input_offset_ = 0;
  if (alloc_mem) {
    this->mem_ = make_shared<Memory>();
    if (RECORD_MEM) MEM_LIST.push_back(this->mem_);
    this->mem_->ref_count_ = 1;
    this->mem_->range_.push_back(make_pair(-MEM_INF, MEM_INF));
  }
}

Register::Register(RegisterPtr reg) {
  this->name_ = reg->name_;
  this->base_offset_ = reg->base_offset_;
  this->input_src_ = reg->input_src_;
  this->input_offset_ = reg->input_offset_;
  this->input_action_ = reg->input_action_;
  this->mem_ = make_shared<Memory>();
  if (RECORD_MEM) MEM_LIST.push_back(this->mem_);
  this->mem_->ref_count_ = reg->mem_->ref_count_;
  this->mem_->mem_id_ = reg->mem_->mem_id_;
  this->mem_->range_ = reg->mem_->range_;
  this->mem_->content_ = reg->mem_->content_;
}

void Register::alias(const RegisterPtr reg, bool copy_mem) {
  if (copy_mem) {
    this->mem_ = reg->mem_;
  }
  this->base_offset_ = reg->base_offset_;
  this->input_src_ = reg->input_src_;
  this->input_offset_ = reg->input_offset_;
  this->input_action_ = reg->input_action_;
  this->mem_->increase_ref_count();
}

bool Register::contain_range(const pair<long, long> &range) {
  return this->mem_->contain_range(
      make_pair(range.first + base_offset_, range.second + base_offset_));
}

bool Register::remove_range(const pair<long, long> &range) {
  return this->mem_->remove_range(
      make_pair(range.first + base_offset_, range.second + base_offset_));
}

void Register::set_content(long offset, const Value &val, OPERATION_LENGTH len) {
  this->mem_->set_content(offset + base_offset_, val, len);
}

void Register::print() {
  cout << "name: " << get_reg_str_by_reg(name_) << endl;
  for (auto &i : mem_->range_) {
    cout << "(" << i.first + base_offset_ << ", " << i.second + base_offset_ << ")" << endl;
  }
}

string Register::to_string() {
  return string_format("%s:\t%s", get_reg_str_by_reg(this->name_).c_str(),
                       this->mem_->to_string().c_str());
}

string Register::get_input_relation() const {
  string relation;
  if (this->input_offset_ == 0) {
    relation = string_format("%s%s", this->input_action_ ? "*" : "", this->input_src_.c_str());
  } else {
    relation = string_format(this->input_action_ ? "*(%s%s0x%lx)" : "%s%s0x%lx",
                             this->input_src_.c_str(), this->input_offset_ < 0 ? "-" : "+",
                             this->input_offset_ < 0 ? -this->input_offset_ : this->input_offset_);
  }
  return relation;
}

void Register::set_input_relation(const RegisterPtr reg, long offset, bool action) {
  this->input_src_ = reg->get_input_relation();
  this->input_action_ = action;
  this->input_offset_ = offset;
  this->mem_->set_input_relation(reg, offset, action);
}

string Memory::get_input_relation() const {
  string relation;
  if (this->input_offset_ == 0) {
    relation = string_format("%s%s", this->input_action_ ? "*" : "", this->input_src_.c_str());
  } else {
    relation = string_format(this->input_action_ ? "*(%s%s0x%lx)" : "%s%s0x%lx",
                             this->input_src_.c_str(), this->input_offset_ < 0 ? "-" : "+",
                             this->input_offset_ < 0 ? -this->input_offset_ : this->input_offset_);
  }
  return relation;
}

void Memory::set_input_relation(const RegisterPtr reg, long offset, bool action) {
  this->input_src_ = reg->get_input_relation();
  this->input_action_ = action;
  this->input_offset_ = offset;
}

bool is_in_input(REG reg, const list<RegisterPtr> &reg_list) {
  for (auto &iter : reg_list) {
    if (iter->name_ == reg) return true;
  }
  return false;
}

unsigned remove_useless_instructions(SegmentPtr inst_list, const list<RegisterPtr> &reg_list) {
  unsigned int index = inst_list->useful_inst_index_;
  auto &insts = inst_list->inst_list_;
  unsigned int siz = insts.size();

  while (index < siz) {
    auto inst = insts[index];
    if (inst->operation_length_ != kQWORD || inst->opcode_ == OP_PUSH) {
      index++;
      continue;
    }

    if (inst->operand_num_ == 2) {
      assert(inst->op_src_);
      assert(inst->op_dst_);
      if (inst->op_src_->reg_num_ == 0) {
        index++;
        continue;
      }
      if (inst->op_dst_->is_dereference_) {
        if (inst->op_dst_->reg_num_ != 1) {
          index++;
          continue;
        }
        if (is_in_input(inst->op_dst_->get_reg_op(), reg_list) == false) {
          index++;
          continue;
        }
      }
      if (inst->op_src_->reg_num_ == 1 && inst->op_src_->reg_list_[0].second == 1) {
        if (is_in_input(inst->op_src_->reg_list_[0].first, reg_list)) {
          break;
        }
      }
    } else {
      // cout << inst->original_inst_ << endl;
      assert(inst->op_dst_);
      if (inst->op_dst_->reg_num_ == 0) {
        index++;
        continue;
      }
      if (inst->op_dst_->reg_num_ == 1 && inst->op_dst_->reg_list_[0].second == 1) {
        if (is_in_input(inst->op_dst_->reg_list_[0].first, reg_list)) {
          break;
        }
      }
    }
    index++;
  }
  inst_list->useful_inst_index_ = index;
  return index;
}

RegisterPtr get_reg_by_idx(REG reg, const list<RegisterPtr> &reg_list) {
  for (auto &iter : reg_list) {
    if (iter->name_ == reg) return iter;
  }
  return nullptr;
}

void remove_reg(RegisterPtr reg_to_remove, list<RegisterPtr> &reg_list) {
  reg_list.remove(reg_to_remove);
}

void remove_reg_by_idx(REG reg, list<RegisterPtr> &reg_list) {
  for (auto iter = reg_list.begin(); iter != reg_list.end(); iter++) {
    assert((*iter) != nullptr);
    if ((*iter)->name_ == reg) {
      reg_list.erase(iter);
      return;
    }
  }
  // reg_list.remove_if([](Register* n){return n->name_ == reg;});
}

void remove_reg_and_alias(RegisterPtr reg_to_remove, list<RegisterPtr> &reg_list) {
  shared_ptr<Memory> tmp = reg_to_remove->mem_;
  for (auto &iter : reg_list) {
    if (iter->mem_ == tmp) {
      reg_list.remove(iter);
      return;
    }
  }
  // reg_list.remove_if([](Register* n){return n->mem_ == tmp;});
}

RegisterPtr make_alias(REG alias_reg_name, RegisterPtr reg, bool copy_mem) {
  auto new_reg = std::make_shared<Register>(false);
  new_reg->name_ = alias_reg_name;
  new_reg->alias(reg, copy_mem);
  return new_reg;
}

RegisterPtr get_reg_by_relation(const string &relation, const list<RegisterPtr> &reg_list) {
  // cout << "offset=" << offset << ", inst:" << inst_str << endl;
  for (auto reg : reg_list) {
    if (reg->get_input_relation() == relation) {
      return reg;
    }
  }

  return nullptr;
}

bool contain_uncontrol_memory_access(const InstrPtr inst, const list<RegisterPtr> &reg_list) {
  unsigned op_num = inst->operand_num_;

  if (op_num == 0 || inst->opcode_ == OP_LEA || inst->opcode_ == OP_NOP) return false;
  auto operand = inst->op_dst_;
  if (operand->is_dereference_ == false) {
    if (!inst->op_src_.has_value() || inst->op_src_->is_dereference_ == false) return false;
    operand = inst->op_src_;
  }
  if (operand->contain_segment_reg()) return false;
  for (auto &p : operand->reg_list_) {
    if (p.second != 1) return true;
    if (is_in_input(p.first, reg_list) == false) {
      if (p.first == REG_RSP || p.first == REG_RIP || p.first == REG_RBP) continue;
      return true;
    }
  }
  return false;
}

list<RegisterPtr> prepare_reg_list(const vector<REG> &reg_name_list) {
  list<RegisterPtr> List;
  for (auto &inter : reg_name_list) {
    RegisterPtr reg = std::make_shared<Register>();
    reg->name_ = inter;
    // reg->mem_ = make_shared<Memory>();
    // if(RECORD_MEM)
    // MEM_LIST.push_back(reg->mem_);
    // reg->mem_->range_.push_back(make_pair(-MEM_INF, MEM_INF));
    reg->input_src_ = get_reg_str_by_reg(inter);
    reg->input_action_ = false;
    reg->mem_->set_input_relation(reg, 0, false);
    List.push_back(reg);
  }
  return List;
}

bool xchg_handler(const InstrPtr inst, list<RegisterPtr> &reg_list, bool record_flag) {
  auto op_src = inst->op_src_;
  auto op_dst = inst->op_dst_;
  assert(op_src && op_dst);

  if (inst->operation_length_ != kQWORD) {
    if (op_src->is_dereference_ == false && op_dst->is_dereference_ == false) {
      // xchg reg, reg
      assert(op_src->reg_num_ == 1 && op_dst->reg_num_ == 1);
      auto reg_src = find_reg64(op_src->get_reg_op());
      auto reg_dst = find_reg64(op_dst->get_reg_op());

      remove_reg_by_idx(reg_src, reg_list);
      remove_reg_by_idx(reg_dst, reg_list);
      return true;
    }

    if (op_dst->is_dereference_) {
      swap(op_src, op_dst);
    }

    if (op_src->reg_num_ != 1) return false;
    assert(op_dst->reg_num_ == 1);
    auto src_range = op_src->get_used_range();
    auto src_ptr = get_reg_by_idx(src_range.first, reg_list);
    if (src_ptr == nullptr || src_ptr->contain_range(src_range.second) == false) return false;
    src_ptr->remove_range(src_range.second);

    auto reg_dst = op_dst->get_reg_op();
    remove_reg_by_idx(reg_dst, reg_list);
    return true;
  }

  if (op_src->is_dereference_ == false && op_dst->is_dereference_ == false) {
    // xchg reg, reg
    assert(op_src->reg_num_ == 1 && op_dst->reg_num_ == 1);
    auto reg_src = op_src->get_reg_op();
    auto reg_dst = op_dst->get_reg_op();

    if (reg_src == reg_dst) return true;
    auto src_ptr = get_reg_by_idx(reg_src, reg_list);
    auto dst_ptr = get_reg_by_idx(reg_dst, reg_list);
    if (src_ptr) {
      src_ptr->name_ = reg_dst;
    }
    if (dst_ptr) {
      dst_ptr->name_ = reg_src;
    }
    return true;
  }

  if (op_dst->is_dereference_) {
    swap(op_src, op_dst);
  }

  if (op_src->reg_num_ != 1) return false;
  assert(op_dst->reg_num_ == 1);
  auto src_range = op_src->get_used_range();
  auto src_ptr = get_reg_by_idx(src_range.first, reg_list);
  if (src_ptr == nullptr || src_ptr->contain_range(src_range.second) == false) return false;
  src_ptr->remove_range(src_range.second);

  auto reg_dst = op_dst->get_reg_op();
  remove_reg_by_idx(reg_dst, reg_list);
  auto new_reg = std::make_shared<Register>();
  new_reg->name_ = reg_dst;
  if (record_flag) {
    new_reg->set_input_relation(src_ptr, src_range.second.first, true);
  }
  reg_list.push_back(new_reg);
  return true;
}

bool mov_handler(const InstrPtr inst, list<RegisterPtr> &reg_list, bool record_flag) {
  auto op_dst = inst->op_dst_;
  auto op_src = inst->op_src_;

  if (inst->operation_length_ != kQWORD) {
    if (op_dst->is_dereference_) {
      if (op_dst->reg_num_ != 1) return false;
      auto reg_dst = op_dst->get_reg_op();
      if (is_in_input(reg_dst, reg_list) == false) {
        return false;
      }
      auto range = op_dst->get_used_range();
      auto dst_reg_ptr = get_reg_by_idx(range.first, reg_list);
      dst_reg_ptr->remove_range(range.second);
      return true;
    } else {
      if (op_dst->contain_segment_reg()) return true;
      auto t_reg = op_dst->reg_list_[0].first;
      auto reg64 = find_reg64(t_reg);
      // assert(is_in_input(t_reg, reg_list));
      remove_reg_by_idx(reg64, reg_list);
      return true;
    }
  }

  if (op_dst->is_dereference_ == false && op_src->is_dereference_ == false) {
    if (op_dst->contain_segment_reg()) return true;
    auto reg_dst = op_dst->get_reg_op();
    if (op_src->reg_num_ == 0) {
      // mov reg, imm
      if (is_stack_frame_reg(reg_dst)) {
        set_stack_frame_reg(reg_dst, false);
      }
      remove_reg_by_idx(reg_dst, reg_list);
      return true;
    }

    // mov reg, reg
    auto reg_src = op_src->reg_list_[0].first;
    if (reg_src == reg_dst) return true;
    remove_reg_by_idx(reg_dst, reg_list);
    if (is_in_input(reg_src, reg_list)) {
      auto new_reg = make_alias(reg_dst, get_reg_by_idx(reg_src, reg_list), true);
      reg_list.push_back(new_reg);
    } else {
      if (is_stack_frame_reg(reg_dst)) set_stack_frame_reg(reg_dst, false);
    }
  } else if (op_src->is_dereference_) {
    // mov reg, []
    auto reg_dst = op_dst->reg_list_[0].first;
    auto reg_src = op_src->reg_list_[0].first;

    if (reg_src != reg_dst) remove_reg_by_idx(reg_dst, reg_list);

    if (op_src->contain_segment_reg()) return true;
    if (is_in_input(reg_src, reg_list) == false) {
      bool tmp_res = false;
      if (is_stack_frame_reg(reg_src))
        tmp_res = is_stack_frame_reg_usable(reg_src);
      else if (reg_src == REG_RIP) {
        tmp_res = true;
      }
      if (is_stack_frame_reg(reg_dst)) set_stack_frame_reg(reg_dst, false);
      return tmp_res;
    }

    if (op_src->reg_list_.size() != 1) {
      return false;  // didn't handle two registers deref right now.
    }

    auto range = op_src->get_used_range();
    auto reg_ptr = get_reg_by_idx(reg_src, reg_list);
    if (reg_ptr->contain_range(range.second)) {
      reg_ptr->remove_range(range.second);
      auto new_reg = std::make_shared<Register>();
      new_reg->name_ = reg_dst;
      new_reg->set_input_relation(reg_ptr, op_src->imm_, true);

      if (reg_src == reg_dst) remove_reg_by_idx(reg_dst, reg_list);
      reg_list.push_back(new_reg);

      if (record_flag == true) {
        reg_ptr->set_content(op_src->imm_, move(Value(kMemValue, new_reg->mem_->mem_id_)));
      }
    } else {
      if (reg_src == reg_dst) remove_reg_by_idx(reg_dst, reg_list);
      return false;
    }
  } else {
    // mov [], reg
    assert(op_dst->is_dereference_);
    if (op_dst->contain_segment_reg()) return true;
    if (op_dst->reg_num_ != 1) return false;

    auto range = op_dst->get_used_range();
    if (is_in_input(range.first, reg_list) == false) {
      if (range.first == REG_RIP) return true;
      return is_stack_frame_reg(range.first) && is_stack_frame_reg_usable(range.first);
    }
    auto reg_dst = get_reg_by_idx(range.first, reg_list);
    if (reg_dst->contain_range(range.second) == false) return false;
    reg_dst->remove_range(range.second);
  }

  return true;
}

bool is_alias(REG reg, const list<RegisterPtr> &reg_list) {
  auto reg_ptr = get_reg_by_idx(reg, reg_list);
  if (reg_ptr == nullptr) return false;
  if (reg_ptr->mem_->ref_count_ > 1) return true;
  return false;
}

bool lea_handler(const InstrPtr inst, list<RegisterPtr> &reg_list) {
  auto op_dst = inst->op_dst_;
  auto op_src = inst->op_src_;

  assert(op_dst->reg_num_ == 1);
  auto reg_dst = op_dst->reg_list_[0].first;
  reg_dst = find_reg64(reg_dst);

  if (inst->operation_length_ != kQWORD || op_src->reg_num_ != 1) {
    remove_reg_by_idx(reg_dst, reg_list);
    return true;
  }

  auto range = op_src->get_used_range();
  if (is_in_input(range.first, reg_list) == false) {
    remove_reg_by_idx(reg_dst, reg_list);
    return true;
  }

  auto reg_alias = get_reg_by_idx(range.first, reg_list);
  assert(reg_alias);
  auto new_reg = make_alias(reg_dst, reg_alias);
  new_reg->base_offset_ = reg_alias->base_offset_ + range.second.first;
  new_reg->set_input_relation(reg_alias, range.second.first, false);
  remove_reg_by_idx(reg_dst, reg_list);
  reg_list.push_back(new_reg);
  return true;
}

bool pop_handler(const InstrPtr inst, list<RegisterPtr> &reg_list, bool record_flag) {
  auto op_dst = inst->op_dst_;
  assert(op_dst->reg_num_ == 1);
  auto reg_dst = op_dst->reg_list_[0].first;

  if (inst->operation_length_ != kQWORD) {
    reg_dst = find_reg64(reg_dst);
    remove_reg_by_idx(reg_dst, reg_list);
    return true;
  }

  if (REG_RSP != reg_dst) remove_reg_by_idx(reg_dst, reg_list);
  if (is_in_input(REG_RSP, reg_list)) {
    auto rsp = get_reg_by_idx(REG_RSP, reg_list);
    if (rsp->contain_range(make_pair(0, 8))) {
      auto new_reg = std::make_shared<Register>();
      new_reg->name_ = reg_dst;
      new_reg->set_input_relation(rsp, 0, true);

      if (record_flag) {
        rsp->set_content(0, Value(kMemValue, new_reg->mem_->mem_id_));
      }

      rsp->remove_range(make_pair(0, 8));
      rsp->base_offset_ += 8;
      rsp->set_input_relation(rsp, 8, false);
      if (reg_dst == REG_RSP) remove_reg_by_idx(REG_RSP, reg_list);
      reg_list.push_back(new_reg);
    }
  } else {
    remove_reg_by_idx(reg_dst, reg_list);
    if (REG_RSP == reg_dst) return false;
  }
  return true;
}

bool add_sub_handler(const InstrPtr inst, list<RegisterPtr> &reg_list) {
  auto op_dst = inst->op_dst_;
  auto op_src = inst->op_src_;

  if (op_dst->is_dereference_ == false) {
    assert(op_dst->reg_num_ == 1);
    auto reg_dst = op_dst->reg_list_[0].first;
    auto is_bh = false;
    if (inst->operation_length_ != kQWORD) {
      if (reg_dst > REG_8H_START && reg_dst < REG_8H_END) is_bh = true;
      reg_dst = find_reg64(reg_dst);
    }
    auto reg_dst_ptr = get_reg_by_idx(reg_dst, reg_list);
    if (!reg_dst_ptr) return true;

    if (op_src->reg_num_ == 0) {
      // add/sub reg, imm
      long imm = op_src->imm_;
      if (inst->opcode_ == OP_SUB) {
        imm = -imm;
      }

      reg_dst_ptr->base_offset_ += is_bh == true ? (imm * 0x100) : imm;
      reg_dst_ptr->set_input_relation(reg_dst_ptr, imm, false);
    } else if (op_src->is_dereference_ == false) {
      // add/sub reg, reg
      remove_reg_by_idx(reg_dst, reg_list);
    } else {
      // add/sub reg, []
      if (op_src->reg_num_ != 1) return false;
      if (op_src->contain_segment_reg()) {
        remove_reg_by_idx(reg_dst, reg_list);
        return true;
      }
      auto range = op_src->get_used_range();
      auto reg_src_ptr = get_reg_by_idx(range.first, reg_list);
      if (reg_src_ptr == nullptr) return false;
      if (reg_src_ptr->contain_range(range.second) == false) {
        remove_reg_by_idx(reg_dst, reg_list);
      } else {
        assert(reg_src_ptr != nullptr);
        reg_src_ptr->remove_range(range.second);
        reg_src_ptr->set_content(range.second.first, Value(kImmValue, 0), inst->operation_length_);
      }
    }
  } else {
    if (op_dst->reg_num_ != 1) return false;
    auto reg_dst = op_dst->get_reg_op();
    auto reg_dst_ptr = get_reg_by_idx(reg_dst, reg_list);
    if (reg_dst_ptr == nullptr) return false;
    auto range = op_dst->get_used_range();
    reg_dst_ptr->remove_range(range.second);
    return true;
  }
  return true;
}

bool push_handler(const InstrPtr inst, list<RegisterPtr> &reg_list) {
  auto rsp_ptr = get_reg_by_idx(REG_RSP, reg_list);
  if (rsp_ptr == nullptr) return true;

  auto range = inst->op_dst_.value().get_used_range();
  if (rsp_ptr->contain_range(range.second) == false) return false;
  rsp_ptr->base_offset_ -= 8;
  rsp_ptr->set_input_relation(rsp_ptr, -8, false);
  return true;
}

bool bitwise_handler(const InstrPtr inst, list<RegisterPtr> &reg_list) {
  auto op_dst = inst->op_dst_;

  if (op_dst->is_dereference_) {
    if (op_dst->reg_num_ != 1) return false;
    auto range = op_dst->get_used_range();
    auto reg_ptr = get_reg_by_idx(range.first, reg_list);
    if (reg_ptr == nullptr) return false;

    reg_ptr->remove_range(range.second);
  } else {
    assert(op_dst->reg_num_ == 1);
    auto reg_dst = op_dst->reg_list_[0].first;
    if (inst->operation_length_ != kQWORD) {
      reg_dst = find_reg64(reg_dst);
    }
    remove_reg_by_idx(reg_dst, reg_list);
  }
  return true;
}

bool branch_handler(const InstrPtr inst, list<RegisterPtr> &reg_list, bool record_flag) {
  auto op_dst = inst->op_dst_;

  if (op_dst->is_dereference_) {
    // call/jmp []

    if (op_dst->reg_num_ != 1) {
      for (auto &reg : op_dst->reg_list_) {
        // could improve
        if (is_independent(reg.first, reg_list) == false) return false;
      }
      auto second_reg = op_dst->reg_list_[1].first;
      auto second_reg_ptr = get_reg_by_idx(second_reg, reg_list);

      if (record_flag == true) {
        second_reg_ptr->set_content(-1, Value(kImmValue, 0xdeadbeef));
      }
      remove_reg_by_idx(second_reg, reg_list);
    } else {
      if (op_dst->reg_list_[0].second != 1) return false;

      auto range = op_dst->get_used_range();
      auto dst_reg_ptr = get_reg_by_idx(range.first, reg_list);
      if (dst_reg_ptr == nullptr) return false;

      assert(range.second.second - range.second.first == 8);
      if (dst_reg_ptr->contain_range(range.second) == false) return false;

      dst_reg_ptr->remove_range(range.second);

      assert(dst_reg_ptr->contain_range(range.second) == false);

      if (record_flag == true) {
        dst_reg_ptr->set_content(range.second.first, Value(kCallValue, inst->offset_));
      }
    }
  } else {
    // call/jmp reg
    auto reg = op_dst->reg_list_[0].first;
    if (is_independent(reg, reg_list) == false) return false;

    if (record_flag) {
      auto reg_ptr = get_reg_by_idx(reg, reg_list);
      reg_ptr->set_content(-1, Value(kCallRegValue, ((unsigned long)(reg_ptr->mem_->mem_id_) << 32)
                                                        + inst->offset_));
    }
    remove_reg_by_idx(reg, reg_list);
  }

  auto rsp_ptr = get_reg_by_idx(REG_RSP, reg_list);
  if (rsp_ptr != nullptr && inst->opcode_ == OP_CALL) {
    auto tr = make_pair(-8, 0);
    rsp_ptr->remove_range(tr);
    rsp_ptr->base_offset_ -= 8;
    rsp_ptr->set_input_relation(rsp_ptr, -8, false);
  }
  return true;
}

bool execute_one_instruction(const InstrPtr inst, list<RegisterPtr> &reg_list, bool record_flag) {
  if (contain_uncontrol_memory_access(inst, reg_list)) {
    return false;
  }
  bool flag = true;
  switch (inst->opcode_) {
    case OP_MOV:
    case OP_MOVSXD:
      flag = mov_handler(inst, reg_list, record_flag);
      break;
    case OP_LEA:
      flag = lea_handler(inst, reg_list);
      break;
    case OP_POP:
      flag = pop_handler(inst, reg_list, record_flag);
      break;
    case OP_PUSH:
      flag = push_handler(inst, reg_list);
      break;
    case OP_ADD:
    case OP_SUB:
      flag = add_sub_handler(inst, reg_list);
      break;
    case OP_XOR:
    case OP_AND:
    case OP_OR:
    case OP_SHR:
    case OP_ROR:
    case OP_SAR:
    case OP_SHL:
      flag = bitwise_handler(inst, reg_list);
      break;
    case OP_TEST:
    case OP_CMP:
    case OP_NOP:
      break;
    case OP_CALL:
    case OP_JMP:
      flag = branch_handler(inst, reg_list, record_flag);
      break;
    case OP_XCHG:
      flag = xchg_handler(inst, reg_list, record_flag);
      break;
    case OP_BSWAP:
      // flag = bswap_handler(inst,)
    default:
      // cout << inst->original_inst_ << endl;
      // assert(0);
      flag = false;
  }
  return flag;
}

bool execute_instructions(const SegmentPtr instructions, list<RegisterPtr> &reg_list,
                          bool record_flag) {
  unsigned start_idx = instructions->useful_inst_index_;
  for (; start_idx < instructions->inst_list_.size(); start_idx++) {
    if (execute_one_instruction(instructions->inst_list_[start_idx], reg_list, record_flag)
        == false) {
      if (record_flag)
        cout << "Inst: " << instructions->inst_list_[start_idx]->original_inst_ << endl;
      return false;
    }
    if (reg_list.empty()) {
      // cout << "empty" << endl;
      return false;
    }
  }
  return true;
}

set<unsigned long> g_visited;

bool dfs(vector<SegmentPtr> &code_segments, const vector<pair<REG, int>> &must_control_list,
         const list<RegisterPtr> &reg_list, list<RegisterPtr> &output_register,
         vector<pair<SegmentPtr, unsigned>> &output_segments, unsigned long search_level) {
  // If the same reg list (regardless of memory) has been explored, skip
  auto tmp_h = hash_reg_list(reg_list);
  if (search_level == 1) {
    if (g_visited.find(tmp_h) != g_visited.end()) {
      return false;
    }
    g_visited.insert(tmp_h);
  }

  auto save_rsp_usable = is_rsp_usable();
  auto save_rbp_usable = is_rbp_usable();

  for (auto segment : code_segments) {
    if (search_level <= 2 && hash_match(Preprocessor::test_[segment], tmp_h) == false) continue;

    segment->useful_inst_index_ = 0;
    unsigned start_index = remove_useless_instructions(segment, reg_list);
    if (segment->inst_list_.size() - segment->useful_inst_index_ < 2) {
      continue;
    }
    if (search_level <= 2 && hash_match(compute_constraint(segment), tmp_h) == false) continue;
    auto tmp_reg_list = copy_reg_list(reg_list);

    set_rsp_usable(save_rsp_usable);
    set_rbp_usable(save_rbp_usable);

    if (execute_instructions(segment, tmp_reg_list, false) == false) {
      if (0) {
        cout << "Segment: " << endl;
        segment->print_inst();
        cout << "Register: " << endl;
        for (auto i : reg_list) {
          i->print();
        }
      }
      delete_reg_list(tmp_reg_list);
      continue;
    }

    assert(tmp_reg_list.size() < 17);

    /*
    check solution before check size. This might increase a little overhead but can catch more
    solutions.
    */
    if (is_solution(must_control_list, tmp_reg_list)) {
      output_segments.push_back(make_pair(segment, start_index));
      output_register = move(tmp_reg_list);
      return true;
    }

    if (tmp_reg_list.size() > reg_list.size()) {
      output_segments.push_back(make_pair(segment, start_index));

      bool flag_dfs = dfs(code_segments, must_control_list, tmp_reg_list, output_register,
                          output_segments, search_level);

      if (flag_dfs) {
        delete_reg_list(tmp_reg_list);
        return true;
      }
      output_segments.pop_back();
    }
    delete_reg_list(tmp_reg_list);
  }
  return false;
}

bool is_independent(REG reg, const list<RegisterPtr> &reg_list) {
  if (is_alias(reg, reg_list)) return false;
  RegisterPtr regptr = get_reg_by_idx(reg, reg_list);
  if (regptr == nullptr) return false;
  if (regptr->mem_->range_.size() == 1) return true;
  return false;
}

bool is_solution(const vector<pair<REG, int>> &must_control_list,
                 const list<RegisterPtr> &reg_list) {
  bool flag = true;

  for (auto &reg : must_control_list) {
    if (is_in_input(reg.first, reg_list) == false) {
      flag = false;
      break;
    }
    // cout << "Reg: " << get_reg_str_by_reg(reg.first) << endl;
    if (reg.second == 1 && is_independent(reg.first, reg_list) == false) {
      flag = false;
      break;
    }
  }
  return flag;
}

void record_memory(const vector<REG> &reg_name_list,
                   vector<pair<SegmentPtr, unsigned>> &code_segments,
                   const vector<pair<REG, int>> &must_control_list) {
  RECORD_MEM = true;
  list<RegisterPtr> reg_list = prepare_reg_list(reg_name_list);
  for (auto &iter : code_segments) {
    iter.first->useful_inst_index_ = iter.second;
    execute_instructions(iter.first, reg_list, true);
  }
  RECORD_MEM = false;

  match_and_print(MEM_LIST, code_segments, must_control_list, reg_list);

  cout << endl << "Memory list:" << endl;

  for (auto &iter : MEM_LIST) {
    std::cout << iter->to_string() << endl;
  }
  cout << endl << "Final state" << endl;
  for (auto &iter : reg_list) {
    std::cout << iter->to_string() << endl;
  }

  for (auto &iter : MEM_LIST) {
    iter.reset();
  }
}

bool minimize_segment(list<RegisterPtr> &sol_register,
                      vector<pair<SegmentPtr, unsigned>> &sol_segements,
                      const list<RegisterPtr> &input_regs,
                      const vector<pair<REG, int>> &must_control_list);
bool minimize_instruction(list<RegisterPtr> &sol_register,
                          vector<pair<SegmentPtr, unsigned>> &sol_segements,
                          const list<RegisterPtr> &input_regs,
                          const vector<pair<REG, int>> &must_control_list);
bool run_segment_list(vector<pair<SegmentPtr, unsigned>> &run_code, list<RegisterPtr> &registers);
void minimize_segment_nb(list<RegisterPtr> &sol_register,
                         vector<pair<SegmentPtr, unsigned>> &sol_segements,
                         const list<RegisterPtr> &input_regs,
                         const vector<pair<REG, int>> &must_control_list, int idx,
                         vector<pair<SegmentPtr, unsigned>> &run_code,
                         vector<pair<SegmentPtr, unsigned>> &orig_segements);

void minimize_result(list<RegisterPtr> &sol_register,
                     vector<pair<SegmentPtr, unsigned>> &sol_segements,
                     const list<RegisterPtr> &input_regs,
                     const vector<pair<REG, int>> &must_control_list) {
  while (true) {
    bool segment_minimize, deep_minimize;
    segment_minimize = minimize_segment(sol_register, sol_segements, input_regs, must_control_list);
    /*       cout << segment_minimize << " after segment_minimize:" << endl;
           for(auto &s: sol_segements) {
               auto seg = s.first;
               auto idx = s.second;
               cout << endl;
               for (auto i=idx; i < seg->inst_list_.size(); i++)
                   cout << seg->inst_list_[i]->original_inst_ << endl;
           }
   */
    deep_minimize
        = minimize_instruction(sol_register, sol_segements, input_regs, must_control_list);
    /*        cout << "\n\n" << deep_minimize << " after instruction_minimize:" << endl;
            for(auto &s: sol_segements) {
                   auto seg = s.first;
                   auto idx = s.second;
                   cout << endl;
                   for (auto i=idx; i < seg->inst_list_.size(); i++)
                       cout << seg->inst_list_[i]->original_inst_ << endl;
            }
      */
    if (!segment_minimize && !deep_minimize) break;
  }

  return;
}

void minimize_segment_nb(list<RegisterPtr> &sol_register,
                         vector<pair<SegmentPtr, unsigned>> &sol_segements,
                         const list<RegisterPtr> &input_regs,
                         const vector<pair<REG, int>> &must_control_list, int idx,
                         vector<pair<SegmentPtr, unsigned>> &run_code,
                         vector<pair<SegmentPtr, unsigned>> &orig_segements) {
  for (size_t i = idx; i < orig_segements.size(); i++) {
    run_code.push_back(orig_segements[i]);
    auto registers = copy_reg_list(input_regs);
    if (run_segment_list(run_code, registers) == false) {
      run_code.pop_back();
      delete_reg_list(registers);
      continue;
    }
    if (is_solution(must_control_list, registers)) {
      if (sol_segements.size() > run_code.size()) {
        sol_segements = run_code;
        sol_register = registers;
      }
    }

    minimize_segment_nb(sol_register, sol_segements, input_regs, must_control_list, i + 1, run_code,
                        orig_segements);
    run_code.pop_back();
    delete_reg_list(registers);
  }
}

bool minimize_segment(list<RegisterPtr> &sol_register,
                      vector<pair<SegmentPtr, unsigned>> &sol_segements,
                      const list<RegisterPtr> &input_regs,
                      const vector<pair<REG, int>> &must_control_list) {
  auto solution_size = sol_segements.size();
  vector<pair<SegmentPtr, unsigned>> tmp;
  auto orig_segment = sol_segements;
  vector<int> log;
  minimize_segment_nb(sol_register, sol_segements, input_regs, must_control_list, 0, tmp,
                      orig_segment);

  return solution_size != sol_segements.size();
}

bool minimize_instruction(list<RegisterPtr> &sol_register,
                          vector<pair<SegmentPtr, unsigned>> &sol_segements,
                          const list<RegisterPtr> &input_regs,
                          const vector<pair<REG, int>> &must_control_list) {
  bool is_optimized = false;
  for (auto &segment_info : sol_segements) {
    auto segment = segment_info.first;
    auto segment_inst_index = segment_info.second;
    auto max_inst_size = segment->inst_list_.size();
    for (auto i = segment_inst_index + 1; i < max_inst_size; i++) {
      segment_info.second = i;
      auto registers = copy_reg_list(input_regs);
      if (run_segment_list(sol_segements, registers) == false) {
        delete_reg_list(registers);
        continue;
      }
      if (is_solution(must_control_list, registers)) {
        segment_inst_index = i;
        delete_reg_list(registers);
        sol_register = registers;
        is_optimized = true;
      } else {
        delete_reg_list(registers);
      }
    }
    segment_info.second = segment_inst_index;
  }

  return is_optimized;
}

bool run_segment_list(vector<pair<SegmentPtr, unsigned>> &run_code, list<RegisterPtr> &registers) {
  for (auto &segment_info : run_code) {
    auto segment = segment_info.first;
    auto segment_inst_index = segment_info.second;
    segment->useful_inst_index_ = segment_inst_index;
    if (execute_instructions(segment, registers, false) == false) return false;
  }

  return true;
}

void delete_reg_list(list<RegisterPtr> &reg_list) {
  for (auto reg : reg_list) {
  }
}

list<RegisterPtr> copy_reg_list(list<RegisterPtr> reg_list) {
  list<RegisterPtr> result;

  for (auto reg : reg_list) {
    auto new_reg = std::make_shared<Register>(reg);
    for (auto reg2 : result) {
      if (reg2->mem_->mem_id_ == new_reg->mem_->mem_id_) {
        new_reg->mem_ = reg2->mem_;
        break;
      }
    }
    result.push_back(new_reg);
  }
  return result;
}

RegisterPtr get_reg(REG reg, const list<RegisterPtr> &reg_list) {
  for (const auto &each : reg_list) {
    if (each->name_ == reg) {
      return each;
    }
  }
  assert(false && "This should never happen");
  return nullptr;
}

void match_and_print(vector<shared_ptr<Memory>> mem_list,
                     const vector<pair<SegmentPtr, unsigned>> &code_segments,
                     const vector<pair<REG, int>> &must_control_list,
                     const list<RegisterPtr> &reg_list) {
  unsigned long real_addr = -1;

  for (const auto &each : mem_list) {
    for (const auto &kv : each->content_) {
      auto key = kv.first;
      auto value = kv.second;
      if (value.type_ == VALUETYPE::kCallValue) {
        real_addr = locate_next_inst_addr(value.value_, code_segments);
        each->content_[key].value_ = real_addr;
      } else if (value.type_ == VALUETYPE::kCallRegValue) {
        auto tmp_mem_id = value.value_ >> 32;
        auto tmp_inst_offset = value.value_ & 0xffffffff;
        for (const auto &m : mem_list) {
          for (const auto &kv2 : m->content_) {
            auto key2 = kv2.first;
            auto value2 = kv2.second;
            if (value2.type_ == kMemValue && value2.value_ == tmp_mem_id) {
              m->content_[key2].type_ = kCallRegValue;
              real_addr = locate_next_inst_addr(tmp_inst_offset, code_segments);
              m->content_[key2].value_ = real_addr;
            }
          }
        }
      } else if (value.type_ == kMemValue) {
        auto is_found = false;
        for (const auto &reg_flag : must_control_list) {
          auto reg = get_reg(reg_flag.first, reg_list);
          if (reg->mem_->mem_id_ == value.value_) {
            assert(!is_found);
            each->content_[key].value_ = reg->mem_->mem_id_;
            is_found = true;
          }
        }
      }
    }
  }
}

// New functions
Solution OnePunch::find_solution(vector<SegmentPtr> &code_segments) {
  Solution sol;
  sol.found = dfs(code_segments, must_control_list_, input_regs_, sol.output_reg_list,
                  sol.output_segments, search_level_);
  return sol;
}

void OnePunch::minimize_solution(Solution &solution) {
  minimize_result(solution.minimized_reg_list, solution.output_segments, input_regs_,
                  must_control_list_);
}

void OnePunch::record_memory_stage(Solution &solution) {
  std::vector<REG> controlled_regs;
  for (auto reg : input_regs_) {
    controlled_regs.push_back(reg->name_);
  }
  record_memory(controlled_regs, solution.output_segments, must_control_list_);
}

void OnePunch::Run() {
  std::srand(0);
  auto t_start = get_cur_time();
  auto instruction_list = get_disasm_code(input_file_);
  auto code_segments = get_call_segment(instruction_list);
  std::sort(code_segments.begin(), code_segments.end(),
            [](const SegmentPtr &a, const SegmentPtr &b) -> bool {
              return a->to_string(false) < b->to_string(false);
            });

  cout << "Segment size: " << code_segments.size() << endl;
  cout << "Collect segment time: " << get_cur_time() - t_start << endl;
  t_start = get_cur_time();

  Preprocessor::process(code_segments);
  cout << "Preprocess time: " << get_cur_time() - t_start << endl;
  t_start = get_cur_time();

  Solution solution = find_solution(code_segments);

  if (solution.found == false) {
    cout << "No solution found!" << endl;
    return;
  }

  cout << "Solution found!" << std::endl;
  for (auto &i : solution.output_segments) {
    for (auto idx = i.second; idx < i.first->inst_list_.size(); idx++) {
      cout << i.first->inst_list_[idx]->original_inst_ << endl;
    }
    cout << "------" << endl;
  }
  // cout << "DFS time: " << get_cur_time() - t_start << endl;
  t_start = get_cur_time();

  minimize_solution(solution);

  cout << "after minimize:" << endl;
  for (auto &i : solution.output_segments) {
    for (auto idx = i.second; idx < i.first->inst_list_.size(); idx++) {
      cout << i.first->inst_list_[idx]->original_inst_ << endl;
    }
    cout << "------" << endl;
  }
  //cout << "Minimization time: " << get_cur_time() - t_start << endl;

  record_memory_stage(solution);
}

std::optional<std::list<RegisterPtr>> ParseInputRegs(std::vector<std::string> input_regs) {
  vector<REG> control_reg_for_prepare;
  map<REG, vector<pair<long, long>>> control_reg_remove_ranges;
  for (auto i : input_regs) {
    auto tmp_vec = str_split(i, ":");
    auto r_name = tmp_vec[0];
    auto reg = get_reg_by_str(r_name);
    if (reg == REG_NONE) {
      return std::nullopt;
    }
    for (size_t idx = 1; idx < tmp_vec.size(); idx++) {
      auto range = str_split(tmp_vec[idx], "-");
      assert(range.size() == 2);
      control_reg_remove_ranges[reg].push_back(
          make_pair(atol(range[0].c_str()), atol(range[1].c_str())));
    }
    control_reg_for_prepare.push_back(reg);
  }

  auto reg_list = prepare_reg_list(control_reg_for_prepare);
  for (auto r : reg_list) {
    for (auto range : control_reg_remove_ranges[r->name_]) {
      r->remove_range(range);
    }
  }
  return reg_list;
}

std::optional<vector<pair<REG, int>>> ParseMustControlRegs(
    std::vector<std::string> must_control_list) {
  vector<pair<REG, int>> must_control_reg_list;

  for (auto &i : must_control_list) {
    auto a = str_split(i, ":");
    auto reg = get_reg_by_str(a[0]);
    if (reg == REG_NONE || a.size() != 2) {
      return std::nullopt;
    }

    if (a[1][0] == '1') {
      must_control_reg_list.push_back(make_pair(reg, 1));
    } else {
      must_control_reg_list.push_back(make_pair(reg, 0));
    }
  }
  return must_control_reg_list;
}
