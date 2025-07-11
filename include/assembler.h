#ifndef ONEPUNCH_ASSEMBLER_H_
#define ONEPUNCH_ASSEMBLER_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "asmutils.h"

namespace onepunch {

class Assembler {
 public:
  Assembler() = default;

  std::vector<InstrPtr> GetDisasmCode(const std::string& filename);
  std::vector<SegmentPtr> GetCallSegment(std::vector<InstrPtr>& insts);
};

}  // namespace onepunch

#endif  // ONEPUNCH_ASSEMBLER_H_