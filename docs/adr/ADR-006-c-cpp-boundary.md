# ADR-006: C/C++ Boundary Structure

## Status
Accepted

## Context

The project uses both C and C++. The low-level HAL and protocol layers
are written in C for maximum portability and compatibility with embedded
toolchains. The higher-level application architecture uses C++. A clear
boundary must be defined between the two languages.

## Decision

Define a strict boundary where:

- C owns: HAL drivers, ring buffer, CRC, protocol serialization,
  all hardware-facing code, C-compatible data structures
- C++ owns: application architecture, state machines, task logic,
  resource abstractions, higher-level control

The boundary uses C-compatible ABI:
- C structs shared across the boundary (no C++ classes in headers
  included by C translation units)
- C enumerations (not enum class) for shared types
- extern "C" declarations for C functions called from C++
- No C++ exceptions, RTTI, or virtual dispatch in the C layer

## Alternatives Considered

**Pure C throughout**
Would maximize portability and simplicity. However, C++ provides
valuable tools for the application layer: RAII for resource management,
enum class for type safety, constexpr for compile-time computation,
and stronger type checking. Avoiding these without a good reason
is a missed opportunity.

**Pure C++ throughout**
Would allow a more uniform codebase. However, C++ features like
constructors, destructors, virtual dispatch, and exceptions add
overhead that is inappropriate in ISR context and low-level driver
code. Real embedded projects routinely use C for HAL and C++ for
application layers — following this convention makes the project
more representative of industry practice.

**C++ with extern "C" wrappers**
The chosen approach. C translation units include only C headers.
C++ translation units can include both C headers (wrapped in
extern "C") and C++ headers. The linker sees C-compatible symbols
at the boundary.

## Consequences

**Gained:**
- C HAL code compiles cleanly with strict C11 settings
- C++ application code uses modern C++17 features where appropriate
- Clear ownership — each layer knows what it is responsible for
- Mirrors real-world embedded software architecture
- C layer can be tested independently of C++ layer

**Trade-offs:**
- Requires discipline to maintain the boundary — C++ features must
  not leak into C headers
- Shared data structures must be C-compatible (no std::string,
  no std::vector, no templates in shared headers)
- Two sets of compiler flags must be maintained in CMake
