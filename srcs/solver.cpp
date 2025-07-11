#include "solver.h"

#include "onepunch.h"
#include "symbolic_executor.h"

namespace onepunch {

Solver::Solver(std::vector<SegmentPtr>& code_segments,
               const std::vector<std::pair<REG, int>>& must_control_list,
               const std::list<RegisterPtr>& reg_list,
               unsigned long search_level)
    : code_segments_(code_segments),
      must_control_list_(must_control_list),
      reg_list_(reg_list),
      search_level_(search_level) {}

bool Solver::Dfs(std::list<RegisterPtr>& output_register,
                 std::vector<std::pair<SegmentPtr, unsigned>>& output_segments) {
  auto tmp_h = hash_reg_list(reg_list_);
  if (search_level_ == 1) {
    if (g_visited.find(tmp_h) != g_visited.end()) {
      return false;
    }
    g_visited.insert(tmp_h);
  }

  for (auto segment : code_segments_) {
    if (search_level_ <= 2 &&
        hash_match(Preprocessor::test_[segment], tmp_h) == false)
      continue;

    segment->useful_inst_index_ = 0;
    unsigned start_index = remove_useless_instructions(segment, reg_list_);
    if (segment->inst_list_.size() - segment->useful_inst_index_ < 2) {
      continue;
    }
    if (search_level_ <= 2 &&
        hash_match(compute_constraint(segment), tmp_h) == false)
      continue;
    auto tmp_reg_list = copy_reg_list(reg_list_);

    SymbolicExecutor executor;
    if (executor.ExecuteInstructions(segment, tmp_reg_list, false) == false) {
      delete_reg_list(tmp_reg_list);
      continue;
    }

    if (is_solution(must_control_list_, tmp_reg_list)) {
      output_segments.push_back(make_pair(segment, start_index));
      output_register = move(tmp_reg_list);
      return true;
    }

    if (tmp_reg_list.size() > reg_list_.size()) {
      output_segments.push_back(make_pair(segment, start_index));

      Solver solver(code_segments_, must_control_list_, tmp_reg_list,
                    search_level_);
      bool flag_dfs = solver.Dfs(output_register, output_segments);

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

}  // namespace onepunch