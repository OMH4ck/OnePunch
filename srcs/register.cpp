#include "register.h"

#include <iostream>

#include "onepunch.h"
#include "utils.h"

extern long MEM_INF;
extern bool RECORD_MEM;
extern std::vector<std::shared_ptr<Memory>> MEM_LIST;

void Memory::increase_ref_count() { this->ref_count_++; }

void Memory::decrease_ref_count() { this->ref_count_--; }

void Memory::set_content(long offset, const Value& val, OPERATION_LENGTH len) {
  (void)len;
  content_[offset] = val;
}

bool Memory::contain_range(const std::pair<long, long>& range) {
  for (auto& r : range_) {
    if (r.second >= range.second) {
      return r.first <= range.first;
    }
  }
  return false;
}

std::string Value::to_string() {
  std::string type_str, value_str;
  char tmp[20];
  if (this->type_ == kCallValue) {
    type_str = "CALL_VALUE";
    if (this->value_ == -1) {
      strcpy(tmp, "Target RIP");
    } else {
      sprintf(tmp, "0x%lx(inst)", this->value_);
    }
    value_str = tmp;
  } else if (this->type_ == kMemValue) {
    type_str = "MEM_VALUE";
    sprintf(tmp, "0x%lx(memid)", this->value_);
    value_str = tmp;
  } else if (this->type_ == kCallRegValue) {
    type_str = "CALL_REG_VALUE";
    if (this->value_ == -1) {
      strcpy(tmp, "Target RIP");
    } else if (this->value_ >> 32) {
      sprintf(tmp, "0x%lx(memid)", this->value_ >> 32);
    } else {
      sprintf(tmp, "0x%lx(inst)", this->value_);
    }
    value_str = tmp;
  } else if (this->type_ == kOtherValue) {
    type_str = "OTHER_VALUE";
    sprintf(tmp, "0x%lx", this->value_);
    value_str = tmp;
  }
  return type_str + "," + value_str;
}

Memory::Memory() {
  static unsigned long id = 0;
  mem_id_ = id++;
}

Memory::~Memory() {
  this->content_.clear();
  this->range_.clear();
}

bool Memory::remove_range(const std::pair<long, long>& range) {
  for (auto start = range_.begin(); start != range_.end(); start++) {
    if (start->second >= range.second) {
      if (start->first > range.first) return false;
      auto saved = start->first;
      start->first = range.second;
      range_.insert(start, std::make_pair(saved, range.first));
      return true;
    }
  }
  return false;
}

std::string Memory::to_string() {
  std::string info_range = "Available:[ ";
  for (const auto& each : this->range_) {
    if (each.first == each.second) continue;
    char left[20];
    if (each.first == -MEM_INF) {
      strcpy(left, "-INF");
    } else {
      sprintf(left, "%s0x%lx", each.first < 0 ? "-" : "",
              each.first < 0 ? -each.first : each.first);
    }
    char right[20];
    if (each.second == MEM_INF) {
      strcpy(right, "INF");
    } else {
      sprintf(right, "%s0x%lx", each.second < 0 ? "-" : "",
              each.second < 0 ? -each.second : each.second);
    }
    auto tmp_range = string_format("[%s,%s], ", left, right);
    info_range += tmp_range;
  }
  info_range.pop_back();
  info_range.pop_back();
  info_range += " ]";

  std::string info_content = "content:[ ";
  for (auto& kv : this->content_) {
    auto tmp_content = string_format(
        "[%s0x%lx:(%s)], ", kv.first < 0 ? "-" : "",
        kv.first < 0 ? -kv.first : kv.first, kv.second.to_string().c_str());
    info_content += tmp_content;
  }

  std::string result;
  char memid[10];
  sprintf(memid, "0x%x", this->mem_id_);
  result = "memid:";
  result += memid;
  result += ", relation: " + this->get_input_relation();
  if (this->content_.size()) {
    info_content.pop_back();
    info_content.pop_back();
    info_content += " ]";
    result += "\n\t" + info_range + "\n\t" + info_content;
  } else {
    result += "\n\t" + info_range;
  }

  return result;
}

Register::Register(bool alloc_mem) {
  this->base_offset_ = 0;
  this->input_src_ = "";
  this->input_action_ = false;
  this->input_offset_ = 0;
  if (alloc_mem) {
    this->mem_ = std::make_shared<Memory>();
    if (RECORD_MEM) MEM_LIST.push_back(this->mem_);
    this->mem_->ref_count_ = 1;
    this->mem_->range_.push_back(std::make_pair(-MEM_INF, MEM_INF));
  }
}

Register::Register(RegisterPtr reg) {
  this->name_ = reg->name_;
  this->base_offset_ = reg->base_offset_;
  this->input_src_ = reg->input_src_;
  this->input_offset_ = reg->input_offset_;
  this->input_action_ = reg->input_action_;
  this->mem_ = std::make_shared<Memory>();
  if (RECORD_MEM) MEM_LIST.push_back(this->mem_);
  this->mem_->ref_count_ = reg->mem_->ref_count_;
  this->mem_->mem_id_ = reg->mem_->mem_id_;
  this->mem_->range_ = reg->mem_->range_;
  this->mem_->content_ = reg->mem_->content_;
}

void Register::alias(const RegisterPtr& reg, bool copy_mem) {
  if (copy_mem) {
    this->mem_ = reg->mem_;
  }
  this->base_offset_ = reg->base_offset_;
  this->input_src_ = reg->input_src_;
  this->input_offset_ = reg->input_offset_;
  this->input_action_ = reg->input_action_;
  this->mem_->increase_ref_count();
}

bool Register::contain_range(const std::pair<long, long>& range) {
  return this->mem_->contain_range(
      std::make_pair(range.first + base_offset_, range.second + base_offset_));
}

bool Register::remove_range(const std::pair<long, long>& range) {
  return this->mem_->remove_range(
      std::make_pair(range.first + base_offset_, range.second + base_offset_));
}

void Register::set_content(long offset, const Value& val,
                           OPERATION_LENGTH len) {
  this->mem_->set_content(offset + base_offset_, val, len);
}

void Register::print() {
  std::cout << "name: " << get_reg_str_by_reg(name_) << std::endl;
  for (auto& i : mem_->range_) {
    std::cout << "(" << i.first + base_offset_ << ", "
              << i.second + base_offset_ << ")" << std::endl;
  }
}

std::string Register::to_string() {
  return string_format("%s:\t%s", get_reg_str_by_reg(this->name_).c_str(),
                       this->mem_->to_string().c_str());
}

std::string Register::get_input_relation() const {
  std::string relation;
  if (this->input_offset_ == 0) {
    relation = string_format("%s%s", this->input_action_ ? "*" : "",
                            this->input_src_.c_str());
  } else {
    relation = string_format(
        this->input_action_ ? "*(%s%s0x%lx)" : "%s%s0x%lx",
        this->input_src_.c_str(), this->input_offset_ < 0 ? "-" : "+",
        this->input_offset_ < 0 ? -this->input_offset_ : this->input_offset_);
  }
  return relation;
}

void Register::set_input_relation(const RegisterPtr& reg, long offset,
                                  bool action) {
  this->input_src_ = reg->get_input_relation();
  this->input_action_ = action;
  this->input_offset_ = offset;
  this->mem_->set_input_relation(reg, offset, action);
}

std::string Memory::get_input_relation() const {
  std::string relation;
  if (this->input_offset_ == 0) {
    relation = string_format("%s%s", this->input_action_ ? "*" : "",
                            this->input_src_.c_str());
  } else {
    relation = string_format(
        this->input_action_ ? "*(%s%s0x%lx)" : "%s%s0x%lx",
        this->input_src_.c_str(), this->input_offset_ < 0 ? "-" : "+",
        this->input_offset_ < 0 ? -this->input_offset_ : this->input_offset_);
  }
  return relation;
}

void Memory::set_input_relation(const RegisterPtr& reg, long offset,
                                bool action) {
  this->input_src_ = reg->get_input_relation();
  this->input_action_ = action;
  this->input_offset_ = offset;
}
