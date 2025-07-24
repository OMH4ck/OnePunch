#include "preprocessor.h"

#include "onepunch.h"

using namespace std;

void Preprocessor::process(const vector<SegmentPtr>& segments) {
  for (const auto& i : segments) {
    auto res = ConstraintAnalyzer::compute_constraint(i);
    test_[i] = res;
  }
}
