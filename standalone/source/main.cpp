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

namespace {
  void PrintUsageAndExit() {
    std::cout << "Example: ./OnePunch -i rdi rsi -c rsp:0 rbp:1 -f libc.so.6" << std::endl;
    std::cout
        << "1 means we want to completely control the value of the register, and 0 means we allow "
           "the register to be a pointer value as long as it can point to a buffer that we control."
        << std::endl;
    exit(-1);
  }
}  // namespace

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
    PrintUsageAndExit();
  }
  std::string file_path = program.get<std::string>("file");
  std::vector<string> control_reg_names = program.get<std::vector<std::string>>("input");
  vector<string> must_control_reg_names = program.get<std::vector<std::string>>("control");
  unsigned long search_level = program.get<int>("level");

  auto reg_list = ParseInputRegs(control_reg_names);
  if (!reg_list.has_value()) {
    PrintUsageAndExit();
  }

  auto must_control_reg_list = ParseMustControlRegs(must_control_reg_names);

  if (!must_control_reg_list.has_value()) {
    PrintUsageAndExit();
  }

  OnePunch onepunch(file_path, *reg_list, *must_control_reg_list, search_level);
  onepunch.Run();

  // TODO: Fix the bug in record_memory before we can enable this feature.
  // record_memory(control_reg_for_prepare, output_segments, must_control_reg_list);

  return 0;
}
