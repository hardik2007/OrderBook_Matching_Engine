#include <iostream>
#include <vector>
#include <string>

#include "../include/PriorityHeap.h"
#include "../include/AuditTrail.h"
#include "../include/Benchmark.h"

using namespace std;

void testPriorityHeaps() {
    cout << "\n===== PRIORITY HEAP TEST =====\n";

    MaxPriorityHeap maxHeap;
    maxHeap.push(PriorityItem(101, 30));
    maxHeap.push(PriorityItem(102, 90));
    maxHeap.push(PriorityItem(103, 50));
    maxHeap.push(PriorityItem(104, 70));

    cout << "Max Heap: highest priority order first\n";
    while (!maxHeap.empty()) {
        PriorityItem item = maxHeap.pop();
        cout << "Order ID: " << item.orderId
             << ", Priority: " << item.priority << '\n';
    }

    MinPriorityHeap minHeap;
    minHeap.push(PriorityItem(201, 30));
    minHeap.push(PriorityItem(202, 90));
    minHeap.push(PriorityItem(203, 50));

    cout << "\nMin Heap: lowest priority value first\n";
    while (!minHeap.empty()) {
        PriorityItem item = minHeap.pop();
        cout << "Order ID: " << item.orderId
             << ", Priority: " << item.priority << '\n';
    }

    vector<StockVolume> stocks;
    stocks.push_back(StockVolume("RELIANCE", 5000));
    stocks.push_back(StockVolume("TCS", 8000));
    stocks.push_back(StockVolume("INFY", 3000));
    stocks.push_back(StockVolume("HDFC", 9000));
    stocks.push_back(StockVolume("WIPRO", 2000));

    vector<StockVolume> topThree = getTopKVolumes(stocks, 3);

    cout << "\nTop 3 stocks by volume:\n";
    for (int i = 0; i < static_cast<int>(topThree.size()); i++) {
        cout << topThree[i].ticker << " : "
             << topThree[i].volume << '\n';
    }
}

void testAuditTrail() {
    cout << "\n===== AUDIT TRAIL TEST =====\n";

    AuditTrail audit("logs/audit_log.csv");

    audit.logOrderInsertion(101, "BUY", 100.00, 10);
    audit.logOrderInsertion(102, "SELL", 100.00, 5);
    audit.logTrade(101, 102, 5, 100.00);
    audit.logCancellation(103);

    cout << "Sample events written to logs/audit_log.csv\n";
}

void testHeapBenchmark() {
    cout << "\n===== HEAP BENCHMARK =====\n";

    int testSizes[] = {100, 1000, 10000, 100000};

    for (int s = 0; s < 4; s++) {
        int n = testSizes[s];

        Benchmark::measureMilliseconds(
            "Insert " + to_string(n) + " heap items",
            [n]() {
                MaxPriorityHeap heap;

                for (int i = 0; i < n; i++) {
                    heap.push(PriorityItem(i, (i * 37) % 100000));
                }
            }
        );
    }
}

int main() {
    testPriorityHeaps();
    testAuditTrail();
    testHeapBenchmark();

    return 0;
}
