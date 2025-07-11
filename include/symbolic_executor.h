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