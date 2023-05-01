#ifndef __HEADER_UTILS__
#define __HEADER_UTILS__

#include <ctime>
#include <string>
#include <vector>

#include "common.h"

using std::string;
using std::vector;

string transfer_operation_len_to_str(unsigned dtype);
// need a function that calcuates hash for string
unsigned long fuck_hash(string &str);
inline unsigned long gen_id();
bool is_imm(string &str);

vector<string> str_split(string str, string delimiter);
string str_trim(const string &str);

double get_cur_time();
#endif