#ifndef ONEPUNCH_REGISTER_H_
#define ONEPUNCH_REGISTER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <list>

#include "asmutils.h"

// Forward declaration
class Register;
using RegisterPtr = std::shared_ptr<Register>;

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
  std::string to_string();
};

class Memory {
 public:
  unsigned ref_count_ = 0;
  unsigned mem_id_;
  std::list<std::pair<long, long>> range_;
  std::map<long, Value> content_;
  std::string input_src_;
  long input_offset_ = 0;
  bool input_action_ = 0;

  Memory();
  ~Memory();
  std::string get_input_relation() const;
  void set_input_relation(const RegisterPtr& reg, long offset, bool action);
  void increase_ref_count();
  void decrease_ref_count();
  bool contain_range(const std::pair<long, long>& range);
  bool remove_range(const std::pair<long, long>& range);
  void set_content(long offset, const Value& val, OPERATION_LENGTH len);
  std::string to_string();
};

class Register {
 public:
  REG name_ = REG_NONE;
  std::shared_ptr<Memory> mem_ = nullptr;
  long base_offset_ = 0;
  std::string input_src_;
  long input_offset_ = 0;
  bool input_action_ = 0;

  Register(RegisterPtr reg);
  Register(bool alloc_mem = true);
  std::string to_string();
  void print();
  std::string get_input_relation() const;
  void set_input_relation(const RegisterPtr& reg, long offset, bool action);

  void alias(const RegisterPtr& reg, bool copy_mem = true);
  bool contain_range(const std::pair<long, long>& range);
  bool remove_range(const std::pair<long, long>& range);
  void set_content(long offset, const Value& val,
                   OPERATION_LENGTH len = kQWORD);
};

#endif  // ONEPUNCH_REGISTER_H_