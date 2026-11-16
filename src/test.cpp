#include "Neighbor-aware.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using nahe::Code;
using nahe::Coordinate;
using nahe::GridPoint;

struct TestPoint {
    Coordinate x;
    Coordinate y;
    Code code;
};

const unsigned CHM_REF[4][2][2] = {
    {{0, 1}, {3, 2}},
    {{0, 3}, {1, 2}},
    {{2, 3}, {1, 0}},
    {{2, 1}, {3, 0}},
};
const unsigned CSM_REF[4][2][2] = {
    {{1, 0}, {3, 0}},
    {{0, 2}, {1, 1}},
    {{2, 1}, {2, 3}},
    {{3, 3}, {0, 2}},
};
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

Code referenceEncode(Coordinate x, Coordinate y, int levels) {
    unsigned state = 0;
    Code code = 0;
    for (int i = levels - 1; i >= 0; --i) {
        const unsigned bit_x = static_cast<unsigned>((x >> i) & 1U);
        const unsigned bit_y = static_cast<unsigned>((y >> i) & 1U);
        code = (code << 2) | CHM_REF[state][bit_x][bit_y];
        state = CSM_REF[state][bit_x][bit_y];
    }
    return code;
}

GridPoint referenceDecode(Code code, int levels) {
    unsigned inverse_key[4][4] = {};
    unsigned inverse_state[4][4] = {};
    for (unsigned state = 0; state < 4; ++state) {
        for (unsigned x_bit = 0; x_bit < 2; ++x_bit) {
            for (unsigned y_bit = 0; y_bit < 2; ++y_bit) {
                const unsigned pair = CHM_REF[state][x_bit][y_bit];
                inverse_key[state][pair] = (x_bit << 1) | y_bit;
                inverse_state[state][pair] = CSM_REF[state][x_bit][y_bit];
            }
        }
    }

    unsigned state = 0;
    Coordinate x = 0;
    Coordinate y = 0;
    for (int i = levels - 1; i >= 0; --i) {
        const unsigned pair = static_cast<unsigned>((code >> (2 * i)) & 3ULL);
        const unsigned position = inverse_key[state][pair];
        x = (x << 1) | ((position >> 1) & 1U);
        y = (y << 1) | (position & 1U);
        state = inverse_state[state][pair];
    }
    return {x, y};
}

void assignReferenceCodes(std::vector<TestPoint>& points, int levels) {
    for (auto& point : points) point.code = referenceEncode(point.x, point.y, levels);
}

void sortByCode(std::vector<TestPoint>& points) {
    std::sort(points.begin(), points.end(), [](const TestPoint& a, const TestPoint& b) {
        return a.code < b.code;
    });
}

void runCase(const std::string& name, int levels, std::vector<TestPoint> points) {
    require(!points.empty(), name + ": empty input");
    assignReferenceCodes(points, levels);

    nahe::Encoder encoder(levels);
    require(encoder.state_bytes() == static_cast<std::size_t>(levels + 1),
            name + ": incorrect encoder state size");
    for (std::size_t i = 0; i < points.size(); ++i) {
        const auto& current = points[i];
        const auto& next = points[std::min(i + 1, points.size() - 1)];
        const Code actual = encoder.encode(current.x, current.y, next.x, next.y);
        require(actual == current.code, name + ": encoding mismatch at " + std::to_string(i));
    }

    sortByCode(points);
    nahe::Decoder decoder(levels);
    require(decoder.state_bytes() == static_cast<std::size_t>(levels + 1),
            name + ": incorrect decoder state size");
    for (std::size_t i = 0; i < points.size(); ++i) {
        const auto& current = points[i];
        const Code next_code = points[std::min(i + 1, points.size() - 1)].code;
        const GridPoint actual = decoder.decode(current.code, next_code);
        require(actual.x == current.x && actual.y == current.y,
                name + ": decoding mismatch at " + std::to_string(i) +
                ", expected (" + std::to_string(current.x) + "," + std::to_string(current.y) +
                "), got (" + std::to_string(actual.x) + "," + std::to_string(actual.y) + ")");
        const GridPoint expected = referenceDecode(current.code, levels);
        require(actual.x == expected.x && actual.y == expected.y,
                name + ": reference decoding mismatch at " + std::to_string(i));
    }

    std::cout << name << ": PASS (" << points.size() << " points, k=" << levels
              << ", AEL=" << static_cast<double>(encoder.coded_levels()) / points.size()
              << ", ADL=" << static_cast<double>(decoder.decoded_levels()) / points.size()
              << ")\n";
}

std::vector<TestPoint> fullGrid(int levels, bool s_order) {
    const Coordinate side = Coordinate{1} << levels;
    std::vector<TestPoint> points;
    points.reserve(static_cast<std::size_t>(side) * side);
    for (Coordinate y = 0; y < side; ++y) {
        for (Coordinate offset = 0; offset < side; ++offset) {
            const Coordinate x = s_order && (y & 1U) ? side - 1 - offset : offset;
            points.push_back({x, y, 0});
        }
    }
    return points;
}

std::vector<TestPoint> randomPoints(int levels, std::size_t count, std::uint64_t seed) {
    std::mt19937_64 generator(seed);
    const Coordinate maximum = levels == 32 ? std::numeric_limits<Coordinate>::max()
                                            : (Coordinate{1} << levels) - 1U;
    std::uniform_int_distribution<Coordinate> distribution(0, maximum);
    std::vector<TestPoint> points;
    points.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        points.push_back({distribution(generator), distribution(generator), 0});
    }
    return points;
}

void checkAverageLevels(int levels) {
    auto points = fullGrid(levels, true);
    assignReferenceCodes(points, levels);
    const double count = static_cast<double>(points.size());

    nahe::Encoder encoder(levels);
    for (std::size_t i = 0; i < points.size(); ++i) {
        const auto& current = points[i];
        const auto& next = points[std::min(i + 1, points.size() - 1)];
        require(encoder.encode(current.x, current.y, next.x, next.y) == current.code,
                "AEL check: encoding mismatch");
    }
    const double side = static_cast<double>(Coordinate{1} << levels);
    const double expected_ael = 2.0 - levels / side - 2.0 / (count);
    const double actual_ael = encoder.coded_levels() / count;
    require(std::abs(actual_ael - expected_ael) < 1e-12, "AEL formula mismatch");
    require(actual_ael < 2.0, "AEL bound violated");

    sortByCode(points);
    nahe::Decoder decoder(levels);
    for (std::size_t i = 0; i < points.size(); ++i) {
        const auto& current = points[i];
        const Code next_code = points[std::min(i + 1, points.size() - 1)].code;
        const GridPoint actual = decoder.decode(current.code, next_code);
        require(actual.x == current.x && actual.y == current.y,
                "ADL check: decoding mismatch");
    }
    const double expected_adl = (4.0 / 3.0) * (1.0 - 1.0 / count);
    const double actual_adl = decoder.decoded_levels() / count;
    require(std::abs(actual_adl - expected_adl) < 1e-12, "ADL formula mismatch");
    require(actual_adl < 4.0 / 3.0, "ADL bound violated");

    std::cout << "level-" << levels << " full window: PASS (AEL=" << actual_ael
              << ", ADL=" << actual_adl << ")\n";
}

void checkInterleavedStreams() {
    constexpr int levels = 8;
    auto first = randomPoints(levels, 200, 17);
    auto second = randomPoints(levels, 200, 29);
    assignReferenceCodes(first, levels);
    assignReferenceCodes(second, levels);

    nahe::Encoder encoder_first(levels);
    nahe::Encoder encoder_second(levels);
    for (std::size_t i = 0; i < first.size(); ++i) {
        const std::size_t next = std::min(i + 1, first.size() - 1);
        require(encoder_first.encode(first[i].x, first[i].y, first[next].x, first[next].y)
                    == first[i].code, "interleaved first encoder mismatch");
        require(encoder_second.encode(second[i].x, second[i].y, second[next].x, second[next].y)
                    == second[i].code, "interleaved second encoder mismatch");
    }

    sortByCode(first);
    sortByCode(second);
    nahe::Decoder decoder_first(levels);
    nahe::Decoder decoder_second(levels);
    for (std::size_t i = 0; i < first.size(); ++i) {
        const std::size_t next = std::min(i + 1, first.size() - 1);
        const GridPoint a = decoder_first.decode(first[i].code, first[next].code);
        const GridPoint b = decoder_second.decode(second[i].code, second[next].code);
        require(a.x == first[i].x && a.y == first[i].y, "interleaved first decoder mismatch");
        require(b.x == second[i].x && b.y == second[i].y, "interleaved second decoder mismatch");
    }
    std::cout << "interleaved independent streams: PASS\n";
}

void checkReset() {
    constexpr int levels = 12;
    auto points = randomPoints(levels, 100, 43);
    assignReferenceCodes(points, levels);
    std::vector<Code> first_pass;
    nahe::Encoder encoder(levels);
    for (std::size_t i = 0; i < points.size(); ++i) {
        const std::size_t next = std::min(i + 1, points.size() - 1);
        first_pass.push_back(encoder.encode(points[i].x, points[i].y,
                                            points[next].x, points[next].y));
    }
    encoder.reset();
    require(encoder.coded_levels() == 0 && encoder.updated_levels() == 0,
            "encoder reset did not clear counters");
    for (std::size_t i = 0; i < points.size(); ++i) {
        const std::size_t next = std::min(i + 1, points.size() - 1);
        require(encoder.encode(points[i].x, points[i].y, points[next].x, points[next].y)
                    == first_pass[i], "encoder reset produced a different stream");
    }

    sortByCode(points);
    nahe::Decoder decoder(levels);
    std::vector<GridPoint> decoded_first;
    for (std::size_t i = 0; i < points.size(); ++i) {
        const std::size_t next = std::min(i + 1, points.size() - 1);
        decoded_first.push_back(decoder.decode(points[i].code, points[next].code));
    }
    decoder.reset();
    require(decoder.decoded_levels() == 0 && decoder.updated_levels() == 0,
            "decoder reset did not clear counters");
    for (std::size_t i = 0; i < points.size(); ++i) {
        const std::size_t next = std::min(i + 1, points.size() - 1);
        const GridPoint actual = decoder.decode(points[i].code, points[next].code);
        require(actual.x == decoded_first[i].x && actual.y == decoded_first[i].y,
                "decoder reset produced a different stream");
    }
    std::cout << "reset and replay: PASS\n";
}

void checkApiValidation() {
    bool threw = false;
    try { nahe::Encoder invalid(0); } catch (const std::invalid_argument&) { threw = true; }
    require(threw, "encoder accepted k=0");
    threw = false;
    try { nahe::Decoder invalid(33); } catch (const std::invalid_argument&) { threw = true; }
    require(threw, "decoder accepted k=33");
    std::cout << "level validation: PASS\n";
}

}  // namespace

int main() {
    try {
        runCase("level-1 exhaustive", 1, fullGrid(1, false));
        runCase("level-4 exhaustive", 4, fullGrid(4, false));
        runCase("duplicate neighbors", 4,
                {{0, 0, 0}, {0, 0, 0}, {0, 1, 0}, {0, 1, 0},
                 {1, 1, 0}, {1, 1, 0}, {1, 0, 0}, {1, 0, 0}});
        runCase("level-8 deterministic random", 8, randomPoints(8, 1000, 3723));

        auto level32 = std::vector<TestPoint>{
            {0, 0, 0},
            {0, 0, 0},
            {std::numeric_limits<Coordinate>::max(), 0, 0},
            {0, std::numeric_limits<Coordinate>::max(), 0},
            {std::numeric_limits<Coordinate>::max(), std::numeric_limits<Coordinate>::max(), 0},
        };
        auto random32 = randomPoints(32, 1000, 97);
        level32.insert(level32.end(), random32.begin(), random32.end());
        runCase("level-32 boundaries and random", 32, level32);
        runCase("level-32 single point", 32,
                {{std::numeric_limits<Coordinate>::max(),
                  std::numeric_limits<Coordinate>::max(), 0}});

        for (int levels = 1; levels <= 6; ++levels) checkAverageLevels(levels);
        checkInterleavedStreams();
        checkReset();
        checkApiValidation();
    } catch (const std::exception& error) {
        std::cerr << "TEST FAILURE: " << error.what() << '\n';
        return 1;
    }

    std::cout << "All NA-HE/NA-HD tests passed.\n";
    return 0;
}
