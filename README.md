# NA-HE & NA-HD CODE

### Efficient Constant-Time Complexity Hilbert Curve Encoding and Decoding Algorithms

We propose novel **Neighbor-aware Hilbert Encoding (NA-HE)** and **Neighbor-aware Hilbert Decoding (NA-HD)** algorithms to address the efficiency bottleneck of traditional Hilbert curve operations. Unlike existing methods that process points individually, our approach systematically exploits inter-point relationships (spatial locality) common in web-scale datasets.

By establishing **2-point and 3-point models**, utilizing a **state array**, and employing a **Dynamic Updating Mechanism (DUM)**, NA-HE and NA-HD minimize redundant iterations. Theoretical analysis and experiments show that our algorithms reduce the average per-point time complexity from **O(k)** to **O(1)** (where *k* is the Hilbert order). Experimental results demonstrate a speedup of up to **35.28x** over state-of-the-art algorithms on trajectory and window datasets.

### Environment

- **System:** Windows 11 / macOS (tested on macOS 15.2+)
- **Language:** C++ (Standard C++11 or higher)
- **Compiler:** GCC / Clang / MSVC

### Files Introduction

- **Neighbor-aware.h**: Header file declaring the NA-HE and NA-HD functions
- **Neighbor-aware.cpp**: Implementation of NA-HE and NA-HD algorithms with detailed comments
- **common.h**: Common type definitions (halfmask_t, bitmask_t)
- **test_imageContour**: Test program for image contour extraction and compression using NA-HD algorithm 
