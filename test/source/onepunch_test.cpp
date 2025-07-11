#include <doctest/doctest.h>
#include "onepunch.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <string>
#include <cctype>
#include <regex>
#include "asmutils.h"

extern bool RECORD_MEM;

// Function to remove all whitespace from a string
std::string remove_whitespace(std::string str) {
    str.erase(std::remove_if(str.begin(), str.end(), ::isspace), str.end());
    return str;
}

// Function to replace memid values with a placeholder
std::string replace_memid(std::string str) {
    std::regex memid_regex("memid:0x[0-9a-f]+");
    str = std::regex_replace(str, memid_regex, "memid:0xDEADBEEF");
    std::regex memid_val_regex("\\(MEM_VALUE,0x[0-9a-f]+\\(memid\\)\\)");
    str = std::regex_replace(str, memid_val_regex, "(MEM_VALUE,0xDEADBEEF(memid))");
    return str;
}

TEST_CASE("Find and Minimize Solution Test") {
    g_visited.clear();
    std::srand(0);
    auto input_regs = ParseInputRegs({"r8"});
    auto must_control_list = ParseMustControlRegs({"rdi:1"});
    
    OnePunch onepunch("/lib/x86_64-linux-gnu/libc.so.6", input_regs.value(), must_control_list.value(), 1);

    // Capture original Run output
    std::stringstream buffer_run;
    std::streambuf * old_run = std::cout.rdbuf(buffer_run.rdbuf());
    onepunch.Run();
    std::cout.rdbuf(old_run);
    std::string captured_output_run = buffer_run.str();

    // Check the "Find Solution" part
    std::string solution_output;
    std::istringstream ss(captured_output_run);
    std::string line;
    bool in_solution = false;
    while (std::getline(ss, line)) {
        if (line.find("Solution found!") != std::string::npos) {
            in_solution = true;
        }
        if (in_solution) {
            if (line.find("after minimize:") != std::string::npos) {
                //break;
            }
            solution_output += line + "\n";
        }
    }

    std::string expected_solution =
R"(Solution found!
  167db7: mov  rax, QWORD PTR [r8+0x38]
  167dbb: mov  rdi, r8
  167dbe: call  QWORD PTR [rax+0x20]
------
  15f4df: lea  r12, [rax+0x23b0]
  15f4e6: xor  esi, esi
  15f4e8: mov  DWORD PTR [rax+0x23b0], 0x1
  15f4f2: mov  rax, QWORD PTR [rax+0x23b8]
  15f4f9: mov  rdi, r12
  15f4fc: call  QWORD PTR [rax+0x28]
------
  1136df: mov  rdx, QWORD PTR [rax+0xb0]
  1136e6: call  QWORD PTR [rax+0x88]
------
  16a063: mov  rbx, rdi
  16a066: sub  rsp, 0x18
  16a06a: mov  rbp, QWORD PTR [rdi+0x48]
  16a06e: mov  rax, QWORD PTR [rbp+0x18]
  16a072: lea  r13, [rbp+0x10]
  16a076: mov  DWORD PTR [rbp+0x10], 0x0
  16a07d: mov  rdi, r13
  16a080: call  QWORD PTR [rax+0x28]
------
  8b7c4: mov  rdx, r8
  8b7c7: mov  rsi, r13
  8b7ca: mov  rdi, rbp
  8b7cd: call  QWORD PTR [rax+0x78]
------
  11622b: mov  rdx, QWORD PTR [rbx+0x38]
  11622f: mov  rdi, QWORD PTR [rbx+0x18]
  116233: lea  rcx, [rbx+0x28]
  116237: mov  rsi, r12
  11623a: mov  edx, DWORD PTR [rdx+rax*1]
  11623d: call  QWORD PTR [rbx+0x40]
------
after minimize:
  167db7:       mov  rax, QWORD PTR [r8+0x38]
  167dbb:       mov  rdi, r8
  167dbe:       call  QWORD PTR [rax+0x20]
------
  15f4df:       lea  r12, [rax+0x23b0]
  15f4e6:       xor  esi, esi
  15f4e8:       mov  DWORD PTR [rax+0x23b0], 0x1
  15f4f2:       mov  rax, QWORD PTR [rax+0x23b8]
  15f4f9:       mov  rdi, r12
  15f4fc:       call  QWORD PTR [rax+0x28]
------
  16a063:       mov  rbx, rdi
  16a066:       sub  rsp, 0x18
  16a06a:       mov  rbp, QWORD PTR [rdi+0x48]
  16a06e:       mov  rax, QWORD PTR [rbp+0x18]
  16a072:       lea  r13, [rbp+0x10]
  16a076:       mov  DWORD PTR [rbp+0x10], 0x0
  16a07d:       mov  rdi, r13
  16a080:       call  QWORD PTR [rax+0x28]
------
  11622b:       mov  rdx, QWORD PTR [rbx+0x38]
  11622f:       mov  rdi, QWORD PTR [rbx+0x18]
  116233:       lea  rcx, [rbx+0x28]
  116237:       mov  rsi, r12
  11623a:       mov  edx, DWORD PTR [rdx+rax*1]
  11623d:       call  QWORD PTR [rbx+0x40]
------

Memory list:
memid:0x5c8, relation: r8
        Available:[ [-INF,0x38], [0x40,INF] ]
        content:[ [0x38:(MEM_VALUE,0x5c9(memid))] ]
memid:0x5c9, relation: *(r8+0x38)+0x23b0+0x28
        Available:[ [-INF,0x20], [0x28,0x23b0], [0x23b4,0x23b8], [0x23c0,0x23c8], [0x23d0,0x23e8], [0x2400,INF] ]
        content:[ [0x20:(CALL_VALUE,0x15f4df(inst))], [0x23b8:(MEM_VALUE,0x5ca(memid))], [0x23c8:(MEM_VALUE,0x5ce(memid))], [0x23e8:(MEM_VALUE,0x5cd(memid))], [0x23f0:(CALL_VALUE,Target RIP)], [0x23f8:(MEM_VALUE,0x5cb(memid))] ]
memid:0x5ca, relation: *(*(r8+0x38)+0x23b8)
        Available:[ [-INF,0x28], [0x30,INF] ]
        content:[ [0x28:(CALL_VALUE,0x16a063(inst))] ]
memid:0x5cb, relation: *(*(r8+0x38)+0x23b0+0x48)+0x10
        Available:[ [-INF,0x10], [0x14,0x18], [0x20,INF] ]
        content:[ [0x18:(MEM_VALUE,0x5cc(memid))] ]
memid:0x5cc, relation: *(*(*(r8+0x38)+0x23b0+0x48)+0x18)
        Available:[ [-INF,0x28], [0x30,INF] ]
        content:[ [0x28:(CALL_VALUE,0x11622b(inst))] ]
memid:0x5cd, relation: *(*(r8+0x38)+0x23b0+0x38)
        Available:[ [-INF,INF] ]
memid:0x5ce, relation: *(*(r8+0x38)+0x23b0+0x18)
        Available:[ [-INF,INF] ]

Final state
r8:     memid:0x5c8, relation: r8
        Available:[ [-INF,0x38], [0x40,INF] ]
        content:[ [0x38:(MEM_VALUE,0x5c9(memid))] ]
r12:    memid:0x5c9, relation: *(r8+0x38)+0x23b0+0x28
        Available:[ [-INF,0x20], [0x28,0x23b0], [0x23b4,0x23b8], [0x23c0,0x23c8], [0x23d0,0x23e8], [0x2400,INF] ]
        content:[ [0x20:(CALL_VALUE,0x15f4df(inst))], [0x23b8:(MEM_VALUE,0x5ca(memid))], [0x23c8:(MEM_VALUE,0x5ce(memid))], [0x23e8:(MEM_VALUE,0x5cd(memid))], [0x23f0:(CALL_VALUE,Target RIP)], [0x23f8:(MEM_VALUE,0x5cb(memid))] ]
rbx:    memid:0x5c9, relation: *(r8+0x38)+0x23b0+0x28
        Available:[ [-INF,0x20], [0x28,0x23b0], [0x23b4,0x23b8], [0x23c0,0x23c8], [0x23d0,0x23e8], [0x2400,INF] ]
        content:[ [0x20:(CALL_VALUE,0x15f4df(inst))], [0x23b8:(MEM_VALUE,0x5ca(memid))], [0x23c8:(MEM_VALUE,0x5ce(memid))], [0x23e8:(MEM_VALUE,0x5cd(memid))], [0x23f0:(CALL_VALUE,Target RIP)], [0x23f8:(MEM_VALUE,0x5cb(memid))] ]
rbp:    memid:0x5cb, relation: *(*(r8+0x38)+0x23b0+0x48)+0x10
        Available:[ [-INF,0x10], [0x14,0x18], [0x20,INF] ]
        content:[ [0x18:(MEM_VALUE,0x5cc(memid))] ]
rax:    memid:0x5cc, relation: *(*(*(r8+0x38)+0x23b0+0x48)+0x18)
        Available:[ [-INF,0x28], [0x30,INF] ]
        content:[ [0x28:(CALL_VALUE,0x11622b(inst))] ]
r13:    memid:0x5cb, relation: *(*(r8+0x38)+0x23b0+0x48)+0x10
        Available:[ [-INF,0x10], [0x14,0x18], [0x20,INF] ]
        content:[ [0x18:(MEM_VALUE,0x5cc(memid))] ]
rdi:    memid:0x5ce, relation: *(*(r8+0x38)+0x23b0+0x18)
        Available:[ [-INF,INF] ]
rcx:    memid:0x5c9, relation: *(r8+0x38)+0x23b0+0x28
        Available:[ [-INF,0x20], [0x28,0x23b0], [0x23b4,0x23b8], [0x23c0,0x23c8], [0x23d0,0x23e8], [0x2400,INF] ]
        content:[ [0x20:(CALL_VALUE,0x15f4df(inst))], [0x23b8:(MEM_VALUE,0x5ca(memid))], [0x23c8:(MEM_VALUE,0x5ce(memid))], [0x23e8:(MEM_VALUE,0x5cd(memid))], [0x23f0:(CALL_VALUE,Target RIP)], [0x23f8:(MEM_VALUE,0x5cb(memid))] ]
rsi:    memid:0x5c9, relation: *(r8+0x38)+0x23b0+0x28
        Available:[ [-INF,0x20], [0x28,0x23b0], [0x23b4,0x23b8], [0x23c0,0x23c8], [0x23d0,0x23e8], [0x2400,INF] ]
        content:[ [0x20:(CALL_VALUE,0x15f4df(inst))], [0x23b8:(MEM_VALUE,0x5ca(memid))], [0x23c8:(MEM_VALUE,0x5ce(memid))], [0x23e8:(MEM_VALUE,0x5cd(memid))], [0x23f0:(CALL_VALUE,Target RIP)], [0x23f8:(MEM_VALUE,0x5cb(memid))] ]
)";
    std::string cleaned_actual_output = remove_whitespace(replace_memid(solution_output));
    std::string cleaned_expected_output = remove_whitespace(replace_memid(expected_solution));
    REQUIRE_EQ(cleaned_actual_output, cleaned_expected_output);
}
