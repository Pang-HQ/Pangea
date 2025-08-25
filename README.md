# Pangea Programming Language

Pangea is a modern, statically typed, compiled programming language designed for clarity, safety, and performance. It combines clean, readable syntax with powerful features and compiles to native code via LLVM.

## 🚀 Quick Start

```bash
# Clone the repository
git clone github.com/Pang-HQ/Pangea
cd Pangea

# Build the compiler
python build.py

# Compile and run a Pangea program
./build/pangea.exe examples/small_test.pang
./a.exe
```

## ✨ Key Features

- **Clean Syntax**: Readable and familiar syntax inspired by modern languages
- **Static Typing**: Strong type system with explicit type annotations
- **Memory Safety**: Smart pointer types and compiler-assisted memory management
- **LLVM Backend**: Efficient native code generation for multiple platforms
- **Module System**: Import/export system with standard library support
- **Foreign Function Interface**: Seamless C interoperability
- **Cross-Platform**: Windows and Linux support

## 📖 Language Overview

### Hello World

```pangea
import "cstdlib/stdio"

fn main() -> i32 {
    printf("Hello, Pangea!\n")
    return 0
}
```

> Note: this uses the c standard library port, the pangea standard library will become the standard after full implementation.

### Types

- **Primitives**: `i32`, `u8`, `f64`, `bool`, `void`
- **Pointers**: `cptr T` (C-compatible pointers)
- **Strings**: Proper string implementation and C-style strings for compatability with C standard library
- **Arrays**: Dynamic and fixed-size arrays -> Not yet implemented
- **Custom Types**: Structs, enums -> Not yet implemented

### Memory Management

```pangea
// Stack allocation (default)
let value: i32 = 42

// Heap allocation via foreign functions
let ptr: cptr u8 = cast<cptr u8>(malloc(100))
free(cast<cptr void>(ptr))

// Type casting
let void_ptr = cast<cptr void>(ptr)
```

### Modules and Imports

```pangea
import "cstdlib/stdio"    // Standard C library

// Foreign function declarations
foreign fn malloc(size: i32) -> cptr void
foreign fn printf(format: cptr u8, args: ) -> i32
```

## 🛠️ Building the Compiler

### Prerequisites

- **C++20 Compiler**: GCC, Clang, or MSVC
- **LLVM 20**: Development libraries and headers
- **Python 3**: For the build script

### Build Instructions

```bash
# Using the build script (recommended)
python build.py [options]

# Available options:
python build.py --help          # Show help
python build.py --debug         # Debug build
python build.py --memory-check  # Enable memory safety checks
python build.py --clean         # Clean before building
python build.py --test          # Run tests after building
```

### Manual Build (Advanced)

```bash
mkdir build; cd build
g++ -std=c++20 -O2 -I/path/to/llvm/include \
    ../src/**/*.cpp \
    -lLLVM-20 -lole32 -lshell32 -ladvapi32 -luuid -ldbghelp \
    -o pangea.exe
```

## 🎯 Using the Compiler

### Command Line Interface

```bash
# Compile to executable
./pangea input.pang -o output.exe

# Debug options
./pangea --tokens input.pang     # Show lexer output
./pangea --ast input.pang        # Show parser output
./pangea --llvm input.pang       # Output LLVM IR

# Compilation options
./pangea --verbose input.pang    # Verbose compilation
./pangea --no-stdlib input.pang  # Skip auto-import standard library
./pangea --no-builtins input.pang # Skip builtin functions (deprecated - will be removed soon)

# Help
./pangea --help
```

### Example Programs

The `examples/` directory contains sample programs:

- `comprehensive_test.pang` - File I/O, memory management, and C interop

## 🏗️ Architecture

```
Source Code (.pang)
    ↓
Lexer (Tokenization)
    ↓
Parser (AST Generation)
    ↓
Semantic Analyzer (Type Checking)
    ↓
LLVM IR Generator
    ↓
LLVM Optimizer & Linker
    ↓
Native Executable
```

### Project Structure

```
src/
├── lexer/          # Tokenization and lexical analysis
├── parser/         # Recursive descent parser and AST
├── ast/            # AST node definitions and visitors
├── semantic/       # Type checking and semantic analysis
├── codegen/        # LLVM IR code generation
├── builtins/       # Built-in functions and types
├── stdlib/         # Standard library implementations
└── utils/          # Error reporting and utilities

stdlib/             # Pangea standard library modules
examples/           # Example programs
tests/              # Test suite
```

## 📊 Project Status

### ✅ Implemented

- **Lexer**: Complete tokenization with error reporting
- **Parser**: Recursive descent parser with full AST generation
- **Type System**: Basic type checking and inference
- **Code Generation**: LLVM IR generation and compilation
- **Module System**: Import/export with dependency resolution
- **Foreign Functions**: C interoperability layer
- **Standard Library**: Core I/O and C standard library bindings

### 🚧 In Progress

- Advanced type features (generics, traits)
- Enhanced error messages and diagnostics
- Optimization passes
- Full integration with C standard library, windows api, unistd, and mac
- Pangea standard library

### 📋 Planned

- Pattern matching
- Concurrency primitives
- Package manager
- IDE integration
- Documentation generator

## 🤝 Contributing

Pangea is in active development. Contributions are welcome!

### Development Setup

1. Fork the repository
2. Set up the development environment
3. Run tests: `python build.py --test`
4. Make your changes
5. Submit a pull request

### Coding Standards

- C++20 features encouraged
- Follow existing code style
- Add tests for new features
- Update documentation

## 📞 Contact

Visit us at www.panghq.com/contact for any inquires, suggestions or support.

---

*Pangea - The future of programming.*
