#include <doctest/doctest.h>
#include "onepunch.h"
#include <algorithm>
#include <sstream>
#include <iostream>

TEST_CASE("Find Solution Test") {
    std::srand(0);
    auto input_regs = ParseInputRegs({"r8"});
    auto must_control_list = ParseMustControlRegs({"rdi:1"});
    OnePunch onepunch("/lib/x86_64-linux-gnu/libc.so.6", input_regs.value(), must_control_list.value(), 1);

    std::stringstream buffer;
    std::streambuf * old = std::cout.rdbuf(buffer.rdbuf());

    onepunch.Run();

    std::cout.rdbuf(old);

    std::string captured_output = buffer.str();
    std::string solution_output;

    std::istringstream ss(captured_output);
    std::string line;
    bool in_solution = false;
    while (std::getline(ss, line)) {
        if (line.find("Solution found!") != std::string::npos) {
            in_solution = true;
        }
        if (in_solution) {
            if (line.find("after minimize:") != std::string::npos) {
                break;
            }
            solution_output += line + "\n";
        }
    }

    std::string expected_solution =
R"(Solution found!
  167db7:	mov  rax, QWORD PTR [r8+0x38]
  167dbb:	mov  rdi, r8
  167dbe:	call  QWORD PTR [rax+0x20]
------
  15f4df:	lea  r12, [rax+0x23b0]
  15f4e6:	xor  esi, esi
  15f4e8:	mov  DWORD PTR [rax+0x23b0], 0x1
  15f4f2:	mov  rax, QWORD PTR [rax+0x23b8]
  15f4f9:	mov  rdi, r12
  15f4fc:	call  QWORD PTR [rax+0x28]
------
  1136df:	mov  rdx, QWORD PTR [rax+0xb0]
  1136e6:	call  QWORD PTR [rax+0x88]
------
  16a063:	mov  rbx, rdi
  16a066:	sub  rsp, 0x18
  16a06a:	mov  rbp, QWORD PTR [rdi+0x48]
  16a06e:	mov  rax, QWORD PTR [rbp+0x18]
  16a072:	lea  r13, [rbp+0x10]
  16a076:	mov  DWORD PTR [rbp+0x10], 0x0
  16a07d:	mov  rdi, r13
  16a080:	call  QWORD PTR [rax+0x28]
------
  8b7c4:	mov  rdx, r8
  8b7c7:	mov  rsi, r13
  8b7ca:	mov  rdi, rbp
  8b7cd:	call  QWORD PTR [rax+0x78]
------
  11622b:	mov  rdx, QWORD PTR [rbx+0x38]
  11622f:	mov  rdi, QWORD PTR [rbx+0x18]
  116233:	lea  rcx, [rbx+0x28]
  116237:	mov  rsi, r12
  11623a:	mov  edx, DWORD PTR [rdx+rax*1]
  11623d:	call  QWORD PTR [rbx+0x40]
------
)";

    std::cout << solution_output << std::endl;
    CHECK(solution_output == expected_solution);
}

