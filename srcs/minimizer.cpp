#include "minimizer.h"

#include "onepunch.h"
#include "symbolic_executor.h"

namespace onepunch {

  Minimizer::Minimizer(std::list<RegisterPtr>& sol_register,
                       std::vector<std::pair<SegmentPtr, unsigned>>& sol_segements,
                       const std::list<RegisterPtr>& input_regs,
                       const std::vector<std::pair<REG, int>>& must_control_list)
      : sol_register_(sol_register),
        sol_segements_(sol_segements),
        input_regs_(input_regs),
        must_control_list_(must_control_list) {}

  void Minimizer::Minimize() {
    while (true) {
      bool segment_minimize, deep_minimize;
      segment_minimize = MinimizeSegment();
      deep_minimize = MinimizeInstruction();
      if (!segment_minimize && !deep_minimize) break;
    }
  }

  bool Minimizer::MinimizeSegment() {
    auto solution_size = sol_segements_.size();
    std::vector<std::pair<SegmentPtr, unsigned>> tmp;
    auto orig_segment = sol_segements_;
    std::vector<int> log;
    MinimizeSegmentNb(0, tmp, orig_segment);

    return solution_size != sol_segements_.size();
  }

  void Minimizer::MinimizeSegmentNb(int idx, std::vector<std::pair<SegmentPtr, unsigned>>& run_code,
                                    std::vector<std::pair<SegmentPtr, unsigned>>& orig_segements) {
    for (size_t i = idx; i < orig_segements.size(); i++) {
      run_code.push_back(orig_segements[i]);
      auto registers = copy_reg_list(input_regs_);
      if (RunSegmentList(run_code, registers) == false) {
        run_code.pop_back();
        delete_reg_list(registers);
        continue;
      }
      if (is_solution(must_control_list_, registers)) {
        if (sol_segements_.size() > run_code.size()) {
          sol_segements_ = run_code;
          sol_register_ = registers;
        }
      }

      MinimizeSegmentNb(i + 1, run_code, orig_segements);
      run_code.pop_back();
      delete_reg_list(registers);
    }
  }

  bool Minimizer::MinimizeInstruction() {
    bool is_optimized = false;
    for (auto& segment_info : sol_segements_) {
      auto segment = segment_info.first;
      auto segment_inst_index = segment_info.second;
      auto max_inst_size = segment->inst_list_.size();
      for (auto i = segment_inst_index + 1; i < max_inst_size; i++) {
        segment_info.second = i;
        auto registers = copy_reg_list(input_regs_);
        if (RunSegmentList(sol_segements_, registers) == false) {
          delete_reg_list(registers);
          continue;
        }
        if (is_solution(must_control_list_, registers)) {
          segment_inst_index = i;
          delete_reg_list(registers);
          sol_register_ = registers;
          is_optimized = true;
        } else {
          delete_reg_list(registers);
        }
      }
      segment_info.second = segment_inst_index;
    }

    return is_optimized;
  }

  bool Minimizer::RunSegmentList(std::vector<std::pair<SegmentPtr, unsigned>>& run_code,
                                 std::list<RegisterPtr>& registers) {
    SymbolicExecutor executor;
    for (auto& segment_info : run_code) {
      auto segment = segment_info.first;
      auto segment_inst_index = segment_info.second;
      segment->useful_inst_index_ = segment_inst_index;
      if (executor.ExecuteInstructions(segment, registers, false) == false) return false;
    }

    return true;
  }

}  // namespace onepunch