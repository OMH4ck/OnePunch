#include "onepunch.h"
#include "minimizer.h"
#include "solver.h"
#include "symbolic_executor.h"


#include <algorithm>

#include "assembler.h"

long MEM_INF = 0x10000000;

// static const char *g_type_strings[] = {"CallValue", "MemValue", "CallRegValue", "OtherValue"};

vector<shared_ptr<Memory>> MEM_LIST;
bool RECORD_MEM = false;

void collect_input_output_regs(const SegmentPtr segment, set<REG> &input_regs,
                               set<REG> &output_regs);



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

bool is_alias(REG reg, const list<RegisterPtr> &reg_list) {
  auto reg_ptr = get_reg_by_idx(reg, reg_list);
  if (reg_ptr == nullptr) return false;
  if (reg_ptr->mem_->ref_count_ > 1) return true;
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
    onepunch::SymbolicExecutor executor;
    executor.ExecuteInstructions(iter.first, reg_list, true);
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
Solution OnePunch::find_solution(vector<SegmentPtr> &code_segments, const Preprocessor& preprocessor) {
  Solution sol;
  onepunch::Solver solver(code_segments, must_control_list_, input_regs_, search_level_, preprocessor);
  sol.found = solver.Dfs(sol.output_reg_list, sol.output_segments);
  return sol;
}

void OnePunch::minimize_solution(Solution &solution) {
  onepunch::Minimizer minimizer(solution.minimized_reg_list, solution.output_segments, input_regs_,
                                must_control_list_);
  minimizer.Minimize();
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
  onepunch::Assembler assembler;
  auto instruction_list = assembler.GetDisasmCode(input_file_);
  auto code_segments = assembler.GetCallSegment(instruction_list);
  std::sort(code_segments.begin(), code_segments.end(),
            [](const SegmentPtr &a, const SegmentPtr &b) -> bool {
              return a->to_string(false) < b->to_string(false);
            });

  cout << "Segment size: " << code_segments.size() << endl;
  cout << "Collect segment time: " << get_cur_time() - t_start << endl;
  t_start = get_cur_time();

  Preprocessor preprocessor;
  preprocessor.process(code_segments);
  cout << "Preprocess time: " << get_cur_time() - t_start << endl;
  t_start = get_cur_time();

  Solution solution = find_solution(code_segments, preprocessor);

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
  // cout << "Minimization time: " << get_cur_time() - t_start << endl;

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
