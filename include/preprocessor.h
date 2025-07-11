#ifndef ONEPUNCH_PREPROCESSOR_H_
#define ONEPUNCH_PREPROCESSOR_H_

#include <map>
#include <vector>

#include "asmutils.h"

class Preprocessor {
 public:
  static std::map<unsigned long, std::vector<SegmentPtr>> result_;
  static std::map<SegmentPtr, unsigned long> test_;

  static void process(const std::vector<SegmentPtr>& segments);
};

#endif  // ONEPUNCH_PREPROCESSOR_H_