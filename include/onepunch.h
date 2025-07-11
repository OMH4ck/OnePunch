#ifndef __HEADER_ONEPUNCH__
#define __HEADER_ONEPUNCH__

#include <optional>

#include "asmutils.h"

extern set<unsigned long> g_visited;

using std::list;
using std::map;
using std::pair;
using std::shared_ptr;
using std::string;
using std::vector;

enum VALUETYPE {
  kCallValue = 0,
  kMemValue,
  kCallRegValue,
  kImmValue,
  kOtherValue,
};

class Register;
using RegisterPtr = std::shared_ptr<Register>;

struct Solution {
  bool found = false;
  list<RegisterPtr> output_reg_list;
  vector<pair<SegmentPtr, unsigned>> output_segments;
  list<RegisterPtr> minimized_reg_list;
};


class Value {
public:
  VALUETYPE type_;
  long value_;

  Value(VALUETYPE type, long value) : type_(type), value_(value) {}
  Value() {}
  string to_string();
};

class Memory {
public:
  unsigned ref_count_ = 0;
  unsigned mem_id_;
  list<pair<long, long>> range_;
  map<long, Value> content_;
  string input_src_;
  long input_offset_ = 0;
  bool input_action_ = 0;

  Memory();
  ~Memory();
  string get_input_relation() const;
  void set_input_relation(const RegisterPtr reg, long offset, bool action);
  void increase_ref_count();
  void decrease_ref_count();
  bool contain_range(const pair<long, long> &range);
  bool remove_range(const pair<long, long> &range);
  void set_content(long offset, const Value &val, OPERATION_LENGTH len);
  string to_string();
};

class Register {
public:
  REG name_ = REG_NONE;
  shared_ptr<Memory> mem_ = nullptr;
  long base_offset_ = 0;
  string input_src_;
  long input_offset_ = 0;
  bool input_action_ = 0;

  Register(RegisterPtr reg);
  Register(bool alloc_mem = true);
  string to_string();
  void print();
  string get_input_relation() const;
  void set_input_relation(const RegisterPtr reg, long offset, bool action);

  void alias(const RegisterPtr reg, bool copy_mem = true);
  bool contain_range(const pair<long, long> &range);
  bool remove_range(const pair<long, long> &range);
  void set_content(long offset, const Value &val, OPERATION_LENGTH len = kQWORD);
};

class Preprocessor {
public:
  static map<unsigned long, vector<SegmentPtr>> result_;
  static map<SegmentPtr, unsigned long> test_;

  // unsigned long compute_constraint(const Segment* segment);
  static void process(const vector<SegmentPtr> &segments);

  // vector<Segment*> sample(unsigned size);
};

unsigned long compute_constraint(const SegmentPtr segment);
unsigned long hash_reg_list(const set<REG> &reg_list);
unsigned long hash_reg_list(const list<RegisterPtr> &reg_list);
void collect_input_output_regs(const SegmentPtr segment, set<REG> &input_regs,
                               set<REG> &output_regs);
bool is_in_input(REG reg, const list<RegisterPtr> &reg_list);  // done

unsigned remove_useless_instructions(
    SegmentPtr inst_list,
    const list<RegisterPtr> &reg_list);  // return the index of the first useful instructions

RegisterPtr get_reg_by_idx(REG reg, const list<RegisterPtr> &reg_list);  // get_reg_by_name

void remove_reg(RegisterPtr reg_to_remove, list<RegisterPtr> &reg_list);

void remove_reg_by_idx(REG reg, list<RegisterPtr> &reg_list);  // remove_reg_by_name

void remove_reg_and_alias(RegisterPtr reg_to_remove, list<RegisterPtr> &reg_list);

RegisterPtr make_alias(REG alias_reg_name, RegisterPtr reg, bool copy_mem = true);

// extract_reg_and_offset, extract_memory_access_reg, these two should be useless

RegisterPtr get_reg_by_relation(const string &relation, const list<RegisterPtr> &reg_list);

bool contain_uncontrol_memory_access(const InstrPtr inst,
                                     const list<RegisterPtr> &reg_list);  // check_uncontrol_rw

bool mov_handler(const InstrPtr inst, list<RegisterPtr> &reg_list, bool record_flag);

bool lea_handler(const InstrPtr inst, list<RegisterPtr> &reg_list);

bool pop_handler(const InstrPtr inst, list<RegisterPtr> &reg_list, bool record_flag);

bool add_sub_handler(const InstrPtr inst, list<RegisterPtr> &reg_list);

bool push_handler(const InstrPtr inst, list<RegisterPtr> &reg_list);

bool bitwise_handler(const InstrPtr inst, list<RegisterPtr> &reg_list);

bool xchg_handler(const InstrPtr inst, list<RegisterPtr> &reg_list, bool record_flag);

bool branch_handler(const InstrPtr inst, list<RegisterPtr> &reg_list, bool record_flag);

bool execute_one_instruction(const InstrPtr inst, list<RegisterPtr> &reg_list, bool record_flag);

bool execute_instructions(const SegmentPtr instructions, list<RegisterPtr> &reg_list,
                          bool record_flag);

list<RegisterPtr> prepare_reg_list(const vector<REG> &reg_name_list);

bool is_alias(REG reg, const list<RegisterPtr> &reg_list);

bool is_independent(REG reg, const list<RegisterPtr> &reg_list);

bool is_solution(const vector<pair<REG, int>> &must_control_list,
                 const list<RegisterPtr> &reg_list);

unsigned long compute_constraint(const SegmentPtr segment);
bool hash_match(unsigned long needed, unsigned long src);

// chain, seems useless

void match_and_print(vector<shared_ptr<Memory>> mem_list,
                     const vector<pair<SegmentPtr, unsigned>> &code_segments,
                     const vector<pair<REG, int>> &must_control_list,
                     const list<RegisterPtr> &reg_list);

void record_memory(const vector<REG> &reg_name_list,
                   vector<pair<SegmentPtr, unsigned>> &code_segments,
                   const vector<pair<REG, int>> &must_control_list);

/* we minimize in segment level and instruction level until it cannot be minimize anymore,
 * result will be filled into sol_register(register status after minimizing) and
 * sol_segements(segments after minimizing) return true for successfully minimize.
 */

void minimize_result(list<RegisterPtr> &sol_register,
                     vector<pair<SegmentPtr, unsigned>> &sol_segements,
                     const list<RegisterPtr> &input_regs,
                     const vector<pair<REG, int>> &must_control_list);

list<RegisterPtr> copy_reg_list(list<RegisterPtr> reg_list);
void delete_reg_list(list<RegisterPtr> &reg_list);

template <typename... Args> std::string string_format(const char *format, Args... args) {
  size_t size = snprintf(nullptr, 0, format, args...) + 1;
  std::unique_ptr<char[]> buf(new char[size]);
  snprintf(buf.get(), size, format, args...);
  string result = buf.get();
  buf.reset();
  return result;
}

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
  Solution find_solution(vector<SegmentPtr> &code_segments);
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
