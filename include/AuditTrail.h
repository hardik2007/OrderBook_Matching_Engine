#ifndef AUDIT_TRAIL_H
#define AUDIT_TRAIL_H

#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <ctime>
#include <filesystem>

using namespace std;

// Writes a CSV record whenever an important trading event occurs.
class AuditTrail {
private:
    string fileName;
    ofstream outputFile;

    string getCurrentTime() {
        time_t now = time(nullptr);
        tm* local = localtime(&now);

        if (local == nullptr) {
            return "UNKNOWN_TIME";
        }

        char buffer[32];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", local);
        return string(buffer);
    }

    void writeEvent(
        const string& event,
        int orderId,
        const string& side,
        double price,
        int quantity,
        int buyOrderId,
        int sellOrderId
    ) {
        if (!outputFile.is_open()) {
            cerr << "Audit log file is not open.\n";
            return;
        }

        outputFile << '"' << getCurrentTime() << "\","
                   << event << ","
                   << orderId << ","
                   << side << ","
                   << fixed << setprecision(2) << price << ","
                   << quantity << ","
                   << buyOrderId << ","
                   << sellOrderId << '\n';

        outputFile.flush();
    }

public:
    explicit AuditTrail(const string& path = "logs/audit_log.csv") {
        fileName = path;

        filesystem::path logPath(fileName);
        if (logPath.has_parent_path()) {
            filesystem::create_directories(logPath.parent_path());
        }

        bool needsHeader = !filesystem::exists(logPath)
                           || filesystem::file_size(logPath) == 0;

        outputFile.open(fileName, ios::app);

        if (outputFile.is_open() && needsHeader) {
            outputFile << "timestamp,event,order_id,side,price,quantity,"
                       << "buy_order_id,sell_order_id\n";
            outputFile.flush();
        }
    }

    ~AuditTrail() {
        if (outputFile.is_open()) {
            outputFile.close();
        }
    }

    // Call only after an order has actually been accepted into the book.
    void logOrderInsertion(
        int orderId, const string& side, double price, int quantity
    ) {
        writeEvent("ORDER_INSERTED", orderId, side, price, quantity, 0, 0);
    }

    // Call after a trade has successfully executed.
    void logTrade(
        int buyOrderId, int sellOrderId, int quantity, double price
    ) {
        writeEvent(
            "TRADE_EXECUTED", 0, "TRADE", price, quantity,
            buyOrderId, sellOrderId
        );
    }

    // Call only after cancellation succeeds.
    void logCancellation(int orderId) {
        writeEvent("ORDER_CANCELLED", orderId, "", 0.0, 0, 0, 0);
    }
};

#endif
