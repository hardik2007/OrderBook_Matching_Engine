#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <iostream>
#include <chrono>
#include <string>
#include <iomanip>

using namespace std;

class Benchmark {
public:
    // Runs the supplied operation and reports elapsed time in milliseconds.
    template <typename Function>
    static double measureMilliseconds(
        const string& operationName,
        Function operation
    ) {
        auto start = chrono::steady_clock::now();

        operation();

        auto finish = chrono::steady_clock::now();

        double elapsed =
            chrono::duration<double, milli>(finish - start).count();

        cout << left << setw(32) << operationName
             << right << fixed << setprecision(3)
             << elapsed << " ms\n";

        return elapsed;
    }
};

#endif
