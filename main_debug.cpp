#include <iostream>
#include <random>
#include <chrono>
#include <fstream>

const bool debug = false;

std::ofstream logfile("output.txt");

inline void msg(const std::string &msg) {
    if (debug) logfile << msg << std::endl;
}

class Sudoku {
public:
    Sudoku(const std::mt19937& rng) : rng(rng), dist(0,8) {/*constructor*/}

    void generate() {
        int* linear = &data[0][0];
        int originalValues[81] = {0};
        std::shuffle(std::begin(options), std::end(options), rng);
        for (int i = 0; i < 9; i++) {
            linear[i] = options[i];
            originalValues[i] = options[i];
        }
        bool isBacktracking = false;
        int i = 9;
        while (i < 81) {
            msg("new loop: " + std::to_string(i));
            if (isBacktracking) {
                msg(std::format("Backtracking loop entered"));
                linear[i] = (linear[i] + 1) % 9;
                msg(std::format("originalValue: {}, linear[i={}]: {}.", originalValues[i], i, linear[i]));
                if (linear[i] == originalValues[i]) {
                    msg("no valid solutions, backtrack one more.");
                    i--;
                    continue;
                }
                if (check.checkAll(i / 9, i % 9)) {
                    i++;
                    msg("backtracking scope resolved, moving on to next value.");
                    isBacktracking = false;
                } else {
                    msg("Number didn't work, trying next number.");
                }
            } else {
                linear[i] = dist(rng);
                originalValues[i] = linear[i];
                msg(std::format("Created new square with number {}, displaying new grid:", std::to_string(linear[i])));
                if (debug) show(logfile);
                msg("running tests...");
                if (!check.checkAll(i / 9, i % 9)) {
                    msg("number rejected. Incrementing number...");
                    isBacktracking = true;
                } else {
                    msg("number accepted!");
                    i++;
                }
            }
        }
    }
    void show(std::ostream& os = std::cout) const {
        for (int i = 0; i < 9; i++) {
            for (int j = 0; j < 9; j++) {
                os << data[i][j] + 1 << ' '; // 0-8 -> 1-9
            }
        os << '\n';
        }
        os << std::endl;
        os << std::endl;
    }
private:
    inline uint64_t rotl(uint64_t x, int k) { // similar to bit shift but brings the value back to the other side
        return (x << k) | (x >> (64 - k));
    }
    inline uint64_t xoshiro256pp(uint64_t (&s)[4]) {
        const uint64_t result = rotl(s[0] + s[3], 23) + s[0];

        // Advance state: this is the actual "scramble" step
        const uint64_t t = s[1] << 17;

        s[2] ^= s[0];   // mix s[0] into s[2]
        s[3] ^= s[1];   // mix s[1] into s[3]
        s[1] ^= s[2];   // mix s[2] back into s[1]
        s[0] ^= s[3];   // mix s[3] back into s[0]

        s[2] ^= t;           // apply the shift from s[1]
        s[3] = rotl(s[3], 45);  // rotate s[3] to prevent patterns

        return result;
    }
    struct Check {
        int (*data)[9]; // pointer to the data
        bool checkAll(int row, int column) const {
            return columnSafe(column, row) && rowSafe(row, column) && block(row, column);
        }
        bool columnSafe(const int wholeColumn, const int currentRow) const {
            for (int row = 0; row < currentRow; row++) {
                if (data[row][wholeColumn] == data[currentRow][wholeColumn]) {
                    msg(std::format("bad column check: data[row={}][wholeColumn={}]==data[currentRow={}][wholeColumn]", row, wholeColumn, currentRow));
                    msg(std::format("(LHS={}, RHS={})", data[row][wholeColumn], data[currentRow][wholeColumn]));
                    return false;
                }
            }
            msg("good column check");
            return true;
        }
        bool rowSafe(int wholeRow, int currentColumn) const {
            for (int column = 0; column < currentColumn; column++) {
                if (data[wholeRow][column] == data[wholeRow][currentColumn]) {
                    msg(std::format("bad row check: data[wholeRow={}][column={}] == data[wholeRow][currentColumn={}]", wholeRow, column, currentColumn));
                    return false;
                }
            }
            msg("good row check");
            return true;
        }
        bool block(const int row, const int column) const {
            int rowBlock = (row / 3) * 3;
            int columnBlock = (column / 3) * 3;
            for (int i = rowBlock; i < row; i++) {
                for (int j = columnBlock; j < columnBlock + 3; j++) {
                    if (data[i][j] == data[row][column]) {
                        msg(std::format("bad block: data[i={}][j={}]==data[row={}][column={}]", i, j, row, column));
                        return false;
                    }
                }
            } // must be run with a column check to get the squares on the same row
            msg("good block");
            return true;
        }
    };

    int data[9][9] = {0}; // [row][column]
    std::mt19937 rng;
    std::uniform_int_distribution<int> dist;
    Check check{data};
    int options[9] = {0, 1,2,3,4,5,6,7,8};
};

struct Timer {
    std::chrono::time_point<std::chrono::system_clock> _start;
    std::chrono::time_point<std::chrono::system_clock> _end;
    void start() {
        _start = std::chrono::high_resolution_clock::now();
    }
    void end() {
        _end = std::chrono::high_resolution_clock::now();
    }
    double getElapsed() const {
        return std::chrono::duration<double>(_end - _start).count();
    }
};
std::ostream& operator<<(std::ostream& os, const Timer& t) {
    return os << t.getElapsed() << " seconds" << std::endl;
}

int main() {
    std::random_device rd;
    const std::mt19937 rng(rd());

    int totalBoards = 100;
    std::vector<Sudoku> boards;
    boards.reserve(totalBoards);
    for (int i = 0; i < totalBoards; i++) {
        boards.emplace_back(rng);
    }

    Timer T;
    T.start();
    for (Sudoku& board : boards) {
        board.generate();
        board.show();
    }
    T.end();
    std::cout << T;
    std::cout << "program finished!!!!";
    return 0;
}