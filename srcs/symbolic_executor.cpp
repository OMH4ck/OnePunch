#include "symbolic_executor.h"

#include "onepunch.h"

namespace onepunch {

  namespace {

    bool ContainsUncontrolMemoryAccess(const InstrPtr inst,
                                       const std::list<RegisterPtr>& reg_list) {
      unsigned op_num = inst->operand_num_;

      if (op_num == 0 || inst->opcode_ == OP_LEA || inst->opcode_ == OP_NOP) return false;
      auto operand = inst->op_dst_;
      if (operand->is_dereference_ == false) {
        if (!inst->op_src_.has_value() || inst->op_src_->is_dereference_ == false) return false;
        operand = inst->op_src_;
      }
      if (operand->contain_segment_reg()) return false;
      for (auto& p : operand->reg_list_) {
        if (p.second != 1) return true;
        if (is_in_input(p.first, reg_list) == false) {
          if (p.first == REG_RSP || p.first == REG_RIP || p.first == REG_RBP) continue;
          return true;
        }
      }
      return false;
    }

    bool ExecuteOneInstruction(const InstrPtr inst, std::list<RegisterPtr>& reg_list,
                               bool record_flag) {
      if (ContainsUncontrolMemoryAccess(inst, reg_list)) {
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
        default:
          flag = false;
      }
      return flag;
    }

  }  // namespace

  bool SymbolicExecutor::ExecuteInstructions(const SegmentPtr instructions,
                                             std::list<RegisterPtr>& reg_list, bool record_flag) {
    unsigned start_idx = instructions->useful_inst_index_;
    for (; start_idx < instructions->inst_list_.size(); start_idx++) {
      if (ExecuteOneInstruction(instructions->inst_list_[start_idx], reg_list, record_flag)
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