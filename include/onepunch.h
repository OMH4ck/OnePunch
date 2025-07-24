#ifndef __HEADER_ONEPUNCH__
#define __HEADER_ONEPUNCH__

#include <optional>
#include <set>

#include "asmutils.h"

extern std::set<unsigned long> g_visited;

using std::list;
using std::map;
using std::pair;
using std::shared_ptr;
using std::string;
using std::vector;

#include "register.h"

struct Solution {
  bool found = false;
  list<RegisterPtr> output_reg_list;
  vector<pair<SegmentPtr, unsigned>> output_segments;
  list<RegisterPtr> minimized_reg_list;
};

#include "preprocessor.h"

class ConstraintAnalyzer {
public:
  static unsigned long compute_constraint(const SegmentPtr segment);
  static unsigned long hash_reg_list(const set<REG> &reg_list);
  static unsigned long hash_reg_list(const list<RegisterPtr> &reg_list);
  static bool hash_match(unsigned long needed, unsigned long src);
  static void collect_input_output_regs(const SegmentPtr segment, set<REG> &input_regs,
                                        set<REG> &output_regs);
private:
  static bool opcode_dst_control(OPCODE opcode);
};

void match_and_print(vector<shared_ptr<Memory>> mem_list,
                     const vector<pair<SegmentPtr, unsigned>> &code_segments,
                     const vector<pair<REG, int>> &must_control_list,
                     const list<RegisterPtr> &reg_list);

void record_memory(const vector<REG> &reg_name_list,
                   vector<pair<SegmentPtr, unsigned>> &code_segments,
                   const vector<pair<REG, int>> &must_control_list);

template <typename... Args> std::string string_format(const char *format, Args... args) {
  size_t size = snprintf(nullptr, 0, format, args...) + 1;
  std::unique_ptr<char[]> buf(new char[size]);
  snprintf(buf.get(), size, format, args...);
  string result = buf.get();
  buf.reset();
  return result;
}

class Preprocessor;

class OnePunch {
public:
  OnePunch(std::string input_file, std::list<RegisterPtr> input_regs,
           std::vector<std::pair<REG, int>> must_control_list, int search_level)
      : input_file_(input_file),
        input_regs_(input_regs),
        must_control_list_(must_control_list),
        search_level_(search_level) {}
  void Run();

  // New functions
  Solution find_solution(vector<SegmentPtr> &code_segments, const Preprocessor& preprocessor);
  void minimize_solution(Solution &solution);
  void record_memory_stage(Solution &solution);

private:
  std::string input_file_;
  std::list<RegisterPtr> input_regs_;
  std::vector<std::pair<REG, int>> must_control_list_;
  int search_level_;
};

std::optional<std::list<RegisterPtr>> ParseInputRegs(std::vector<std::string> input_regs);
std::optional<vector<pair<REG, int>>> ParseMustControlRegs(
    std::vector<std::string> must_control_list);

REG find_reg64(REG r);
unsigned long locate_next_inst_addr(unsigned long offset,
                                    const vector<pair<SegmentPtr, unsigned>> &code_segments);
#endif
