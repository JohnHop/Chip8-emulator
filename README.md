# Chip8 emulator

Just another CHIP-8 emulator written in C++ as a pet-project idea.
Inspired on [Austin Morlan nice work](https://austinmorlan.com/posts/chip8_emulator/).

Writing this emulator could be a good idea to learn how a microprocessor works. So I tried and it was fun.
As CHIP-8 is a simple virtual machine, this was a quite easy project that helped me to put in practise a little of what I learned about some specific embedded-like courses in my university.

## Usage
Just type
```
> ./chip8 <scale> <delay> <path to ROM file>
```

For example, you can use the [corax89 test rom](https://github.com/corax89/chip8-test-rom) with
```
> ./chip8 10 1 ./test_opcode.ch8
```

## How is done?
Briefly, a CHIP-8 program is just a sequence of 2 bytes fixed-length assembly instructions like this (representation is hexadecimal)
```
6000
6100
620A
F229
D015
F000
```
We need someone (the Control Unit) that (1) _fetch_ the instruction, (2) _decode_ (operation assembly) and finally (3) _execute_ it.
Also, we need the data structure that represents some registers. 

So, what we have is just a C++ class with some member variables like `pc` (Program Counter), `sp` (Stack Pointer), a set of general purpose registers (from V0 to VF) and so on...
```c++
uint8_t registers[16];  //V0, V1, V2... VF
uint16_t index;
uint16_t pc;
uint8_t sp;
```

The CHIP-8 program is loaded in main memory. We can implement it simply using an uint8_t array of 4096 byte (a word is 1 byte length).
```c++
uint8_t memory[4096];
```

Finally, to implement Von Neumann cycle, we just write this small and quite self explanatory `cycle()` member function 
```c++
this->opcode = (memory[pc] << 8u) | memory[pc + 1]; //fetch
this->pc += 2;  //pc increment

(this->*(table[(opcode & 0xF000u) >> 12u]))();  //decode and execute
```
Each opcode has its equivalent member function, that is a void function with no arguments because operands are decoded inside it. 

### Decode and Execute
The last instruction of `cycle()` can be quite obscure for newbies. Let's reveal the magic under it.

A CHIP-8 instruction is something like
```
6xkk  # or LD Vx, kk
```
where
- 6 is a costant that helps recognizing the instruction
- x is the index of a general purpose register (Vx)
- kk is a byte in hexadecimal representation

_xkk_ is parametrized. The only way to recognize this instruction is the first digit. Fortunately, there are no other instructions that start with the digit **6**.

Opcodes that start with *1, 2, 3, 4, 5, 6, 7, 9, A, B, C, D* are unique.
There are more than one opcode that start with **8**, but they differ for the last digit and it can be *0, 1, 2, 3, 4, 5, 6, 7, E*.

There are more than one opcode that start with **0**, but they differ for the last digit and it can be *0* or *E*.

There are more than one opcode that start with **E**, but they differ for the last digit and it can be *1* or *E*.

Finally, there are more than one opcode that start with **F**, but they differ for the last two digits and they can be *07, 0A, 15, 18, 1E, 29, 33, 55, 65*.

A way to implement a dispatcher that executes the correct function based on a opcode is an array of pointer functions, one for each possible opcode.
Let's start distinguish them though first digit. This array of pointer function is 16 elements length
```c++
Chip8Fun table[0xF + 1];

table[0x0] = &Chip8::Table0;  //need a second stage. There are more than one opcode that start with zero
table[0x1] = &Chip8::OP_1nnn;
table[0x2] = &Chip8::OP_2nnn;
table[0x3] = &Chip8::OP_3xkk;
table[0x4] = &Chip8::OP_4xkk;
table[0x5] = &Chip8::OP_5xy0;
table[0x6] = &Chip8::OP_6xkk;
table[0x7] = &Chip8::OP_7xkk;
table[0x8] = &Chip8::Table8;  //to second stage: an array of function pointer for opcode that start with 8
table[0x9] = &Chip8::OP_9xy0;
table[0xA] = &Chip8::OP_Annn;
table[0xB] = &Chip8::OP_Bnnn;
table[0xC] = &Chip8::OP_Cxkk;
table[0xD] = &Chip8::OP_Dxyn;
table[0xE] = &Chip8::TableE;  //to second stage
table[0xF] = &Chip8::TableF;  //to second stage
```

The C++ instruction in `cycle()`
```c++
(this->*(table[(opcode & 0xF000u) >> 12u]))();
```
executes the right function based on first digit of instruction. If it is something like 0, 8, E, F we need a second dispatcher aka another array of pointer functions.

When opcode start with 0, 8 and E, we need to distinguish base on last digit
```c++
void Table0() { (this->*(table0[opcode & 0x000Fu]))(); }
void Table8() { (this->*(table8[opcode & 0x000Fu]))(); }
void TableE() { (this->*(tableE[opcode & 0x000Fu]))(); }
```

meanwhile with F-class function we need the last two digits
```c++
void TableF() { (this->*(tableF[opcode & 0x00FFu]))(); }
```

As you can see by the definitions above, the second dispatcher is similar to the instruction inside the `cycle()` function. Just need to initialize the arrays inside the Chip8 constructor.

## How to compile?
You need to install the SDL2 library.
Using CMake, this is a working example of `CMakeLists.txt` (partially auto-generated by vscode extension)
```c
cmake_minimum_required(VERSION 3.0.0)
project(chip8emulator VERSION 0.1.0)

include(CTest)
enable_testing()

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

add_executable(chip8 main.cpp chip8.cpp)
target_link_libraries(chip8 ${SDL2_LIBRARIES})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
```

## More informations and references
Austin Morlan [blog](https://austinmorlan.com/posts/chip8_emulator/).

Cowgod's Chip-8 Technical Reference v1.0 [link](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM).

Wikipedia CHIP-8 [article](https://en.wikipedia.org/wiki/CHIP-8).