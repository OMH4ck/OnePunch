#ifndef __HEADER_ONEPUNCH__
#define __HEADER_ONEPUNCH__

#include "asmutils.h"

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
  void set_input_relation(const class Register *reg, long offset, bool action);
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

  Register(Register *reg);
  Register(bool alloc_mem = true);
  string to_string();
  void print();
  string get_input_relation() const;
  void set_input_relation(const Register *reg, long offset, bool action);

  void alias(const Register *reg, bool copy_mem = true);
  bool contain_range(const pair<long, long> &range);
  bool remove_range(const pair<long, long> &range);
  void set_content(long offset, const Value &val, OPERATION_LENGTH len = kQWORD);
};

class Preprocessor {
public:
  static map<unsigned long, vector<Segment *>> result_;
  static map<Segment *, unsigned long> test_;

  // unsigned long compute_constraint(const Segment* segment);
  static void process(const vector<Segment *> &segments);

  // vector<Segment*> sample(unsigned size);
};

unsigned long compute_constraint(const Segment *segment);
unsigned long hash_reg_list(const set<REG> &reg_list);
unsigned long hash_reg_list(const list<Register *> &reg_list);
void collect_input_output_regs(const Segment *segment, set<REG> &input_regs, set<REG> &output_regs);
bool is_in_input(REG reg, const list<Register *> &reg_list);  // done

unsigned remove_useless_instructions(
    Segment *inst_list,
    const list<Register *> &reg_list);  // return the index of the first useful instructions

Register *get_reg_by_idx(REG reg, const list<Register *> &reg_list);  // get_reg_by_name

void remove_reg(Register *reg_to_remove, list<Register *> &reg_list);

void remove_reg_by_idx(REG reg, list<Register *> &reg_list);  // remove_reg_by_name

void remove_reg_and_alias(Register *reg_to_remove, list<Register *> &reg_list);

Register *make_alias(REG alias_reg_name, Register *reg, bool copy_mem = true);

// extract_reg_and_offset, extract_memory_access_reg, these two should be useless

Register *get_reg_by_relation(const string &relation, const list<Register *> &reg_list);

bool contain_uncontrol_memory_access(const Instruction *inst,
                                     const list<Register *> &reg_list);  // check_uncontrol_rw

bool mov_handler(const Instruction *inst, list<Register *> &reg_list, bool record_flag);

bool lea_handler(const Instruction *inst, list<Register *> &reg_list);

bool pop_handler(const Instruction *inst, list<Register *> &reg_list, bool record_flag);

bool add_sub_handler(const Instruction *inst, list<Register *> &reg_list);

bool push_handler(const Instruction *inst, list<Register *> &reg_list);

bool bitwise_handler(const Instruction *inst, list<Register *> &reg_list);

bool xchg_handler(const Instruction *inst, list<Register *> &reg_list, bool record_flag);

bool branch_handler(const Instruction *inst, list<Register *> &reg_list, bool record_flag);

bool execute_one_instruction(const Instruction *inst, list<Register *> &reg_list, bool record_flag);

bool execute_instructions(const Segment *instructions, list<Register *> &reg_list,
                          bool record_flag);

list<Register *> prepare_reg_list(const vector<REG> &reg_name_list);

bool is_alias(REG reg, const list<Register *> &reg_list);

bool is_independent(REG reg, const list<Register *> &reg_list);

bool is_solution(const vector<pair<REG, int>> &must_control_list, const list<Register *> &reg_list);

bool dfs(vector<Segment *> &code_segments, const vector<pair<REG, int>> &must_control_list,
         const list<Register *> &reg_list, list<Register *> &output_register,
         vector<pair<Segment *, unsigned>> &output_segments, unsigned long search_level = 1);

// chain, seems useless

void match_and_print(vector<shared_ptr<Memory>> mem_list,
                     const vector<pair<Segment *, unsigned>> &code_segments,
                     const vector<pair<REG, int>> &must_control_list,
                     const list<Register *> &reg_list);

void record_memory(const vector<REG> &reg_name_list,
                   vector<pair<Segment *, unsigned>> &code_segments,
                   const vector<pair<REG, int>> &must_control_list);

/* we minimize in segment level and instruction level until it cannot be minimize anymore,
 * result will be filled into sol_register(register status after minimizing) and
 * sol_segements(segments after minimizing) return true for successfully minimize.
 */

void minimize_result(list<Register *> &sol_register,
                     vector<pair<Segment *, unsigned>> &sol_segements,
                     const list<Register *> &input_regs,
                     const vector<pair<REG, int>> &must_control_list);

list<Register *> copy_reg_list(list<Register *> reg_list);
void delete_reg_list(list<Register *> &reg_list);

template <typename... Args> std::string string_format(const char *format, Args... args) {
  size_t size = snprintf(nullptr, 0, format, args...) + 1;
  std::unique_ptr<char[]> buf(new char[size]);
  snprintf(buf.get(), size, format, args...);
  string result = buf.get();
  buf.reset();
  return result;
}

#endif
