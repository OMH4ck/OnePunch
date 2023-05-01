#include "utils.h"

using namespace std;

vector<string> str_split(string s, string delimiter) {
  vector<string> result;
  size_t pos = 0;
  string token;
  while ((pos = s.find(delimiter)) != string::npos) {
    result.push_back(s.substr(0, pos));
    s = s.substr(pos + delimiter.size());
  }
  result.push_back(s);
  return result;
}

string str_trim(const string &str) {
  size_t start = 0;
  size_t siz = str.size();
  size_t idx = 0;
  while (idx < siz && str[idx] == ' ') idx++;
  start = idx;
  idx = siz - 1;
  while (idx > start && str[idx] == ' ') idx--;
  if (start > idx) return "";
  return str.substr(start, idx - start + 1);
}

double get_cur_time() {
  // second
  auto t1 = std::clock();
  return t1 / (double)CLOCKS_PER_SEC;
}