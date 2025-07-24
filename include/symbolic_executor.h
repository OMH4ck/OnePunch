#ifndef ONEPUNCH_SYMBOLIC_EXECUTOR_H_
#define ONEPUNCH_SYMBOLIC_EXECUTOR_H_

#include <list>
#include <memory>
#include <vector>

#include "onepunch.h"

namespace onepunch {

  class SymbolicExecutor {
  public:
    SymbolicExecutor() = default;

    bool ExecuteInstructions(const SegmentPtr instructions, std::list<RegisterPtr>& reg_list,
                             bool record_flag);

    // Register state management utilities
    static bool is_in_input(REG reg, const list<RegisterPtr> &reg_list);
    static RegisterPtr get_reg_by_idx(REG reg, const list<RegisterPtr> &reg_list);
    static void remove_reg(RegisterPtr reg_to_remove, list<RegisterPtr> &reg_list);
    static void remove_reg_by_idx(REG reg, list<RegisterPtr> &reg_list);
    static void remove_reg_and_alias(RegisterPtr reg_to_remove, list<RegisterPtr> &reg_list);
    static RegisterPtr make_alias(REG alias_reg_name, RegisterPtr reg, bool copy_mem = true);
    static RegisterPtr get_reg_by_relation(const string &relation, const list<RegisterPtr> &reg_list);
    static bool is_alias(REG reg, const list<RegisterPtr> &reg_list);
    static bool is_independent(REG reg, const list<RegisterPtr> &reg_list);
    static list<RegisterPtr> copy_reg_list(list<RegisterPtr> reg_list);
    static void delete_reg_list(list<RegisterPtr> &reg_list);
    static list<RegisterPtr> prepare_reg_list(const vector<REG> &reg_name_list);

    // Solution analysis utilities
    static unsigned remove_useless_instructions(SegmentPtr inst_list, const list<RegisterPtr> &reg_list);
    static bool is_solution(const vector<pair<REG, int>> &must_control_list,
                           const list<RegisterPtr> &reg_list);
  
  private:
    bool contain_uncontrol_memory_access(const InstrPtr inst, const list<RegisterPtr> &reg_list);
    bool execute_one_instruction(const InstrPtr inst, list<RegisterPtr> &reg_list, bool record_flag);
    bool mov_handler(const InstrPtr inst, list<RegisterPtr> &reg_list, bool record_flag);
    bool lea_handler(const InstrPtr inst, list<RegisterPtr> &reg_list);
    bool pop_handler(const InstrPtr inst, list<RegisterPtr> &reg_list, bool record_flag);
    bool add_sub_handler(const InstrPtr inst, list<RegisterPtr> &reg_list);
    bool push_handler(const InstrPtr inst, list<RegisterPtr> &reg_list);
    bool bitwise_handler(const InstrPtr inst, list<RegisterPtr> &reg_list);
    bool xchg_handler(const InstrPtr inst, list<RegisterPtr> &reg_list, bool record_flag);
    bool branch_handler(const InstrPtr inst, list<RegisterPtr> &reg_list, bool record_flag);
  };

}  // namespace onepunch

#endif  // ONEPUNCH_SYMBOLIC_EXECUTOR_H_