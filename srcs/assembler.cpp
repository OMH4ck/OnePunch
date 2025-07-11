#include "assembler.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <map>

#include "asmutils.h"
#include "utils.h"

namespace onepunch {

namespace {

bool IsInteresting(const Operand& operand) {
  for (const auto& each : operand.reg_list_) {
    if (each.first > REG_64_START && each.first < REG_64_END) {
      return each.first != REG_RIP;
    }
  }
  return false;
}

bool IsHarmful(OPCODE opcode) {
  return opcode == OP_IMUL || opcode == OP_SYSCALL || opcode == OP_INT3 ||
         opcode == OP_NONE;
}

bool IsUnwantedInstructions(OPCODE opcode) {
  const OPCODE unwanted[] = {
      OP_NOP,    OP_SFENCE, OP_SAR,  OP_XOR,     OP_ADD,    OP_SUB,
      OP_MUL,    OP_DIV,    OP_ROR,  OP_BSWAP,   OP_MOVAPS, OP_MOVDQA,
      OP_MOVNTDQ, OP_SHL,    OP_SHR,  OP_NONE};
  for (const auto& each : unwanted) {
    if (opcode == each) {
      return true;
    }
  }
  return false;
}

}  // namespace

std::vector<InstrPtr> Assembler::GetDisasmCode(const std::string& filename) {
  std::array<char, 256> buf;
  std::string cmd("objdump -M intel --no-show-raw-insn -d " + filename);
  std::vector<InstrPtr> res;
  std::string disasm;

  auto pipe = popen(cmd.c_str(), "r");
  if (!pipe) {
    std::cout << "error in disasm" << std::endl;
    exit(-1);
  }

  while (fgets(buf.data(), 256, pipe) != NULL) {
    disasm += buf.data();
  }

  pclose(pipe);

  std::stringstream ss(disasm);
  std::string line;
  char useless[2] = {'#', '<'};
  while (getline(ss, line, '\n')) {
    for (int i = 0; i < 2; i++) {
      auto pos = line.find(useless[i]);
      if (pos == std::string::npos) continue;
      line = line.substr(0, pos);
    }

    line = refine(line);

    auto offset_pos = line.find(":\t");
    if (offset_pos == std::string::npos) continue;
    auto offset_str = line.substr(0, offset_pos);
    auto inst_str = line.substr(offset_pos + 2);
    char* endptr;
    unsigned long offset = strtol(offset_str.c_str(), &endptr, 16);

    auto opcode_pos = inst_str.find(" ");
    if (opcode_pos == std::string::npos) continue;
    auto opcode_str = inst_str.substr(0, opcode_pos);
    auto operand_str = inst_str.substr(opcode_pos + 1);

    auto inst_ptr =
        std::make_shared<Instruction>(offset, opcode_str, operand_str);
    inst_ptr->original_inst_ = line;
    res.push_back(inst_ptr);
  }

  return res;
}

std::vector<SegmentPtr> Assembler::GetCallSegment(
    std::vector<InstrPtr>& insts) {
  int start = 0;
  const int size = insts.size();
  std::map<std::string, size_t> duplicate_helper = {};

  std::vector<SegmentPtr> result;

  for (int idx = 0; idx < size; idx++) {
    auto inst = insts[idx];
    if (inst->opcode_ == OPCODE::OP_CALL || inst->opcode_ == OPCODE::OP_JMP) {
      if (!inst->op_dst_.has_value() ||
          IsInteresting(inst->op_dst_.value()) == false ||
          inst->op_dst_->contain_reg(REG_RIP)) {
        continue;
      }
      start = idx - 1;
      while (start >= 0) {
        auto tmp_inst = insts[start];
        if (tmp_inst->opcode_ == OP_CALL || tmp_inst->opcode_ == OP_JCC ||
            tmp_inst->opcode_ == OP_RET || tmp_inst->opcode_ == OP_JMP ||
            IsHarmful(tmp_inst->opcode_)) {
          break;
        }
        start -= 1;
      }
      start += 1;
      while (start < idx + 1) {
        auto tmp_inst = insts[start];
        if (IsUnwantedInstructions(tmp_inst->opcode_) ||
            (tmp_inst->operation_length_ != kQWORD) ||
            (tmp_inst->op_dst_ &&
             (tmp_inst->op_dst_->contain_reg(REG_CR3) ||
              tmp_inst->op_dst_->contain_reg(REG_CR4) ||
              tmp_inst->op_dst_->contain_reg(REG_RIP) ||
              tmp_inst->op_dst_->contain_segment_reg())) ||
            (tmp_inst->op_src_ &&
             (tmp_inst->op_src_->contain_reg(REG_CR4) ||
              tmp_inst->op_src_->contain_reg(REG_CR4) ||
              tmp_inst->op_src_->contain_reg(REG_RIP) ||
              tmp_inst->op_src_->contain_segment_reg()))) {
          start += 1;
          continue;
        }
        break;
      }

      std::vector<InstrPtr> inst_list;
      for (int tmp_idx = start; tmp_idx < idx + 1; tmp_idx++)
        inst_list.push_back(insts[tmp_idx]);

      if (inst_list.size() <= 1) {
        continue;
      }

      SegmentPtr seg = std::make_shared<Segment>(inst_list);

      std::string tmp_asm = seg->to_string(false);

      if (duplicate_helper.count(tmp_asm)) {  // contains
        continue;
      }
      duplicate_helper[tmp_asm] = result.size();
      result.push_back(seg);
    }
  }
  return result;
}

}  // namespace onepunch