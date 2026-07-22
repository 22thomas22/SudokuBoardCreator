#include <iostream>
#include <random> // random_device for seeding
#include <chrono> // timekeeping
#include <fstream> // for long debug logs
#include <vector> // to hold the stats
#include <bitset> // for debugging bits
#include <immintrin.h> // Required for BMI2 intrinsics
#include <array>
#include <cfloat>
#include <climits> // INT_MAX, UINT64_MAX
#include <cstring> // memcpy
#include <filesystem>

struct fastRand {
    uint64_t state;
    fastRand() {
        std::random_device rd;
        state = (uint64_t{rd()} << 32) | uint64_t{rd()};
    }
    using result_type = uint64_t;
    // static constexpr result_type min() { return 0; }
    // static constexpr result_type max() { return UINT64_MAX; }
    inline result_type operator()() {
        state += 0x9E3779B97F4A7C15ULL;
        uint64_t z = state;
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
};

class Sudoku {
public:
    Sudoku() = default;
    void resetMasks() {
        memset(data, 0, sizeof(data));
        memset(cellMasks, 0, sizeof(cellMasks));
    }
    void generate2() {
        // restart:
        resetMasks();
        // generate the first row:
        for (uint8_t i = 8; i > 0; --i) { // fisher-yates
            const uint8_t j = fastClamp(rng(), (i + 1));
            std::swap(shuffleOptions[i], shuffleOptions[j]);
        }
        memcpy(data, shuffleOptions, 9);

        // generate the side row:
        for (bool valid = false; !valid;) { // repeat until valid
            for (int8_t i = 8; i > 1; --i) { // fisher-yates, don't include the first element
                const uint8_t j = 1 + fastClamp(rng(), i);
                std::swap(shuffleOptions[i], shuffleOptions[j]);
            }
            valid = (
                shuffleOptions[1] != data[1]) && (shuffleOptions[2] != data[2])
                && (shuffleOptions[1] != data[2]) && (shuffleOptions[2] != data[1] // make sure the new column doesn't break rules
            );
        }
        for (uint8_t i = 9, j = 0; i < 81; i += 9) {
            data[i] = shuffleOptions[++j];

            rowMasks[j-1] = 1 << data[i];
            columnMasks[j-1] = 1 << data[j];
        }

        boxMasks[0] = 1 << data[0] | columnMasks[0] | columnMasks[1] | rowMasks[0] | rowMasks[1];
        boxMasks[1] = columnMasks[2] | columnMasks[3] | columnMasks[4];
        boxMasks[2] = columnMasks[5] | columnMasks[6] | columnMasks[7];
        boxMasks[3] = rowMasks[2] | rowMasks[3] | rowMasks[4];
        boxMasks[4] = 0, boxMasks[5] = 0;
        boxMasks[6] = rowMasks[5] | rowMasks[6] | rowMasks[7];
        boxMasks[7] = 0, boxMasks[8] = 0;

        cell = 10;
        uint8_t pos = 0;
        /**
         *  Algorithm Pseudocode
         *  start loop
         *    if last cell:
         *      check mask. If options avaliable, pick one and exit.
         *      otherwise backtrack and continue
         *    Cache the values of modulo and divide 9
         *    if cell has data after it (next cell has been explored):
         *      reset the next cell (that we just came from) and erase the current value from the current cell
         *    generate a mask of avaliable spots and remove any spots we have already tried
         *    if the mask is not blank (if there are still spots we can visit):
         *      count how many options, generate a random number 0-options
         *      find the offset of that option, this number is our new value
         *      store this value in our 3 bitmasks and maks sure we record this value in our cell's bitmask (to not try it again)
         *      jump to the next cell in line
         *      update the cell
         *    otherwise if the mask is indeed blank:
         *      go back to the previous cell, cleanup will be handled on the next iteration by that cell.
         *  **/
        while (true) {
            if (cell == 80)/*[[unlikely]]*/ {
                data[80] = ~(rowMasks[7] | columnMasks[7] | boxMasks[8]) & 0x3FE;
                if (data[80] != 0) {
                    data[80] = std::countr_zero(_pdep_u64(1u << fastClamp(rng(), std::popcount(data[80])), data[80]));
                    break;
                } else {
                    cell = 79, pos = 62;
                }
            }

            const uint8_t cm9 = cell_mod9[cell] - 1;
            const uint8_t cd9 = cell_div9[cell] - 1;

            if (data[cell] != 0) {
                cellMasks[mappedPath[pos + 1]] = 0;
                rowMasks[cd9] &= ~(1 << data[cell]);
                columnMasks[cm9] &= ~(1 << data[cell]);
                boxMasks[box[cell]] &= ~(1 << data[cell]);
            }
            uint16_t mask = ~(rowMasks[cd9] | columnMasks[cm9] | boxMasks[box[cell]]) & 0x3FE;
            mask &= ~cellMasks[mapping[cell]];
            if (mask != 0) {
                const uint8_t options = std::popcount(mask);
                const uint8_t index = fastClamp(rng(), options);

                const uint8_t number = std::countr_zero(_pdep_u64(1u << index, mask)); // grab the Nth 1 and read the offset
                cellMasks[mapping[cell]] |= (1 << number); // add to our tried mask so we don't pick it again
                data[cell] = number;

                rowMasks[cd9] |= (1 << number);
                columnMasks[cm9] |= (1 << number);
                boxMasks[box[cell]] |= (1 << number);

                data[path[pos + 1]] = 0;
                cell = path[++pos];
            } else {
                cell = path[--pos];
            }
        }
    }
    void show() const {
        for (int i = 0; i < 81; i++) {
            if (i == cell) {
                os << '[' << int{data[i]} << ']';
            } else {
                os << ' ' << int{data[i]} << ' ';
            }
            if (i % 9 == 8) os << '\n';
        }
        os << std::endl << std::endl;
    }
private:
    // helper functions
    static inline uint32_t fastClamp(const uint32_t x, const uint32_t N) { // Daniel Lemire's fast range reduction
        return ((uint64_t) x * (uint64_t) N) >> 32;
    }
    // variables
    static constexpr uint8_t path[64] = {
        10, 11, 19, 20, // finish the first box
        12, 13, 14, 15, 16, 17, // first row
        21, 22, 23, 24, 25, 26, // second row
        28, 37, 46, 55, 64, 73,
        29, 38, 47, 56, 65, 74,
        30, 31, 32, 33, 34, 35,
        39, 48, 57, 66, 75,
        40, 41, 49, 50,
        42, 43, 44,
        51, 52, 53,
        58, 67, 76,
        59, 68, 77,
        60, 61, 62,
        69, 78,
        70, 71,
        79, 80
    };
    inline static constexpr auto box = [] {
        std::array<uint8_t, 81> b{};
        for (int i = 0; i < 81; ++i)
            b[i] = (i / 9 / 3) * 3 + (i % 9 / 3);
        return b;
    }();

    inline static constexpr auto mapping = [] {
        std::array<uint8_t, 81> m{};
        for (int i = 10, j = 0; i < 81; i++) {
            m[i] = j;
            if (i % 9 != 0) ++j;
        }
        return m;
    }();

    inline static constexpr auto cell_mod9 = [] {
        std::array<uint8_t, 81> mod{};
        for (int i = 0; i < 81; i++) {
            mod[i] = i % 9;
        }
        return mod;
    }();
    inline static constexpr auto cell_div9 = [] {
        std::array<uint8_t, 81> div{};
        for (int i = 0; i < 81; i++) {
            div[i] = i / 9;
        }
        return div;
    }();
    inline static constexpr auto mappedPath = [] {
        std::array<uint8_t, 64> a{};
        for (int i = 0; i < 64; i++)
            a[i] = mapping[path[i]];
        return a;
    }();

    fastRand rng; // a class instance that is compatible with shuffle, dist, etc.

    uint16_t data[81] = {0}; // the visual data, the end product

    uint8_t cell; // the big loop



    uint8_t shuffleOptions[9] = {1,2,3,4,5,6,7,8,9};

    // masks
    uint16_t rowMasks[8];
    uint16_t columnMasks[8];
    uint16_t boxMasks[9];
    uint16_t cellMasks[64];

    // debugging tools
    [[maybe_unused]] void showBitmasks() const {
        int boxCounter = 0;
        for (int i = 0; i < 9; i++) {
            if (i == 0) {
                os << "           ";
                continue;
            }
            if (cell % 9 == i) {
                os << '[' << std::bitset<16>(columnMasks[i - 1]).to_string().erase(0, 7) << ']';
            } else {
                os << ' ' << std::bitset<16>(columnMasks[i - 1]).to_string().erase(0, 7) << ' ';
            }
        }
        os << '\n';
        for (int i = 1; i < 9; i++) {
            if (cell / 9 == i) {
                os << '[' << std::bitset<16>(rowMasks[i - 1]).to_string().erase(0, 7) << ']';
            } else {
                os << ' ' << std::bitset<16>(rowMasks[i - 1]).to_string().erase(0, 7) << ' ';
            }
            if ((i) % 3 == 1) {
                char lb1 = (boxCounter == box[cell]) ? '[' : ' ';
                char rb1 = (boxCounter == box[cell]) ? ']' : ' ';
                char lb2 = (boxCounter+1 == box[cell]) ? '[' : ' ';
                char rb2 = (boxCounter+1 == box[cell]) ? ']' : ' ';
                char lb3 = (boxCounter+2 == box[cell]) ? '[' : ' ';
                char rb3 = (boxCounter+2 == box[cell]) ? ']' : ' ';
                os << lb1 << std::bitset<16>(boxMasks[boxCounter++]).to_string().erase(0, 7) << rb1 << "                      "
                    << lb2 << std::bitset<16>(boxMasks[boxCounter++]).to_string().erase(0, 7) << rb2 << "                      "
                    << lb3 << std::bitset<16>(boxMasks[boxCounter++]).to_string().erase(0, 7) << rb3;
            }
            os << '\n';
        }
    }
    [[maybe_unused]] mutable std::ofstream os = std::ofstream("output.txt");
    [[maybe_unused]] void msg(const std::string &note) const {
        os << note << std::endl;
    }
    [[maybe_unused]] void showCellMasks() const {
        for (int i = 0; i < 64; i++) {
            if (const int b8tob9 = (i+10) + i/9; cell == b8tob9) {
                os << '[' << (std::bitset<16>(cellMasks[i]).to_string()).erase(0, 7) << ']';
            } else {
                os << ' ' << (std::bitset<16>(cellMasks[i]).to_string()).erase(0, 7) << ' ';
            }

            if (i % 8 == 7) os << '\n';
        }
    }
    [[maybe_unused]] void checkValid() const {
        int rows[9][9] = {0}, cols[9][9] = {0};
        for (int i = 0; i < cell; i++) {
            if (rows[i / 9][data[i]] != 0 || cols[i % 9][data[i]] != 0) {
                msg("error found");
                msg("cell " + std::to_string(i));
                msg("value " + std::to_string(data[i]));
                exit(0);
            }
            rows[i / 9][data[i]] = 1;
            cols[i % 9][data[i]] = 1;
        }
    }
    [[maybe_unused]] void showAllPossibleMasks() const {
        for (int i = 0; i < 81; i++) {
            uint16_t calculatedMask =  ~(rowMasks[i / 9] |          // couldn't use my builtin updateMask function,
                columnMasks[i % 9] | boxMasks[box[i]]) & 0x3FE;  // as it's tied to cell data and not a real function
            char lb = cell == i ? '[' : ' ';
            char rb = cell == i ? ']' : ' ';
            if (i % 9 == 0 || i < 9) {
                os << "(reserved)    ";
            } else {
                os << lb << std::bitset<16>(calculatedMask).to_string().erase(0, 7) << '(' << std::popcount(calculatedMask) << ')' << rb;
            }

            if (i % 9 == 8) {
                os << '\n';
            }
        }
        os << std::endl;
    }
};
std::ostream& operator<<(std::ostream& os, const Sudoku& S) {
    return os; // TODO
}

struct Timer {
    std::chrono::time_point<std::chrono::system_clock> _start;
    std::chrono::time_point<std::chrono::system_clock> _stop;
    void start() {
        _start = std::chrono::high_resolution_clock::now();
    }
    void stop() {
        _stop = std::chrono::high_resolution_clock::now();
    }
    [[nodiscard]] double getElapsed() const {
        return std::chrono::duration<double>(_stop - _start).count();
    }
};
std::ostream& operator<<(std::ostream& os, const Timer& t) {
    return os << t.getElapsed() << " seconds" << std::endl;
}

struct stats {
    stats(const int cuts) : cuts(cuts) {}
    std::vector<double> data;
    long long count = 0;
    int cuts;
    void addElement(const double time) {
        data.push_back(time);
    }
};
std::ostream& operator<<(std::ostream& os, const stats& S) {
    double min = DBL_MAX;
    //double max = -DBL_MAX;
    for (double i : S.data) {
        min = std::min(min, i);
        //max = std::max(max, i);
    }
    double max = 0.002;
    double delta = (max - min) / S.cuts;
    std::vector<long long int> bars(S.cuts, 0);
    for (const double v : S.data) {
        int idx = int((v - min) / delta);
        if (idx >= S.cuts) idx = S.cuts - 1;
        ++bars[idx];
    }
    for (int i = 0; i < S.cuts; i++) {
        const double binStart = min + i * delta;
        const double binEnd = min + (i+1) * delta;
        os << binStart << "," << binEnd << "," << bars[i] << "\n";
    }
    return os;
}

int main() {
    // about 6.8 microseconds per board
    Sudoku S;

    constexpr long int iterations = 100000; // to estimate the time, take iterations * 7e-6 seconds
    stats Z(10'000); // create a histogram with 10000 cuts in it.

    Timer T;
    T.start();
    for (long int i = 0; i < iterations; i++) {
        S.generate2();
    }
    T.stop();
    std::cout << T;
    return 0;
}