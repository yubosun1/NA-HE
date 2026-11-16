#include "Neighbor-aware.h"

#include <algorithm>
#include <stdexcept>

namespace nahe {
namespace {

constexpr int kMaxLevels = 32;

const std::uint8_t CHM[4][2][2] = {
    {{0, 1}, {3, 2}},
    {{0, 3}, {1, 2}},
    {{2, 3}, {1, 0}},
    {{2, 1}, {3, 0}},
};

const std::uint8_t CSM[4][2][2] = {
    {{1, 0}, {3, 0}},
    {{0, 2}, {1, 1}},
    {{2, 1}, {2, 3}},
    {{3, 3}, {0, 2}},
};

const std::uint8_t HCM[4][4] = {
    {0, 1, 3, 2},
    {0, 2, 3, 1},
    {3, 2, 0, 1},
    {3, 1, 0, 2},
};

const std::uint8_t HSM[4][4] = {
    {1, 0, 0, 3},
    {0, 1, 1, 2},
    {3, 2, 2, 1},
    {2, 3, 3, 0},
};

int validate_levels(int levels) {
    if (levels < 1 || levels > kMaxLevels) {
        throw std::invalid_argument("NA-HE/NA-HD supports levels in [1, 32]");
    }
    return levels;
}

int msb32(Coordinate value) {
    if (value == 0) return -1;
    int position = 0;
    if (value >= (Coordinate{1} << 16)) { position += 16; value >>= 16; }
    if (value >= (Coordinate{1} << 8)) { position += 8; value >>= 8; }
    if (value >= (Coordinate{1} << 4)) { position += 4; value >>= 4; }
    if (value >= (Coordinate{1} << 2)) { position += 2; value >>= 2; }
    if (value >= (Coordinate{1} << 1)) { position += 1; }
    return position;
}

int msb64(Code value) {
    if (value == 0) return -1;
    int position = 0;
    if (value >= (Code{1} << 32)) { position += 32; value >>= 32; }
    if (value >= (Code{1} << 16)) { position += 16; value >>= 16; }
    if (value >= (Code{1} << 8)) { position += 8; value >>= 8; }
    if (value >= (Code{1} << 4)) { position += 4; value >>= 4; }
    if (value >= (Code{1} << 2)) { position += 2; value >>= 2; }
    if (value >= (Code{1} << 1)) { position += 1; }
    return position;
}

}  // namespace

Encoder::Encoder(int levels)
    : levels_(validate_levels(levels)),
      iter_start_pos_(levels_ - 1),
      previous_code_(0),
      memory_(static_cast<std::size_t>(levels_ + 1), 0),
      coded_levels_(0),
      updated_levels_(0) {}

Code Encoder::encode(Coordinate x, Coordinate y, Coordinate next_x, Coordinate next_y) {
    const int new_iter_start_pos = msb32(std::max(next_x ^ x, next_y ^ y));
    unsigned state = memory_[static_cast<std::size_t>(levels_ - iter_start_pos_ - 1)];
    Code result = 0;

    for (int i = iter_start_pos_; i >= 0; --i) {
        ++coded_levels_;
        const unsigned bit_x = static_cast<unsigned>((x >> i) & 1U);
        const unsigned bit_y = static_cast<unsigned>((y >> i) & 1U);
        result = (result << 2) | CHM[state][bit_x][bit_y];
        state = CSM[state][bit_x][bit_y];

        // DUM: refresh exactly the states reused by the next point.
        if (i > new_iter_start_pos) {
            ++updated_levels_;
            memory_[static_cast<std::size_t>(levels_ - i)] =
                static_cast<std::uint8_t>(state);
        }
    }

    const int moved_bits = 2 * (iter_start_pos_ + 1);
    // At k=32 the first point recomputes all 64 bits; shifting by 64 is invalid.
    if (moved_bits < 64) {
        result |= (previous_code_ >> moved_bits) << moved_bits;
    }

    iter_start_pos_ = new_iter_start_pos;
    previous_code_ = result;
    return result;
}

void Encoder::reset() {
    iter_start_pos_ = levels_ - 1;
    previous_code_ = 0;
    std::fill(memory_.begin(), memory_.end(), 0);
    coded_levels_ = 0;
    updated_levels_ = 0;
}

Decoder::Decoder(int levels)
    : levels_(validate_levels(levels)),
      iter_start_pos_(levels_ - 1),
      previous_x_(0),
      previous_y_(0),
      memory_(static_cast<std::size_t>(levels_ + 1), 0),
      decoded_levels_(0),
      updated_levels_(0) {}

GridPoint Decoder::decode(Code current_code, Code next_code) {
    const int highest_difference = msb64(current_code ^ next_code);
    const int new_iter_start_pos = highest_difference < 0 ? -1 : highest_difference / 2;
    unsigned state = memory_[static_cast<std::size_t>(levels_ - iter_start_pos_ - 1)];
    Coordinate x = 0;
    Coordinate y = 0;

    for (int i = iter_start_pos_; i >= 0; --i) {
        ++decoded_levels_;
        const unsigned code_pair = static_cast<unsigned>((current_code >> (2 * i)) & 3ULL);
        const unsigned position = HCM[state][code_pair];
        x = (x << 1) | ((position >> 1) & 1U);
        y = (y << 1) | (position & 1U);
        state = HSM[state][code_pair];

        if (i > new_iter_start_pos) {
            ++updated_levels_;
            memory_[static_cast<std::size_t>(levels_ - i)] =
                static_cast<std::uint8_t>(state);
        }
    }

    const int moved_bits = iter_start_pos_ + 1;
    // At k=32 the first code reconstructs all 32 coordinate bits.
    if (moved_bits < 32) {
        const Coordinate prefix_mask = ~((Coordinate{1} << moved_bits) - 1U);
        x |= previous_x_ & prefix_mask;
        y |= previous_y_ & prefix_mask;
    }

    iter_start_pos_ = new_iter_start_pos;
    previous_x_ = x;
    previous_y_ = y;
    return {x, y};
}

void Decoder::reset() {
    iter_start_pos_ = levels_ - 1;
    previous_x_ = 0;
    previous_y_ = 0;
    std::fill(memory_.begin(), memory_.end(), 0);
    decoded_levels_ = 0;
    updated_levels_ = 0;
}

}  // namespace nahe
