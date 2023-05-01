#include <unistd.h>

#include <algorithm>
#include <argparse/argparse.hpp>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>

#include "asmutils.h"
#include "onepunch.h"
#include "utils.h"

using namespace std;

void usage() {
  cout << "Example: ./OnePunch -i rdi rsi -c rsp:0 rbp:1 -f libc.so.6" << endl;
  cout << "1 means completely control the value of the register, and 0 means the register might "
          "have to be a pointer"
       << endl;
}

int main(int argc, char **argv) {
  argparse::ArgumentParser program("OnePunch");
  program.add_argument("-i", "--input")
      .nargs(argparse::nargs_pattern::at_least_one)
      .required()
      .help("The registers that we control");
  program.add_argument("-c", "--control")
      .nargs(argparse::nargs_pattern::at_least_one)
      .required()
      .help("The registers we want to control");
  program.add_argument("-f", "--file").required().help("The binary file that we want to analyze");
  program.add_argument("-l", "--level").default_value(1).help("The search level").scan<'i', int>();

  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error &err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    usage();
    return 1;
  }
  std::string file_path = program.get<std::string>("file");
  std::vector<string> control_reg_names = program.get<std::vector<std::string>>("input");
  // std::string must_control_reg_str = program.get<std::string>("control");
  vector<string> must_control_reg_names = program.get<std::vector<std::string>>("control");
  unsigned long search_level = program.get<int>("level");

  // vector<string> must_control_reg_names = str_split(must_control_reg_str, ",");
  vector<REG> control_reg_for_prepare;
  map<REG, vector<pair<long, long>>> control_reg_remove_ranges;
  for (auto i : control_reg_names) {
    auto tmp_vec = str_split(i, ":");
    auto r_name = tmp_vec[0];
    auto reg = get_reg_by_str(r_name);
    if (reg == REG_NONE) {
      usage();
    }
    for (auto idx = 1; idx < tmp_vec.size(); idx++) {
      auto range = str_split(tmp_vec[idx], "-");
      assert(range.size() == 2);
      control_reg_remove_ranges[reg].push_back(
          make_pair(atol(range[0].c_str()), atol(range[1].c_str())));
    }
    control_reg_for_prepare.push_back(reg);
  }

  auto reg_list = prepare_reg_list(control_reg_for_prepare);
  for (auto r : reg_list) {
    for (auto range : control_reg_remove_ranges[r->name_]) {
      r->remove_range(range);
    }
  }

  vector<pair<REG, int>> must_control_reg_list;

  for (auto &i : must_control_reg_names) {
    auto a = str_split(i, ":");
    auto reg = get_reg_by_str(a[0]);
    if (reg == REG_NONE || a.size() != 2) usage();

    if (a[1][0] == '1') {
      must_control_reg_list.push_back(make_pair(reg, 1));
    } else {
      must_control_reg_list.push_back(make_pair(reg, 0));
    }
  }
  std::srand(unsigned(std::time(0)));
  auto t_start = get_cur_time();
  auto instruction_list = get_disasm_code(file_path);
  auto code_segments = get_call_segment(instruction_list);
  random_shuffle(code_segments.begin(), code_segments.end());
  list<Register *> output_reg_list;
  vector<pair<Segment *, unsigned>> output_segments;
  cout << "Segment size: " << code_segments.size() << endl;
  cout << "Collect segment time: " << get_cur_time() - t_start << endl;
  ;
  t_start = get_cur_time();

  Preprocessor::process(code_segments);
  cout << "Preprocess time: " << get_cur_time() - t_start << endl;
  t_start = get_cur_time();

  if (0) {
    for (auto i : code_segments) {
      for (auto k : i->inst_list_) {
        cout << k->original_inst_ << endl;
      }
      cout << "-------" << endl;
    }
  }
  bool res = dfs(code_segments, must_control_reg_list, reg_list, output_reg_list, output_segments,
                 search_level);

  if (res == false) {
    cout << "No solution found!" << endl;
    return 0;
  }

  for (auto &i : output_segments) {
    for (auto idx = i.second; idx < i.first->inst_list_.size(); idx++) {
      cout << i.first->inst_list_[idx]->original_inst_ << endl;
    }
    cout << "------" << endl;
  }
  cout << "DFS time: " << get_cur_time() - t_start << endl;
  t_start = get_cur_time();
  cout << "after minimize:" << endl;
  list<Register *> sol_reg;
  minimize_result(sol_reg, output_segments, reg_list, must_control_reg_list);
  for (auto &i : output_segments) {
    for (auto idx = i.second; idx < i.first->inst_list_.size(); idx++) {
      cout << i.first->inst_list_[idx]->original_inst_ << endl;
    }
    cout << "------" << endl;
  }
  cout << "Minimization time: " << get_cur_time() - t_start << endl;

  record_memory(control_reg_for_prepare, output_segments, must_control_reg_list);

  for (auto &each : instruction_list) {
    delete each;
  }

  for (auto &each : code_segments) {
    delete each;
  }

  return 0;
}
