#include "preprocessor.h"

#include "onepunch.h"

using namespace std;

map<unsigned long, vector<SegmentPtr>> Preprocessor::result_;
map<SegmentPtr, unsigned long> Preprocessor::test_;

void Preprocessor::process(const vector<SegmentPtr>& segments) {
  for (const auto& i : segments) {
    auto res = compute_constraint(i);
    test_[i] = res;
  }
}
