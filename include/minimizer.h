#ifndef ONEPUNCH_MINIMIZER_H_
#define ONEPUNCH_MINIMIZER_H_

#include <list>
#include <memory>
#include <vector>

#include "onepunch.h"

namespace onepunch {

class Minimizer {
 public:
  Minimizer(
      std::list<RegisterPtr>& sol_register,
      std::vector<std::pair<SegmentPtr, unsigned>>& sol_segements,
      const std::list<RegisterPtr>& input_regs,
      const std::vector<std::pair<REG, int>>& must_control_list);

  void Minimize();

 private:
  bool MinimizeSegment();
  bool MinimizeInstruction();
  void MinimizeSegmentNb(int idx,
                         std::vector<std::pair<SegmentPtr, unsigned>>& run_code,
                         std::vector<std::pair<SegmentPtr, unsigned>>& orig_segements);
  bool RunSegmentList(std::vector<std::pair<SegmentPtr, unsigned>>& run_code,
                      std::list<RegisterPtr>& registers);

  std::list<RegisterPtr>& sol_register_;
  std::vector<std::pair<SegmentPtr, unsigned>>& sol_segements_;
  const std::list<RegisterPtr>& input_regs_;
  const std::vector<std::pair<REG, int>>& must_control_list_;
};

}  // namespace onepunch

#endif  // ONEPUNCH_MINIMIZER_H_