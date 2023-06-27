# ARM Assembler

This repository contains an ARM Assembler project implemented in C. The assembler is designed to generate an executable binary file or hex file from a given assembly source file. It includes a scanner to scan the assembly file for specific tokens, a parser to construct a parse tree from the scanned tokens, and a code generator to generate hex code based on the parse tree.

## Project Overview

The ARM Assembler project focuses on the following key functionalities:

- **Scanning**: The scanner module reads the assembly source file and tokenizes it by identifying specific tokens such as operation codes, registers, and operands.

- **Parsing**: The parser module constructs a parse tree from the scanned tokens, representing the syntactic structure of the assembly code.

- **Code Generation**: The code generator module traverses the parse tree and generates the corresponding hex code instructions based on the syntax and semantics of the assembly language.
