# NA-HE & NA-HD

Core reference implementation of **Neighbor-Aware Hilbert Encoding (NA-HE)** and **Decoding (NA-HD)**. The algorithms reuse coordinate/code prefixes across consecutive inputs and use the three-point Dynamic Updating Mechanism (DUM) to keep cached Hilbert states valid.

This repository intentionally contains only the core codecs, their correctness tests, and formal proofs. Range-query benchmarks and application-specific pipelines are outside its scope.

## Requirements

- C++11 or newer
- GCC, Clang, or MSVC
- Grid levels `k` in `[1, 32]`
- 32-bit coordinates and 64-bit Hilbert codes

Each encoder or decoder owns a `(k+1)`-byte state array (`33` bytes when `k=32`). Use a separate object for every independent sequence; no state is shared globally.

## Files

- `src/Neighbor-aware.h`: stream-local encoder/decoder interfaces
- `src/Neighbor-aware.cpp`: NA-HE/NA-HD implementation
- `src/test.cpp`: reference-based correctness and boundary tests
- `Proofs.tex`: source of the formal proofs
- `Proofs.pdf`: compiled formal proofs

## Build and test

```sh
mkdir -p build
c++ -std=c++11 -O3 -DNDEBUG -Wall -Wextra -Wpedantic \
  src/Neighbor-aware.cpp src/test.cpp -o build/test
./build/test
```

The tests cover exhaustive small grids, S-order windows, sorted-code decoding, duplicate and single-point inputs, random sequences, `k=32` boundaries, interleaved independent streams, reset/replay, and the exact AEL/ADL formulas.

## Usage

```cpp
#include "src/Neighbor-aware.h"

nahe::Encoder encoder(32);
nahe::Code code = encoder.encode(x, y, next_x, next_y);

nahe::Decoder decoder(32);
nahe::GridPoint point = decoder.decode(code, next_code);
```

Pass the current point/code again as the lookahead input for the final item in a sequence. NA-HD expects codes in sorted order. Call `reset()` before reusing an object for a new sequence.

The `coded_levels()`, `decoded_levels()`, and `updated_levels()` accessors expose operation counts for evaluating AEL, ADL, and DUM updates. `state_bytes()` reports the `(k+1)`-byte state-array size; it does not include bookkeeping fields in the C++ object.
