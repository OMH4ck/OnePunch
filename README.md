[![Standalone](https://github.com/OMH4ck/OnePunch/actions/workflows/standalone.yml/badge.svg)](https://github.com/OMH4ck/OnePunch/actions/workflows/standalone.yml)
[![Install](https://github.com/OMH4ck/OnePunch/actions/workflows/install.yml/badge.svg)](https://github.com/OMH4ck/OnePunch/actions/workflows/install.yml)
[![Style](https://github.com/OMH4ck/OnePunch/actions/workflows/style.yml/badge.svg)](https://github.com/OMH4ck/OnePunch/actions/workflows/style.yml)
[![Ubuntu](https://github.com/OMH4ck/OnePunch/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/OMH4ck/OnePunch/actions/workflows/ubuntu.yml)

# OnePunch

`OnePunch` is a tool for finding gadgets to achieve arbitrary code execution. Given a list of registers that you control, a list of registers that you want to control and a binary file, `OnePunch` will find a gadget chain that will allow you to control the target registers.
Currently `OnePunch` only supports `x86_64` architecture.

For example, if you have control the `r8`, which you can set its value to the address of some buffer you fully control, and you want to control `rdi` because you want to call `system("sh")`, you can use `OnePunch` to make your life easier:
```bash
./build/OnePunch -i r8 -c rdi:1  -f /lib/x86_64-linux-gnu/libc.so.6 

Result:
  167f47:       mov  rax, QWORD PTR [r8+0x38]
  167f4b:       mov  rdi, r8
  167f4e:       call  QWORD PTR [rax+0x20]
------
  174fc4:       mov  rdi, QWORD PTR [rdi+0x8]
  174fc8:       push  0x0
  174fca:       lea  rcx, [rsi+0x3a0]
  174fd1:       push  0x2
  174fd3:       call  QWORD PTR [rax+0x340]
------
```

## Build
```bash
cmake -Sstandalone -Bbuild
cmake --build build -j4
./build/OnePunch
```

## Usage
```bash
./build/OnePunch -h

Usage: OnePunch [-h] --input VAR... --control VAR... --file VAR [--level VAR]

Optional arguments:
  -h, --help    shows help message and exits 
  -v, --version prints version information and exits 
  -i, --input   The registers that we control [nargs: 1 or more] [required]
  -c, --control The registers we want to control [nargs: 1 or more] [required]
  -f, --file    The binary file that we want to analyze [required]
  -l, --level   The search level [default: 1]
Example: ./OnePunch -i rdi rsi -c rsp:0 rbp:1 -f libc.so.6
1 means we want to completely control the value of the register, and 0 means we allow the register to be a pointer value as long as it can point to a buffer that we control.
```

## TODO
- [ ] Support specifying the range of input registers
- [ ] Support specifying the range of output registers
- [ ] Better print the results
- [ ] Support analyzing multiple binaries

## Note
The code is messy right now. 