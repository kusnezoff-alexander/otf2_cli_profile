# AGENTS.md - Agent Coding Guidelines for OTF-Profiler

This file provides guidelines for agentic coding agents working on this codebase.

## Project Overview

OTF-Profiler is a C++20 CMake-based profiling tool for analyzing OTF2 (Open Trace Format 2) traces. It extracts performance data from traces and can output results in various formats (Cube, JSON).

## Build System

- **Build Tool**: CMake (minimum 3.10)
- **C++ Standard**: C++20 for library (`otf-profiler-lib`), C++11 for executable
- **Working Directory**: Always use the `build/` directory for builds

## Build/Lint/Test Commands

### Building the Project

```bash
# Configure and build (from project root)
cd build
cmake ..
make -j

# Build specific targets
make otf-profiler          # Build CLI tool
make otf-profiler-lib     # Build static library
make my_tests             # Build unit tests
```

### Running Tests

```bash
# Run all tests (CTest)
cd build
make test
# or
ctest

# Run unit tests specifically
cd build
make my_tests
./my_tests

# Run a single unit test with GTest filter
cd build
./my_tests --gtest_filter=AccessPattern.Simple

# Run integration tests (BATS) - requires scorep and MPI
cd build
make run_tests
```

### Loading Dependencies (HPC Systems)

On systems with modules/spack:
```bash
# Load MPI (if using environment modules)
ml load mpi

# Load scorep and its dependencies via spack
source /home/alex/.local/bin/spack/share/spack/setup-env.sh
spack load scorep@8.4
```

### Code Formatting

```bash
# Format C++ code (uses .clang-format - Google-based style)
clang-format -i src/*.cpp src/**/*.cpp include/*.h include/**/*.h

# Check formatting without modifying
clang-format --dry-run -Werror src/*.cpp
```

## Code Style Guidelines

### Formatting (per .clang-format)

- **Style**: Google with customizations
- **Column Limit**: 120 characters
- **Indent Width**: 4 spaces
- **Tab Usage**: Never (use spaces)
- **Language**: C++

### Naming Conventions

- **Types/Classes**: `PascalCase` (e.g., `SystemTree`, `RegionDefinition`)
- **Functions/Methods**: `snake_case` (e.g., `copy_reduced`, `detect_local_access_pattern`)
- **Variables**: `snake_case` (e.g., `location_id`, `num_ranks`)
- **Constants/Enums**: `SCREAMING_SNAKE_CASE` or `PascalCase` for enum values
- **Namespaces**: `snake_case` (e.g., `definitions`, `access_pattern_detection`)

### Header Files

- Use `#pragma once` for include guards
- Include order (alphabetical within groups):
  1. Associated header (e.g., definitions.h in definitions.cpp)
  2. C++ standard library (`<algorithm>`, `<iostream>`, etc.)
  3. External libraries (`<otf2/...>`, Boost, etc.)
  4. Project internal headers (`"utils.h"`, `"reader/tracereader.h"`)
- Keep headers self-contained when possible

### Code Patterns

#### Namespaces
```cpp
namespace definitions {

// ... code ...

}  // namespace definitions
```

#### Structs (POD-like)
```cpp
struct Region {
    std::string    name;
    paradigm_id_t   paradigm_id;
    uint32_t       begin_source_line;
    uint32_t       end_source_line;
    std::string    file_name;
};
```

#### Error Handling
- Return `1` for errors, `0` for success
- Use `std::cerr` for error messages with `"ERROR: "` prefix
- For MPI builds, use `MPI_Abort(MPI_COMM_WORLD, 1)` on fatal errors

```cpp
int error() {
#ifdef OTFPROFILER_MPI
    MPI_Abort(MPI_COMM_WORLD, 1);
#endif
    return 1;
}

// Usage:
if (!alldata.params.parseCommandLine(argc, argv))
    return error();
```

#### Conditional Compilation
```cpp
#ifdef HAVE_JSON
#include "create_json.h"
#endif  // HAVE_JSON
```

### Dependencies

The project depends on:
- **OTF2**: Trace format library (auto-downloaded if not found)
- **Boost** (1.71+): `system`, `filesystem` components
- **RapidJSON**: Optional, for JSON output
- **Cubelib**: Optional, for Cube output
- **MPI**: Optional, for parallel trace processing

### Project Structure

```
src/
  analysis/         # Analysis algorithms (e.g., access_pattern_detection.cpp)
  reader/          # Trace readers (OTF2Reader.cpp, tracereader.cpp)
  output/          # Output formatters (create_json.cpp)
  *.cpp            # Core implementation files

include/           # Header files (mirrors src/ structure)
tests/
  unittests/       # GTest-based unit tests
  integration/     # BATS integration tests
cmake/             # CMake modules and configuration
docs/              # Documentation (Quarto-based)
```

### Writing Tests

#### Unit Tests (GTest)
```cpp
#include <gtest/gtest.h>

TEST(TestSuite, TestName) {
    // Arrange
    // Act
    // Assert
    EXPECT_EQ(actual, expected);
}
```

#### Integration Tests (BATS)
- Place in `tests/integration/`
- Use BATS syntax for test cases

## Common Tasks

### Adding a New Source File

1. Add source to `SOURCE_FILES` list in `CMakeLists.txt`
2. Add header to appropriate `include_directories` path
3. Update main executable or library target as needed

### Adding Unit Tests

1. Create test file in `tests/unittests/`
2. Add to `my_tests` target in `cmake/GTest.cmake`

### Running the Profiler

```bash
./build/otf-profiler --help
./build/otf-profiler <trace_directory> [options]
```

## Important Notes

- The project uses `-Wno-error` to treat warnings as non-fatal
- Always rebuild after adding new files: `make clean && make -j`
- Integration tests require `scorep` and `mpirun` to be installed
