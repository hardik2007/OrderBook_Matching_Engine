#include <iostream>
#include <list>
#include <algorithm>
#include <vector>
#include <string>
#include <cmath>

// Ayush's Modules
#include "../include/PriorityHeap.h"
#include "../include/AuditTrail.h"
#include "../include/Benchmark.h"

// Sahib's Modules
#include "../include/Trie.h"
#include "../include/Graph.h"

using namespace std;

const int BUY = 0;
const int SELL = 1;

// ==========================================
// HARDIK'S MODULE: Core AVL & Matching Engine
// ==========================================

class Order {
public:
    int orderId;
    int side;
    double price;
    int quantity;
    long long timestamp;

    Order(int id = 0, int s = BUY, double p = 0.0, int q = 0, long long ts = 0)
        : orderId(id), side(s), price(p), quantity(q), timestamp(ts) {}
};

class Pricelevel {
public:
    double price;
    int totalVolume;
    list<Order> orders; // JATIN: Replace with custom Doubly Linked List
    Pricelevel* left;
    Pricelevel* right;
    int height;

    Pricelevel(double p) {
        totalVolume = 0;
        left = nullptr;
        right = nullptr;
        height = 1;
        price = p;
    }
};

class PriceBookAVL {
private:
    Pricelevel* root = nullptr;

public:
    friend class OrderBook;

    PriceBookAVL() {
        root = nullptr;
    }

    int getHeight(Pricelevel* node) {
        if (node == nullptr) return 0;
        return node->height;
    }

    int getBalanceFactor(Pricelevel* node) {
        if (node == nullptr) return 0;
        return getHeight(node->left) - getHeight(node->right);
    }

    void updateHeight(Pricelevel* node) {
        node->height = 1 + max(getHeight(node->left), getHeight(node->right));
    }

    Pricelevel* rightRotate(Pricelevel* y) {
        Pricelevel* x = y->left;
        Pricelevel* t2 = x->right;
        x->right = y;
        y->left = t2;
        updateHeight(y);
        updateHeight(x);
        return x;
    }

    Pricelevel* leftRotate(Pricelevel* x) {
        Pricelevel* y = x->right;
        Pricelevel* t2 = y->left;
        y->left = x;
        x->right = t2;
        updateHeight(x);
        updateHeight(y);
        return y;
    }

    Pricelevel* insertHelper(Order& ord, Pricelevel* node) {
        if (node == nullptr) {
            Pricelevel* newNode = new Pricelevel(ord.price);
            newNode->orders.push_back(ord);
            newNode->totalVolume += ord.quantity;
            return newNode;
        }
        if (ord.price < node->price) {
            node->left = insertHelper(ord, node->left);
        } else if (ord.price > node->price) {
            node->right = insertHelper(ord, node->right);
        } else {
            node->orders.push_back(ord);
            node->totalVolume += ord.quantity;
            return node;
        }

        updateHeight(node);
        int bf = getBalanceFactor(node);

        if (bf > 1 && ord.price < node->left->price) {
            return rightRotate(node);
        }
        if (bf < -1 && ord.price > node->right->price) {
            return leftRotate(node);
        }
        if (bf > 1 && ord.price > node->left->price) {
            node->left = leftRotate(node->left);
            return rightRotate(node);
        }
        if (bf < -1 && ord.price < node->right->price) {
            node->right = rightRotate(node->right);
            return leftRotate(node);
        }

        return node;
    }

    void insertOrder(Order& ord) {
        root = insertHelper(ord, root);
    }

    void displayInOrder(Pricelevel* node) {
        if (node == nullptr) return;
        displayInOrder(node->left);
        if (node->totalVolume > 0) {
            cout << "  Price : " << node->price << " Rs. | Total Volume : " << node->totalVolume << " shares\n";
        }
        displayInOrder(node->right);
    }

    void printBook() {
        if (root == nullptr) {
            cout << "  [Empty]\n";
            return;
        }
        displayInOrder(root);
    }

    long long queryRangeHelper(Pricelevel* node, double lowPrice, double highPrice) {
        if (node == nullptr) return 0;
        if (node->price > highPrice) {
            return queryRangeHelper(node->left, lowPrice, highPrice);
        }
        if (node->price < lowPrice) {
            return queryRangeHelper(node->right, lowPrice, highPrice);
        }
        return node->totalVolume + queryRangeHelper(node->left, lowPrice, highPrice) + queryRangeHelper(node->right, lowPrice, highPrice);
    }

    long long getVolumeInRange(double lowPrice, double HighPrice) {
        return queryRangeHelper(root, lowPrice, HighPrice);
    }
};

class OrderBook {
private:
    AuditTrail* auditLogger;
    bool verbose;

public:
    string tickerSymbol;
    PriceBookAVL bids;
    PriceBookAVL asks;
    long long totalTradedVolume;

    OrderBook(string symbol = "", AuditTrail* logger = nullptr, bool printOutput = true)
        : auditLogger(logger), verbose(printOutput), tickerSymbol(symbol), totalTradedVolume(0) {}

    Pricelevel* getMinAsk(Pricelevel* node) {
        if (node == nullptr) return nullptr;
        while (node->left != nullptr) {
            node = node->left;
        }
        return node;
    }

    Pricelevel* getMaxBid(Pricelevel* node) {
        if (node == nullptr) return nullptr;
        while (node->right != nullptr) {
            node = node->right;
        }
        return node;
    }

    void processOrder(Order& ord) {
        if (ord.side == BUY) {
            while (ord.quantity > 0 && asks.root != nullptr) {
                Pricelevel* bestAsk = getMinAsk(asks.root);
                if (bestAsk->orders.empty() || ord.price < bestAsk->price) {
                    break;
                }

                Order& restingOrder = bestAsk->orders.front();
                int buyId = ord.orderId;
                int sellId = restingOrder.orderId;
                double tradePrice = bestAsk->price;

                if (ord.quantity >= restingOrder.quantity) {
                    int matchedQty = restingOrder.quantity;
                    ord.quantity -= matchedQty;
                    bestAsk->totalVolume -= matchedQty;
                    totalTradedVolume += matchedQty;
                    bestAsk->orders.pop_front();

                    if (auditLogger) auditLogger->logTrade(buyId, sellId, matchedQty, tradePrice);
                    if (verbose) {
                        cout << ">>> TRADE EXECUTED! Matched " << matchedQty << " shares at " << tradePrice << " Rs (Buy #" << buyId << " vs Sell #" << sellId << ")\n";
                    }

                    if (bestAsk->orders.empty()) {
                        // JATIN: trigger AVL node deletion here
                        break;
                    }
                } else {
                    int matchedQty = ord.quantity;
                    restingOrder.quantity -= matchedQty;
                    bestAsk->totalVolume -= matchedQty;
                    totalTradedVolume += matchedQty;
                    ord.quantity = 0;

                    if (auditLogger) auditLogger->logTrade(buyId, sellId, matchedQty, tradePrice);
                    if (verbose) {
                        cout << ">>> TRADE EXECUTED! Matched " << matchedQty << " shares at " << tradePrice << " Rs (Buy #" << buyId << " vs Sell #" << sellId << ")\n";
                    }
                    break;
                }
            }

            if (ord.quantity > 0) {
                bids.insertOrder(ord);
                if (auditLogger) auditLogger->logOrderInsertion(ord.orderId, "BUY", ord.price, ord.quantity);
                if (verbose) {
                    cout << ">>> Order #" << ord.orderId << " added to " << tickerSymbol << " Bids book (Resting: " << ord.quantity << " shares @ " << ord.price << " Rs)\n";
                }
            }
        } else {
            while (ord.quantity > 0 && bids.root != nullptr) {
                Pricelevel* bestBid = getMaxBid(bids.root);
                if (bestBid->orders.empty() || ord.price > bestBid->price) {
                    break;
                }

                Order& restingOrder = bestBid->orders.front();
                int buyId = restingOrder.orderId;
                int sellId = ord.orderId;
                double tradePrice = bestBid->price;

                if (ord.quantity >= restingOrder.quantity) {
                    int matchedQty = restingOrder.quantity;
                    ord.quantity -= matchedQty;
                    bestBid->totalVolume -= matchedQty;
                    totalTradedVolume += matchedQty;
                    bestBid->orders.pop_front();

                    if (auditLogger) auditLogger->logTrade(buyId, sellId, matchedQty, tradePrice);
                    if (verbose) {
                        cout << ">>> TRADE EXECUTED! Matched " << matchedQty << " shares at " << tradePrice << " Rs (Buy #" << buyId << " vs Sell #" << sellId << ")\n";
                    }

                    if (bestBid->orders.empty()) {
                        // JATIN: trigger AVL node deletion here
                        break;
                    }
                } else {
                    int matchedQty = ord.quantity;
                    restingOrder.quantity -= matchedQty;
                    bestBid->totalVolume -= matchedQty;
                    totalTradedVolume += matchedQty;
                    ord.quantity = 0;

                    if (auditLogger) auditLogger->logTrade(buyId, sellId, matchedQty, tradePrice);
                    if (verbose) {
                        cout << ">>> TRADE EXECUTED! Matched " << matchedQty << " shares at " << tradePrice << " Rs (Buy #" << buyId << " vs Sell #" << sellId << ")\n";
                    }
                    break;
                }
            }

            if (ord.quantity > 0) {
                asks.insertOrder(ord);
                if (auditLogger) auditLogger->logOrderInsertion(ord.orderId, "SELL", ord.price, ord.quantity);
                if (verbose) {
                    cout << ">>> Order #" << ord.orderId << " added to " << tickerSymbol << " Asks book (Resting: " << ord.quantity << " shares @ " << ord.price << " Rs)\n";
                }
            }
        }
    }
};

// ==========================================
// INTERACTIVE CLI MENU (STARTS EMPTY)
// ==========================================

int main() {
    AuditTrail auditLogger("logs/audit_log.csv");
    Trie tickerDirectory;
    vector<OrderBook> exchangeBooks;
    Graph forexNet;
    MaxPriorityHeap priorityOrderQueue;

    int nextOrderId = 1;
    long long logicalClock = 1000;
    int choice = -1;

    while (true) {
        cout << "\n========================================================\n";
        cout << "   ALGORITHMIC TRADING EXCHANGE - INTERACTIVE CLI\n";
        cout << "========================================================\n";
        cout << " [1] Register New Stock Ticker (Sahib's Trie)\n";
        cout << " [2] Search / Autocomplete Ticker (Sahib's Trie)\n";
        cout << " [3] Place BUY or SELL Order (Hardik's Engine + Ayush's Audit)\n";
        cout << " [4] View Order Book for a Ticker (Hardik's AVL In-Order)\n";
        cout << " [5] Query Market Depth in Price Range (Hardik's Range Query)\n";
        cout << " [6] Add Currency Exchange Rate (Sahib's Graph)\n";
        cout << " [7] Find Shortest Forex Conversion Path (Sahib's BFS)\n";
        cout << " [8] Detect Forex Arbitrage Cycle (Sahib's Bellman-Ford)\n";
        cout << " [9] Add & Process Priority Orders (Ayush's Max-Heap)\n";
        cout << "[10] View Top-K Traded Stocks by Volume (Ayush's Min-Heap)\n";
        cout << "[11] Run High-Load Benchmark Suite (Ayush's Benchmark)\n";
        cout << " [0] Exit\n";
        cout << "--------------------------------------------------------\n";
        cout << "Enter choice: ";
        
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(10000, '\n');
            continue;
        }

        if (choice == 0) {
            cout << "Exiting Exchange. All audit logs saved to logs/audit_log.csv\n";
            break;
        }

        switch (choice) {
            case 1: {
                string ticker;
                cout << "Enter new stock ticker symbol (e.g., RELIANCE, TCS): ";
                cin >> ticker;
                if (tickerDirectory.search(ticker)) {
                    cout << "Ticker '" << ticker << "' is already registered!\n";
                } else {
                    int newBookId = static_cast<int>(exchangeBooks.size());
                    exchangeBooks.push_back(OrderBook(ticker, &auditLogger, true));
                    tickerDirectory.insert(ticker, newBookId);
                    cout << "Registered '" << ticker << "' with OrderBook ID: " << newBookId << "\n";
                }
                break;
            }
            case 2: {
                string prefix;
                cout << "Enter ticker prefix to autocomplete: ";
                cin >> prefix;
                vector<string> results = tickerDirectory.autocomplete(prefix);
                if (results.empty()) {
                    cout << "No matching tickers found for prefix '" << prefix << "'.\n";
                } else {
                    cout << "Matching Tickers: ";
                    for (const string& s : results) cout << s << " ";
                    cout << "\n";
                }
                break;
            }
            case 3: {
                string ticker;
                cout << "Enter stock ticker: ";
                cin >> ticker;
                int bookId = tickerDirectory.getOrderBookId(ticker);
                if (bookId == -1) {
                    cout << "Ticker not found! Please register it first using Option [1].\n";
                    break;
                }

                int sideInput, qty;
                double price;
                cout << "Select Side (0 for BUY, 1 for SELL): ";
                cin >> sideInput;
                cout << "Enter Price (Rs): ";
                cin >> price;
                cout << "Enter Quantity (shares): ";
                cin >> qty;

                if ((sideInput != BUY && sideInput != SELL) || price <= 0 || qty <= 0) {
                    cout << "Invalid order parameters!\n";
                    break;
                }

                Order ord(nextOrderId++, sideInput, price, qty, logicalClock++);
                exchangeBooks[bookId].processOrder(ord);
                break;
            }
            case 4: {
                string ticker;
                cout << "Enter stock ticker to view Order Book: ";
                cin >> ticker;
                int bookId = tickerDirectory.getOrderBookId(ticker);
                if (bookId == -1) {
                    cout << "Ticker not found!\n";
                    break;
                }
                cout << "\n--- " << exchangeBooks[bookId].tickerSymbol << " ASKS (SELL) BOOK ---\n";
                exchangeBooks[bookId].asks.printBook();
                cout << "\n--- " << exchangeBooks[bookId].tickerSymbol << " BIDS (BUY) BOOK ---\n";
                exchangeBooks[bookId].bids.printBook();
                break;
            }
            case 5: {
                string ticker;
                cout << "Enter stock ticker: ";
                cin >> ticker;
                int bookId = tickerDirectory.getOrderBookId(ticker);
                if (bookId == -1) {
                    cout << "Ticker not found!\n";
                    break;
                }
                int sideInput;
                double lowP, highP;
                cout << "Query which book? (0 for BIDS, 1 for ASKS): ";
                cin >> sideInput;
                cout << "Enter Low Price (Rs): ";
                cin >> lowP;
                cout << "Enter High Price (Rs): ";
                cin >> highP;

                long long vol = (sideInput == BUY)
                    ? exchangeBooks[bookId].bids.getVolumeInRange(lowP, highP)
                    : exchangeBooks[bookId].asks.getVolumeInRange(lowP, highP);

                cout << "Total Volume between " << lowP << " Rs and " << highP << " Rs: " << vol << " shares\n";
                break;
            }
            case 6: {
                string fromCurr, toCurr;
                double rate;
                int dir;
                cout << "Enter Source Currency (e.g., USD): ";
                cin >> fromCurr;
                cout << "Enter Target Currency (e.g., INR): ";
                cin >> toCurr;
                cout << "Enter Conversion Rate (1 " << fromCurr << " = ? " << toCurr << "): ";
                cin >> rate;
                cout << "Add as (1 = One-way directed edge, 2 = Two-way reciprocal pair): ";
                cin >> dir;

                bool ok = (dir == 2) ? forexNet.addPair(fromCurr, toCurr, rate)
                                     : forexNet.addEdge(fromCurr, toCurr, rate);
                if (ok) cout << "Rate added to Forex Graph.\n";
                else cout << "Invalid rate (must be > 0).\n";
                break;
            }
            case 7: {
                string src, dst;
                cout << "Enter Starting Currency: ";
                cin >> src;
                cout << "Enter Target Currency: ";
                cin >> dst;
                vector<string> path = forexNet.bfsPath(src, dst);
                if (path.empty()) {
                    cout << "No conversion path exists between " << src << " and " << dst << ".\n";
                } else {
                    cout << "Shortest Hop Path: ";
                    for (size_t i = 0; i < path.size(); i++) {
                        cout << path[i] << (i + 1 < path.size() ? " -> " : "");
                    }
                    cout << " | Effective Rate: " << forexNet.pathRate(path) << "\n";
                }
                break;
            }
            case 8: {
                double profitFactor = 0.0;
                vector<string> cycle = forexNet.findArbitrage(&profitFactor);
                if (cycle.empty()) {
                    cout << "No arbitrage cycle detected in current Forex Graph.\n";
                } else {
                    cout << "ARBITRAGE DETECTED! Cycle: ";
                    for (size_t i = 0; i < cycle.size(); i++) {
                        cout << cycle[i] << (i + 1 < cycle.size() ? " -> " : "");
                    }
                    cout << " | Profit Factor: " << profitFactor << "x\n";
                }
                break;
            }
            case 9: {
                int subChoice;
                cout << "(1) Push Priority Order to Queue | (2) Pop & Execute All Priority Orders: ";
                cin >> subChoice;
                if (subChoice == 1) {
                    int id, prio;
                    cout << "Enter Order ID: ";
                    cin >> id;
                    cout << "Enter Priority Score (higher executes first): ";
                    cin >> prio;
                    priorityOrderQueue.push(PriorityItem(id, prio));
                    cout << "Order #" << id << " queued with priority " << prio << ".\n";
                } else {
                    if (priorityOrderQueue.empty()) {
                        cout << "Priority queue is currently empty.\n";
                    } else {
                        cout << "Executing Priority Queue:\n";
                        while (!priorityOrderQueue.empty()) {
                            PriorityItem item = priorityOrderQueue.pop();
                            cout << "  -> Executed Order ID: " << item.orderId << " (Priority: " << item.priority << ")\n";
                        }
                    }
                }
                break;
            }
            case 10: {
                if (exchangeBooks.empty()) {
                    cout << "No stocks registered yet!\n";
                    break;
                }
                int k;
                cout << "Enter K (number of top traded stocks to display): ";
                cin >> k;

                vector<StockVolume> liveVolumes;
                for (const auto& book : exchangeBooks) {
                    liveVolumes.push_back(StockVolume(book.tickerSymbol, book.totalTradedVolume));
                }

                vector<StockVolume> topK = getTopKVolumes(liveVolumes, k);
                cout << "\n--- Top " << topK.size() << " Stocks by Executed Trade Volume ---\n";
                for (const auto& sv : topK) {
                    cout << "  " << sv.ticker << " : " << sv.volume << " shares traded\n";
                }
                break;
            }
            case 11: {
                int n;
                cout << "Enter number of items to stress-test (e.g., 10000 or 100000): ";
                cin >> n;
                if (n <= 0) break;

                Benchmark::measureMilliseconds("AVL Tree Insert (" + to_string(n) + " orders)", [n]() {
                    PriceBookAVL benchTree;
                    for (int i = 0; i < n; i++) {
                        Order ord(i, BUY, 100.0 + (i % 500) * 0.05, 10, i);
                        benchTree.insertOrder(ord);
                    }
                });

                Benchmark::measureMilliseconds("Priority Heap Push (" + to_string(n) + " items)", [n]() {
                    MaxPriorityHeap heap;
                    for (int i = 0; i < n; i++) {
                        heap.push(PriorityItem(i, (i * 37) % 100000));
                    }
                });
                break;
            }
            default:
                cout << "Invalid choice. Please select [0-11].\n";
        }
    }

    return 0;
}