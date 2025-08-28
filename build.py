import os
import subprocess
import sys

def main():
    # Parse command line arguments
    build_type = "Release"
    memory_check = "OFF"
    clean_build = False
    run_tests = False

    if len(sys.argv) > 1:
        for arg in sys.argv[1:]:
            if arg == "--debug":
                build_type = "Debug"
            elif arg == "--memory-check":
                memory_check = "ON"
            elif arg == "--clean":
                clean_build = True
            elif arg == "--test":
                run_tests = True
            elif arg == "--help":
                print("Pangea Compiler Build Script (Python)")
                print()
                print("Usage: python build.py [options]")
                print()
                print("Options:")
                print("  --debug         Build in debug mode (default: Release)")
                print("  --memory-check  Enable memory safety checking")
                print("  --clean         Clean build directory before building")
                print("  --test          Run tests after building")
                print("  --help          Show this help message")
                return
            else:
                print(f"Unknown option: {arg}")
                print("Use --help for usage information")
                return

    # Check for required tools
    if os.system("g++ --version"):
        print("[ERROR] g++ is required but not installed")
        sys.exit(1)

    # Set build directory
    build_dir = "build"

    # Clean build directory if requested
    if clean_build:
        if os.path.exists(build_dir):
            print(f"[INFO] Cleaning {build_dir} directory...")
            for root, dirs, files in os.walk(build_dir):
                for file in files:
                    os.remove(os.path.join(root, file))
            os.rmdir(build_dir)
        else:
            print(f"[INFO] {build_dir} directory does not exist, skipping clean")

    # Create build directory
    if not os.path.exists(build_dir):
        print(f"[INFO] Creating {build_dir} directory...")
        os.makedirs(build_dir)

    # Change to build directory
    os.chdir(build_dir)

    # Compiler flags based on build type
    if build_type == "Debug":
        compile_flags = ["-g", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic"]
    else:
        compile_flags = ["-O2", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic"]

    # Add memory check flag if enabled
    if memory_check == "ON":
        compile_flags.append("-DPANGEA_MEMORY_CHECK")

    # LLVM include directories and libraries
    llvm_dir = os.environ.get("LLVM_DIR", "C:/msys64/mingw64/include/llvm")
    llvm_include = os.path.join(llvm_dir, "include")
    # Automatically generate LLVM -l flags from libLLVM*.a files
    llvm_libs = ["-lLLVM-20.dll"]

    # System libraries for Windows
    other_libs = [
        "-lole32",
        "-lshell32",
        "-ladvapi32",
        "-luuid",
        "-ldbghelp"
    ]
    
    # Source files
    sources = [
        "../src/main.cpp",
        "../src/lexer/lexer.cpp",
        "../src/lexer/token.cpp",
        "../src/parser/parser.cpp",
        "../src/ast/ast_nodes.cpp",
        "../src/semantic/type_checker.cpp",
        "../src/codegen/llvm_codegen.cpp",
        "../src/utils/source_location.cpp",
        "../src/utils/unicode/unicode_escape.cpp",
        "../src/utils/error_reporter.cpp"
    ]

    # Compile command
    compile_cmd = ["g++"] + compile_flags + [f"-I{llvm_include}"] + sources + other_libs + llvm_libs + ["-o", "pangea.exe"]
    
    # Build the project
    print(f"[INFO] Building project in {build_type} mode...")
    result = subprocess.run(compile_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

    if result.returncode != 0:
        print("[ERROR] Build failed!")
        print(result.stderr)
        sys.exit(1)
    else:
        print("[SUCCESS] Build completed successfully!")

    # Run tests if requested
    if run_tests:
        print("[ERROR] Tests not implemented yet")
        sys.exit(1)

if __name__ == "__main__":
    main()
