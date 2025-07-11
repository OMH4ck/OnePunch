#include "symbolic_executor.h"

#include "onepunch.h"

namespace onepunch {

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

bool SymbolicExecutor::mov_handler(const InstrPtr inst, list<RegisterPtr> &reg_list, bool record_flag) {
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

bool SymbolicExecutor::lea_handler(const InstrPtr inst, list<RegisterPtr> &reg_list) {
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

bool SymbolicExecutor::pop_handler(const InstrPtr inst, list<RegisterPtr> &reg_list, bool record_flag) {
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

bool SymbolicExecutor::add_sub_handler(const InstrPtr inst, list<RegisterPtr> &reg_list) {
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

bool SymbolicExecutor::push_handler(const InstrPtr inst, list<RegisterPtr> &reg_list) {
  auto rsp_ptr = get_reg_by_idx(REG_RSP, reg_list);
  if (rsp_ptr == nullptr) return true;

  auto range = inst->op_dst_.value().get_used_range();
  if (rsp_ptr->contain_range(range.second) == false) return false;
  rsp_ptr->base_offset_ -= 8;
  rsp_ptr->set_input_relation(rsp_ptr, -8, false);
  return true;
}

bool SymbolicExecutor::bitwise_handler(const InstrPtr inst, list<RegisterPtr> &reg_list) {
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

bool SymbolicExecutor::xchg_handler(const InstrPtr inst, list<RegisterPtr> &reg_list, bool record_flag) {
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

bool SymbolicExecutor::branch_handler(const InstrPtr inst, list<RegisterPtr> &reg_list, bool record_flag) {
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

bool SymbolicExecutor::contain_uncontrol_memory_access(const InstrPtr inst, const list<RegisterPtr> &reg_list) {
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

bool SymbolicExecutor::execute_one_instruction(const InstrPtr inst, list<RegisterPtr> &reg_list, bool record_flag) {
  if (this->contain_uncontrol_memory_access(inst, reg_list)) {
    return false;
  }
  bool flag = true;
  switch (inst->opcode_) {
    case OP_MOV:
    case OP_MOVSXD:
      flag = this->mov_handler(inst, reg_list, record_flag);
      break;
    case OP_LEA:
      flag = this->lea_handler(inst, reg_list);
      break;
    case OP_POP:
      flag = this->pop_handler(inst, reg_list, record_flag);
      break;
    case OP_PUSH:
      flag = this->push_handler(inst, reg_list);
      break;
    case OP_ADD:
    case OP_SUB:
      flag = this->add_sub_handler(inst, reg_list);
      break;
    case OP_XOR:
    case OP_AND:
    case OP_OR:
    case OP_SHR:
    case OP_ROR:
    case OP_SAR:
    case OP_SHL:
      flag = this->bitwise_handler(inst, reg_list);
      break;
    case OP_TEST:
    case OP_CMP:
    case OP_NOP:
      break;
    case OP_CALL:
    case OP_JMP:
      flag = this->branch_handler(inst, reg_list, record_flag);
      break;
    case OP_XCHG:
      flag = this->xchg_handler(inst, reg_list, record_flag);
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

bool SymbolicExecutor::ExecuteInstructions(const SegmentPtr instructions,
                                             std::list<RegisterPtr>& reg_list, bool record_flag) {
    unsigned start_idx = instructions->useful_inst_index_;
    for (; start_idx < instructions->inst_list_.size(); start_idx++) {
      if (this->execute_one_instruction(instructions->inst_list_[start_idx], reg_list, record_flag)
          == false) {
        if (record_flag)
          cout << "Inst: " << instructions->inst_list_[start_idx]->original_inst_ << endl;
        return false;
      }
      if (reg_list.empty()) {
        return false;
      }
    }
    return true;
  }

}  // namespace onepunch