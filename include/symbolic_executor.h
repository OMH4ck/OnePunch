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

  bool ExecuteInstructions(const SegmentPtr instructions,
                           std::list<RegisterPtr>& reg_list,
                           bool record_flag);
};

}  // namespace onepunch

#endif  // ONEPUNCH_SYMBOLIC_EXECUTOR_H_