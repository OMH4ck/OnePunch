#ifndef ONEPUNCH_SOLVER_H_
#define ONEPUNCH_SOLVER_H_

#include <list>
#include <memory>
#include <vector>

#include "onepunch.h"

namespace onepunch {

class Solver {
 public:
  Solver(std::vector<SegmentPtr>& code_segments,
         const std::vector<std::pair<REG, int>>& must_control_list,
         const std::list<RegisterPtr>& reg_list, unsigned long search_level);

  bool Dfs(std::list<RegisterPtr>& output_register,
           std::vector<std::pair<SegmentPtr, unsigned>>& output_segments);

 private:
  std::vector<SegmentPtr>& code_segments_;
  const std::vector<std::pair<REG, int>>& must_control_list_;
  const std::list<RegisterPtr>& reg_list_;
  unsigned long search_level_;
};

}  // namespace onepunch

#endif  // ONEPUNCH_SOLVER_H_