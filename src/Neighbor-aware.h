#ifndef NAHE_NEIGHBOR_AWARE_H
#define NAHE_NEIGHBOR_AWARE_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace nahe {

using Coordinate = std::uint32_t;
using Code = std::uint64_t;

struct GridPoint {
    Coordinate x;
    Coordinate y;
};

// One encoder object owns the state of one coordinate sequence.
class Encoder {
public:
    explicit Encoder(int levels);

    // Encode the current point with one-step lookahead. For the last point,
    // pass its coordinates again as next_x and next_y.
    Code encode(Coordinate x, Coordinate y, Coordinate next_x, Coordinate next_y);

    void reset();
    int levels() const { return levels_; }
    std::size_t state_bytes() const { return memory_.size(); }
    std::uint64_t coded_levels() const { return coded_levels_; }
    std::uint64_t updated_levels() const { return updated_levels_; }

private:
    int levels_;
    int iter_start_pos_;
    Code previous_code_;
    std::vector<std::uint8_t> memory_;
    std::uint64_t coded_levels_;
    std::uint64_t updated_levels_;
};

// One decoder object owns the state of one sorted Hilbert-code sequence.
class Decoder {
public:
    explicit Decoder(int levels);

    // Decode the current code with one-step lookahead. For the last code,
    // pass current_code again as next_code.
    GridPoint decode(Code current_code, Code next_code);

    void reset();
    int levels() const { return levels_; }
    std::size_t state_bytes() const { return memory_.size(); }
    std::uint64_t decoded_levels() const { return decoded_levels_; }
    std::uint64_t updated_levels() const { return updated_levels_; }

private:
    int levels_;
    int iter_start_pos_;
    Coordinate previous_x_;
    Coordinate previous_y_;
    std::vector<std::uint8_t> memory_;
    std::uint64_t decoded_levels_;
    std::uint64_t updated_levels_;
};

}  // namespace nahe

#endif  // NAHE_NEIGHBOR_AWARE_H
