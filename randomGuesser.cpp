#include <iostream>
#include <vector>
#include <chrono>
#include <cstdint>
#include <random>

#include <thread>
#include <atomic> // it's gettin' real now

//the threads need this information shared:
static std::atomic<bool> g_solved{false};
static std::atomic<long long> g_totalAttempts{0};

// randomize and check functions
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

void randomizeAll(uint8_t (&data)[81], uint64_t (&s)[4]) {
#pragma GCC unroll 12
    for (int i = 0; i < 81; i += 7) {
        uint64_t r = xoshiro256pp(s);
        int end = std::min(i + 7, 81);
        // Extract 7 x 9-bit chunks from the 64-bit output
        // 7 * 9 = 63 bits used, 1 bit wasted — fits cleanly
        for (int j = i; j < end; j++, r >>= 9)
            data[j] = (uint8_t)(((uint64_t)(uint32_t)r * 9) >> 32);
    }
}


bool isValid(const uint8_t (&data)[81]) {
    int rows[9] = {0}, cols[9] = {0}, boxes[9] = {0};

    for (int i = 0; i < 81; i++) {
        uint8_t val = data[i];

        int bit = 1 << val;
        int r = i / 9, c = i % 9, b = (r/3)*3 + c/3;

        if (rows[r] & bit) return false;
        if (cols[c] & bit) return false;
        if (boxes[b] & bit) return false;

        rows[r] |= bit;
        cols[c] |= bit;
        boxes[b] |= bit;
    }
    return true;
}

void showData(const uint8_t (&data)[81]) {
    for (int i = 0; i < 81; i++) {
        std::cout << (int)data[i];
        if (i % 9 == 8) {
            std::cout << '\n';
        }
    }
}

void workerThread(const int maxAttempts) {
    // to keep the state from seeding the same, copy it but offset.
    uint64_t localState[4] = {
        0x180ec6d33cfd0abaULL ^ (uint64_t)std::hash<std::thread::id>{}(std::this_thread::get_id()),
        0xd5a61266f0c9392cULL,
        0xa9582618e03fc9aaULL,
        0x39abdc4529b1661cULL
    };
    uint8_t data[81];
    int attempts = 0;
    while (!(g_solved || attempts > maxAttempts)) {
        randomizeAll(data, localState); // pass state as parameter
        if (isValid(data)) {
            g_solved = true;
            showData(data);
        }
        attempts++;
    }
    g_totalAttempts += attempts;
}

struct timer {
    std::chrono::time_point<std::chrono::system_clock> _start;
    std::chrono::time_point<std::chrono::system_clock> _end;
    double _duration;
    void start() {
        _start = std::chrono::high_resolution_clock::now();
    }
    void end() {
        _end = std::chrono::high_resolution_clock::now();
    }
    double getElapsed() {
        _duration = std::chrono::duration<double>(_end - _start).count();
        return _duration;
    }
};

int main() {
    const unsigned int numThreads = std::thread::hardware_concurrency(); // pull the right number of threads for your machine
    std::vector<std::thread> threads;
    std::cout << "Number of running threads: " << numThreads << std::endl;
    timer T;
    T.start();
    for (int i = 0; i < numThreads; i++)
        threads.emplace_back(workerThread, 10'000'000);
    for (auto& t : threads)
        t.join(); // clean up time
    T.end();
    double duration = T.getElapsed();

    const long long totalAttempts = g_totalAttempts.load();
    std::cout << totalAttempts << std::endl;
    std::cout << "time: " << duration << " seconds" << std::endl;
    const double loopTime = duration / (double)totalAttempts;
    std::cout << "time per iteration: " << loopTime << " seconds" << std::endl;
    std::cout << "average time to finish: " << (loopTime * 2.95e55)/*seconds*/ / 3600/*hours*/ / 24/*days*/ / 365/*years*/ << " years" << std::endl;
    std::cout << "cycles per second: " << 1.0 / loopTime << " cycles" << std::endl;
    return 0;
}