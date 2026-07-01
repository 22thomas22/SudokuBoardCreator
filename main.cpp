    #include <iostream>
    #include <random> // random_device for seeding
    #include <chrono> // timekeeping
    #include <fstream>
    #include <array>
    #include <vector>
    #include <bitset> // for debugging bits

    #include <climits> // INT_MAX, UINT64_MAX
    #include <cstring> // memcpy
    class fastRand {
    public:
        fastRand() {
            std::random_device rd;
            randomSeeds = {
                0x180ec6d33cfd0abaULL ^ (uint64_t(rd()) << 32) | uint64_t(rd()),
                0xd5a61266f0c9392cULL ^ (uint64_t(rd()) << 32) | uint64_t(rd()),
                0xa9582618e03fc9aaULL ^ (uint64_t(rd()) << 32) | uint64_t(rd()),
                0x39abdc4529b1661cULL ^ (uint64_t(rd()) << 32) | uint64_t(rd()),
            };
        }
        using result_type = uint64_t;

        static constexpr result_type min() {return 0UL;}
        static constexpr result_type max() {return UINT64_MAX;}
        result_type operator()() {
            const result_type result = rotl(randomSeeds[0] + randomSeeds[3], 23) + randomSeeds[0];

            // Advance state: this is the actual "scramble" step
            const result_type t = randomSeeds[1] << 17;

            randomSeeds[2] ^= randomSeeds[0];            // mix s[0] into s[2]
            randomSeeds[3] ^= randomSeeds[1];            // mix s[1] into s[3]
            randomSeeds[1] ^= randomSeeds[2];            // mix s[2] back into s[1]
            randomSeeds[0] ^= randomSeeds[3];            // mix s[3] back into s[0]

            randomSeeds[2] ^= t;                         // apply the shift from s[1]
            randomSeeds[3] = rotl(randomSeeds[3], 45); // rotate s[3] to prevent patterns

            return result;
        }
    private:
        std::array<result_type, 4> randomSeeds;

        static result_type rotl(const uint64_t x, const int k) { // similar to bit shift but brings the value back to the other side
            return (x << k) | (x >> (64 - k));
        }
    };

    class Sudoku {
    public:
        Sudoku() {
            resetMasks();

            for (int i = 0; i < 81; i++) {
                box[i] = (i/9 / 3) * 3 + (i%9 / 3);
                // 0 1 2
                // 3 4 5
                // 6 7 8
            }
            for (int i = 10, j = 0; i < 81; i++) {
                mapping[i] = j;
                ++j;if (i % 9 == 0) --j;
            }
        }
        void resetMasks() {
            memset(rowMasks, 0, sizeof(rowMasks));
            memset(columnMasks, 0, sizeof(columnMasks));
            memset(boxMasks, 0, sizeof(boxMasks));

            memset(data, 0, sizeof(data));
            memset(cellMasks, 0, sizeof(cellMasks));
        }
        int generate2() {
            int restartCounter = 0;

            restart:
            restartCounter++;
            resetMasks();
            // generate the first row:
            std::shuffle(std::begin(shuffleOptions), std::end(shuffleOptions), rng);
            memcpy(data, shuffleOptions, 9);

            // generate the side row:
            //bool valid = false;
            for (bool valid = false; !valid;) { // repeat until valid
                std::shuffle(std::begin(shuffleOptions) + 1, std::end(shuffleOptions), rng);
                valid = (shuffleOptions[1] != data[1]) && (shuffleOptions[2] != data[2]);
            }
            for (uint8_t i = 9, j = 0; i < 81; i += 9) {
                data[i] = shuffleOptions[++j];

                rowMasks[j-1] |= 1 << data[i];
                columnMasks[j-1] |= 1 << data[j];
            }

            boxMasks[0] = 1 << data[0] | columnMasks[0] | columnMasks[1] | rowMasks[0] | rowMasks[1];
            boxMasks[1] = columnMasks[2] | columnMasks[3] | columnMasks[4];
            boxMasks[2] = columnMasks[5] | columnMasks[6] | columnMasks[7];
            boxMasks[3] = rowMasks[2] | rowMasks[3] | rowMasks[4];
            boxMasks[6] = rowMasks[5] | rowMasks[6] | rowMasks[7];

            cell = 10;
            int counter = 0;
            while (cell < 81) {
                counter++;
                if (counter % 50'000 == 0) {
                    msg("start message output for loop" + std::to_string(counter));
                    msg("cell masks");
                    showCellMasks();
                    msg("row/col/block masks");
                    showBitmasks();
                    msg("data (iteration " + std::to_string(counter) + ")");
                    show();
                    msg("stop message output for loop " + std::to_string(counter));
                }
                cell_div9 = cell / 9 - 1;
                cell_mod9 = cell % 9 - 1;
                uint16_t cellMapping = cell - 10 - (cell-10)/9;
                uint16_t _cp1 = cell + 1; if (_cp1 % 9 == 0) ++_cp1;
                uint16_t cellMappingPlus1 = _cp1 - 10 - (_cp1-10)/9;

                //msg("top of loop " + std::to_string(cell));
                //msg("masks");
                //showCellMasks();
                //msg("data array");
                //show();
                //msg("bitmask:");
                //showBitmasks();

                if (data[cell] != 0) { // visited before
                    cellMasks[cellMappingPlus1] = 0; // next cell data is now useless, we are about to change something upstream
                    rowMasks[cell_div9] &= ~(1 << data[cell]); // take out the mask data
                    columnMasks[cell_mod9] &= ~(1 << data[cell]);
                    boxMasks[box[cell]] &= ~(1 << data[cell]);
                }
                uint16_t mask = updateMask();
                if (data[cell] != 0) { // not our first time on this cell
                    mask &= ~cellMasks[cellMapping]; // make sure we aren't picking a number we tried another time
                }
                if (mask != 0) { // nwe have options avaliable to us
                    uint8_t options = std::popcount(mask);
                    uint8_t index = rng() % options;
                    uint16_t maskCopy = mask;
                    for (int k = 0; k < index; k++) {
                        maskCopy &= maskCopy - 1;
                    }
                    uint8_t number = std::countr_zero(maskCopy);
                    cellMasks[cellMapping] |= (1 << number); // add to our tried mask
                    data[cell] = number;

                    rowMasks[cell_div9] |= (1 << number);
                    columnMasks[cell_mod9] |= (1 << number);
                    boxMasks[box[cell]] |= (1 << number);

                    if (_cp1 < 81) data[_cp1] = 0; // IMPORTANT:

                    cell = _cp1; //++cell; if (cell % 9 == 0) ++cell;
                } else {
                    --cell;
                    if (cell % 9 == 0) --cell;
                    if (cell < 10) goto restart; // hacky temp fix for avoiding undefined behavior
                }
            }
            return restartCounter;
        }
        void show() const {
            for (int i = 0; i < 81; i++) {
                if (i == cell) {
                    os << '[' << (int)data[i] << ']';
                } else {
                    os << ' ' << (int)data[i] << ' ';
                }
                if (i % 9 == 8) os << '\n';
            }
            os << std::endl << std::endl;
        }
    private:
        // secondary helper functions

        // helper functions
        static uint8_t getDigit(uint16_t temp, const uint8_t& target) {
            for (int k = 0; k < target; k++) { // strip away the 1s so we can count how many leading zeros
                temp &= temp - 1; // turns the next exposed 1 into a 0
            }
            return std::countr_zero(temp); // count the number of leading zeros
        }
        uint16_t updateMask() const {
            return ~(                           // not: return the bits that are not being used
                rowMasks[cell_div9]         // overlay the current row
                | columnMasks[cell_mod9]    // overlay the current column
                | boxMasks[box[cell]]           // overlay the box where our box table tells us to go
                ) & 0x3FE;                      // hide the extra bits on the end, since we are using 9/16 bits
        }
        // variables
        uint8_t box[81]; // maps each square to a 3x3 box for speed. Look-up table is set in constuctor
        uint8_t mapping[81]; // maps each 9x9 location to an 8x8 location. Fails on the last digit.

        fastRand rng; // a class instance that is compatible with shuffle, dist, etc.

        uint8_t data[81] = {0}; // the visual data, the end product

        uint8_t cell; // the big loop

        uint8_t cell_mod9;
        uint8_t cell_div9;


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
                int b8tob9 = (i+10) + i/9;
                if (cell == b8tob9) {
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
    };
std::ostream& operator<<(std::ostream& os, const Sudoku& S) {
    return os;
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

    class stats {
    public:
        int min = INT_MAX;
        int max = 0;
        double average() const {
            double avg = 0;
            for (const int &val : data) {
                avg += (double)val;
            }
            avg /= (double)data.size();
            return avg;
        }
        double standardDeviation() const {
            const double avg = average();
            double stddev = 0;
            for (const int &val : data) {
                const double tmp = avg - (double)val;
                stddev += tmp * tmp;
            }
            stddev /= (double)(data.size() - 1);
            return stddev;
        }
        void capture(const int value) {
            data.push_back(value);
            min = std::min(min, value);
            max = std::max(max, value);
        }
    private:
        std::vector<int> data;
    };
    std::ostream& operator<<(std::ostream& os, const stats& S) {
        return os << std::endl << "Average: " << S.average() <<"\nMin: " << S.min
        << "\nMax: " << S.max << "\nStandard Deviation: " << S.standardDeviation();
    }

    int main() {
        Timer T;
        Sudoku S;

        constexpr int iterations = 100'000;
        stats Z;

        T.start();
        for (int i = 0; i < 100; i++) {
            Z.capture(S.generate2());
        }
        T.stop();
        S.show();
        std::cout << T;
        std::cout << Z;
        return 0;
    }