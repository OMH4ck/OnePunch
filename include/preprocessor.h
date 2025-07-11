#ifndef ONEPUNCH_PREPROCESSOR_H_
#define ONEPUNCH_PREPROCESSOR_H_

#include <map>
#include <vector>

#include "asmutils.h"

class Preprocessor {
 public:
  std::map<unsigned long, std::vector<SegmentPtr>> result_;
  std::map<SegmentPtr, unsigned long> test_;

  void process(const std::vector<SegmentPtr>& segments);
};

#endif  // ONEPUNCH_PREPROCESSOR_H_