#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <memory>

// Ayush's Modules
#include "../include/PriorityHeap.h"
#include "../include/AuditTrail.h"
#include "../include/Benchmark.h"

// Sahib's Modules
#include "../include/Trie.h"
#include "../include/Graph.h"

using namespace std;

constexpr int BUY = 0;
constexpr int SELL = 1;

// =========================================================
// HARDIK & JATIN: Order, Custom DLL, Pricelevel & AVL Tree
// =========================================================

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

class Pricelevel;

// JATIN: Heap-allocated DLL node storing a back-pointer to its owning Pricelevel
struct OrderNode {
    Order order;
    OrderNode* prev = nullptr;
    OrderNode* next = nullptr;
    Pricelevel* level = nullptr;

    OrderNode(const Order& value, Pricelevel* owner)
        : order(value), level(owner) {}
};

// JATIN: Custom FIFO Doubly Linked List supporting O(1) arbitrary node erasure
class DoublyLinkedList {
private:
    OrderNode* head_ = nullptr;
    OrderNode* tail_ = nullptr;
    size_t size_ = 0;

    void unlinkAndDelete(OrderNode* node) noexcept {
        if (node->prev != nullptr) {
            node->prev->next = node->next;
        } else {
            head_ = node->next;
        }

        if (node->next != nullptr) {
            node->next->prev = node->prev;
        } else {
            tail_ = node->prev;
        }

        delete node;
        --size_;
    }

public:
    DoublyLinkedList() = default;
    ~DoublyLinkedList() { clear(); }

    DoublyLinkedList(const DoublyLinkedList&) = delete;
    DoublyLinkedList& operator=(const DoublyLinkedList&) = delete;

    OrderNode* push_back(const Order& order, Pricelevel* owner) {
        OrderNode* node = new OrderNode(order, owner);
        node->prev = tail_;

        if (tail_ != nullptr) {
            tail_->next = node;
        } else {
            head_ = node;
        }

        tail_ = node;
        ++size_;
        return node;
    }

    OrderNode* frontNode() const noexcept { return head_; }
    OrderNode* backNode() const noexcept { return tail_; }

    Order& front() {
        if (head_ == nullptr) {
            throw out_of_range("front() called on an empty order queue");
        }
        return head_->order;
    }

    const Order& front() const {
        if (head_ == nullptr) {
            throw out_of_range("front() called on an empty order queue");
        }
        return head_->order;
    }

    bool empty() const noexcept { return size_ == 0; }
    size_t size() const noexcept { return size_; }

    void pop_front() noexcept {
        if (head_ != nullptr) {
            unlinkAndDelete(head_);
        }
    }

    bool erase(OrderNode* node, const Pricelevel* expectedOwner) noexcept {
        if (node == nullptr || node->level != expectedOwner) {
            return false;
        }
        unlinkAndDelete(node);
        return true;
    }

    void clear() noexcept {
        while (head_ != nullptr) {
            pop_front();
        }
    }

    bool validateLinks(const Pricelevel* expectedOwner, long long& volume,
                       size_t& count) const noexcept {
        const OrderNode* previous = nullptr;
        const OrderNode* current = head_;
        size_t seen = 0;
        volume = 0;

        while (current != nullptr) {
            if (current->prev != previous || current->level != expectedOwner ||
                current->order.quantity <= 0) {
                return false;
            }
            volume += current->order.quantity;
            ++seen;
            previous = current;
            current = current->next;
        }

        count = seen;
        return previous == tail_ && seen == size_ &&
               ((head_ == nullptr) == (tail_ == nullptr));
    }
};

class Pricelevel {
public:
    double price;
    long long totalVolume;
    DoublyLinkedList orders;
    Pricelevel* left;
    Pricelevel* right;
    int height;

    explicit Pricelevel(double p)
        : price(p), totalVolume(0), left(nullptr), right(nullptr), height(1) {}

    Pricelevel(const Pricelevel&) = delete;
    Pricelevel& operator=(const Pricelevel&) = delete;
};

class OrderBook;

class PriceBookAVL {
private:
    Pricelevel* root = nullptr;

    static void destroyTree(Pricelevel* node) noexcept {
        if (node == nullptr) return;
        destroyTree(node->left);
        destroyTree(node->right);
        delete node;
    }

    // JATIN: Rebalancing helper after AVL node removal
    Pricelevel* rebalanceAfterDeletion(Pricelevel* node) {
        if (node == nullptr) return nullptr;

        updateHeight(node);
        const int bf = getBalanceFactor(node);

        if (bf > 1) {
            if (getBalanceFactor(node->left) < 0) {
                node->left = leftRotate(node->left);
            }
            return rightRotate(node);
        }

        if (bf < -1) {
            if (getBalanceFactor(node->right) > 0) {
                node->right = rightRotate(node->right);
            }
            return leftRotate(node);
        }

        return node;
    }

    // JATIN: Detaches the in-order successor without invalidating OrderNode::level pointers
    Pricelevel* detachMin(Pricelevel* node, Pricelevel*& minimum) {
        if (node->left == nullptr) {
            minimum = node;
            Pricelevel* replacement = node->right;
            minimum->right = nullptr;
            return replacement;
        }

        node->left = detachMin(node->left, minimum);
        return rebalanceAfterDeletion(node);
    }

    Pricelevel* removeHelper(Pricelevel* node, double price, bool& removed) {
        if (node == nullptr) return nullptr;

        if (price < node->price) {
            node->left = removeHelper(node->left, price, removed);
        } else if (price > node->price) {
            node->right = removeHelper(node->right, price, removed);
        } else {
            removed = true;

            if (node->left == nullptr) {
                Pricelevel* replacement = node->right;
                node->right = nullptr;
                delete node;
                return replacement;
            }

            if (node->right == nullptr) {
                Pricelevel* replacement = node->left;
                node->left = nullptr;
                delete node;
                return replacement;
            }

            Pricelevel* successor = nullptr;
            Pricelevel* newRight = detachMin(node->right, successor);
            successor->left = node->left;
            successor->right = newRight;

            node->left = nullptr;
            node->right = nullptr;
            delete node;
            return rebalanceAfterDeletion(successor);
        }

        return rebalanceAfterDeletion(node);
    }

    bool validateTreeHelper(const Pricelevel* node, double lower,
                            double upper, int& computedHeight,
                            size_t& nodeCount) const {
        if (node == nullptr) {
            computedHeight = 0;
            return true;
        }

        if (!(node->price > lower && node->price < upper)) {
            return false;
        }

        int leftHeight = 0;
        int rightHeight = 0;
        if (!validateTreeHelper(node->left, lower, node->price,
                                leftHeight, nodeCount) ||
            !validateTreeHelper(node->right, node->price, upper,
                                rightHeight, nodeCount)) {
            return false;
        }

        computedHeight = 1 + max(leftHeight, rightHeight);
        ++nodeCount;
        return node->height == computedHeight &&
               abs(leftHeight - rightHeight) <= 1;
    }

public:
    friend class OrderBook;

    PriceBookAVL() = default;
    ~PriceBookAVL() { destroyTree(root); }

    PriceBookAVL(const PriceBookAVL&) = delete;
    PriceBookAVL& operator=(const PriceBookAVL&) = delete;

    // HARDIK: Core AVL height, balance, rotations, and insertion
    int getHeight(Pricelevel* node) const {
        if (node == nullptr) return 0;
        return node->height;
    }

    int getBalanceFactor(Pricelevel* node) const {
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
            newNode->orders.push_back(ord, newNode);
            newNode->totalVolume += ord.quantity;
            return newNode;
        }
        if (ord.price < node->price) {
            node->left = insertHelper(ord, node->left);
        } else if (ord.price > node->price) {
            node->right = insertHelper(ord, node->right);
        } else {
            node->orders.push_back(ord, node);
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

    Pricelevel* findPriceLevel(double price) noexcept {
        Pricelevel* current = root;
        while (current != nullptr) {
            if (price < current->price) current = current->left;
            else if (price > current->price) current = current->right;
            else return current;
        }
        return nullptr;
    }

    const Pricelevel* findPriceLevel(double price) const noexcept {
        const Pricelevel* current = root;
        while (current != nullptr) {
            if (price < current->price) current = current->left;
            else if (price > current->price) current = current->right;
            else return current;
        }
        return nullptr;
    }

    bool removePriceLevel(double price) {
        Pricelevel* target = findPriceLevel(price);
        if (target == nullptr || !target->orders.empty() || target->totalVolume != 0) {
            return false;
        }

        bool removed = false;
        root = removeHelper(root, price, removed);
        return removed;
    }

    void displayInOrder(Pricelevel* node) const {
        if (node == nullptr) return;
        displayInOrder(node->left);
        cout << "  Price : " << node->price << " Rs. | Total Volume : " << node->totalVolume
             << " shares (" << node->orders.size() << " orders)\n";
        displayInOrder(node->right);
    }

    void printBook() const {
        if (root == nullptr) {
            cout << "  [Empty]\n";
            return;
        }
        displayInOrder(root);
    }

    long long queryRangeHelper(const Pricelevel* node, double lowPrice, double highPrice) const {
        if (node == nullptr) return 0;
        if (node->price > highPrice) {
            return queryRangeHelper(node->left, lowPrice, highPrice);
        }
        if (node->price < lowPrice) {
            return queryRangeHelper(node->right, lowPrice, highPrice);
        }
        return node->totalVolume +
               queryRangeHelper(node->left, lowPrice, highPrice) +
               queryRangeHelper(node->right, lowPrice, highPrice);
    }

    long long getVolumeInRange(double lowPrice, double highPrice) const {
        if (lowPrice > highPrice) return 0;
        return queryRangeHelper(root, lowPrice, highPrice);
    }

    bool validate() const {
        int computedHeight = 0;
        size_t count = 0;
        return validateTreeHelper(root,
                                  -numeric_limits<double>::infinity(),
                                  numeric_limits<double>::infinity(),
                                  computedHeight, count);
    }
};

// =========================================================
// ORDER BOOK: Hardik's Matching + Jatin's Cancellation + Ayush's Audit
// =========================================================

class OrderBook {
private:
    AuditTrail* auditLogger;
    bool verbose;
    unordered_map<int, OrderNode*> orderIndex; // JATIN: O(1) orderId -> OrderNode* index

    static bool isValidOrder(const Order& order) noexcept {
        return (order.side == BUY || order.side == SELL) &&
               isfinite(order.price) && order.price > 0.0 &&
               order.quantity > 0;
    }

    void addRestingOrder(PriceBookAVL& book, Order& order) {
        book.insertOrder(order);
        Pricelevel* level = book.findPriceLevel(order.price);
        OrderNode* node = level->orders.backNode();
        orderIndex.emplace(order.orderId, node);
    }

    bool validateLevels(const Pricelevel* node, int expectedSide,
                        size_t& countedOrders) const {
        if (node == nullptr) return true;
        if (!validateLevels(node->left, expectedSide, countedOrders)) return false;

        long long queueVolume = 0;
        size_t queueCount = 0;
        if (!node->orders.validateLinks(node, queueVolume, queueCount) ||
            queueVolume != node->totalVolume || node->totalVolume <= 0) {
            return false;
        }

        for (OrderNode* current = node->orders.frontNode(); current != nullptr;
             current = current->next) {
            if (current->order.side != expectedSide) return false;
            const auto it = orderIndex.find(current->order.orderId);
            if (it == orderIndex.end() || it->second != current) return false;
            ++countedOrders;
        }

        return validateLevels(node->right, expectedSide, countedOrders);
    }

public:
    string tickerSymbol;
    PriceBookAVL bids;
    PriceBookAVL asks;
    long long totalTradedVolume;

    OrderBook(string symbol = "", AuditTrail* logger = nullptr, bool printOutput = true)
        : auditLogger(logger), verbose(printOutput), tickerSymbol(symbol), totalTradedVolume(0) {}

    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;

    Pricelevel* getMinAsk(Pricelevel* node) const {
        if (node == nullptr) return nullptr;
        while (node->left != nullptr) node = node->left;
        return node;
    }

    Pricelevel* getMaxBid(Pricelevel* node) const {
        if (node == nullptr) return nullptr;
        while (node->right != nullptr) node = node->right;
        return node;
    }

    bool processOrder(Order& ord) {
        if (!isValidOrder(ord)) {
            cerr << "Rejected order " << ord.orderId << ": invalid side, price, or quantity.\n";
            return false;
        }
        if (orderIndex.find(ord.orderId) != orderIndex.end()) {
            cerr << "Rejected order " << ord.orderId << ": order ID is already resting in the book.\n";
            return false;
        }

        if (ord.side == BUY) {
            while (ord.quantity > 0 && asks.root != nullptr) {
                Pricelevel* bestAsk = getMinAsk(asks.root);
                if (ord.price < bestAsk->price) {
                    break;
                }

                OrderNode* restingNode = bestAsk->orders.frontNode();
                Order& restingOrder = restingNode->order;
                const int buyId = ord.orderId;
                const int sellId = restingOrder.orderId;
                const double tradePrice = bestAsk->price;

                if (ord.quantity >= restingOrder.quantity) {
                    const int matchedQty = restingOrder.quantity;
                    ord.quantity -= matchedQty;
                    bestAsk->totalVolume -= matchedQty;
                    totalTradedVolume += matchedQty;
                    orderIndex.erase(sellId);
                    bestAsk->orders.pop_front();

                    if (auditLogger) auditLogger->logTrade(buyId, sellId, matchedQty, tradePrice);
                    if (verbose) {
                        cout << ">>> TRADE EXECUTED! Matched " << matchedQty << " shares at "
                             << tradePrice << " Rs (Buy #" << buyId << " vs Sell #" << sellId << ")\n";
                    }

                    if (bestAsk->orders.empty()) {
                        asks.removePriceLevel(tradePrice);
                    }
                } else {
                    const int matchedQty = ord.quantity;
                    restingOrder.quantity -= matchedQty;
                    bestAsk->totalVolume -= matchedQty;
                    totalTradedVolume += matchedQty;
                    ord.quantity = 0;

                    if (auditLogger) auditLogger->logTrade(buyId, sellId, matchedQty, tradePrice);
                    if (verbose) {
                        cout << ">>> TRADE EXECUTED! Matched " << matchedQty << " shares at "
                             << tradePrice << " Rs (Buy #" << buyId << " vs Sell #" << sellId << ")\n";
                    }
                    break;
                }
            }

            if (ord.quantity > 0) {
                addRestingOrder(bids, ord);
                if (auditLogger) auditLogger->logOrderInsertion(ord.orderId, "BUY", ord.price, ord.quantity);
                if (verbose) {
                    cout << ">>> Order #" << ord.orderId << " added to " << tickerSymbol
                         << " Bids book (Resting: " << ord.quantity << " shares @ " << ord.price << " Rs)\n";
                }
            }
        } else {
            while (ord.quantity > 0 && bids.root != nullptr) {
                Pricelevel* bestBid = getMaxBid(bids.root);
                if (ord.price > bestBid->price) {
                    break;
                }

                OrderNode* restingNode = bestBid->orders.frontNode();
                Order& restingOrder = restingNode->order;
                const int buyId = restingOrder.orderId;
                const int sellId = ord.orderId;
                const double tradePrice = bestBid->price;

                if (ord.quantity >= restingOrder.quantity) {
                    const int matchedQty = restingOrder.quantity;
                    ord.quantity -= matchedQty;
                    bestBid->totalVolume -= matchedQty;
                    totalTradedVolume += matchedQty;
                    orderIndex.erase(buyId);
                    bestBid->orders.pop_front();

                    if (auditLogger) auditLogger->logTrade(buyId, sellId, matchedQty, tradePrice);
                    if (verbose) {
                        cout << ">>> TRADE EXECUTED! Matched " << matchedQty << " shares at "
                             << tradePrice << " Rs (Buy #" << buyId << " vs Sell #" << sellId << ")\n";
                    }

                    if (bestBid->orders.empty()) {
                        bids.removePriceLevel(tradePrice);
                    }
                } else {
                    const int matchedQty = ord.quantity;
                    restingOrder.quantity -= matchedQty;
                    bestBid->totalVolume -= matchedQty;
                    totalTradedVolume += matchedQty;
                    ord.quantity = 0;

                    if (auditLogger) auditLogger->logTrade(buyId, sellId, matchedQty, tradePrice);
                    if (verbose) {
                        cout << ">>> TRADE EXECUTED! Matched " << matchedQty << " shares at "
                             << tradePrice << " Rs (Buy #" << buyId << " vs Sell #" << sellId << ")\n";
                    }
                    break;
                }
            }

            if (ord.quantity > 0) {
                addRestingOrder(asks, ord);
                if (auditLogger) auditLogger->logOrderInsertion(ord.orderId, "SELL", ord.price, ord.quantity);
                if (verbose) {
                    cout << ">>> Order #" << ord.orderId << " added to " << tickerSymbol
                         << " Asks book (Resting: " << ord.quantity << " shares @ " << ord.price << " Rs)\n";
                }
            }
        }

        return true;
    }

    // JATIN: O(1) Hash-Indexed Order Cancellation + AVL Price Level Cleanup + Ayush's Audit Log
    bool cancelOrder(int orderId) {
        const auto it = orderIndex.find(orderId);
        if (it == orderIndex.end()) return false;

        OrderNode* node = it->second;
        Pricelevel* level = node->level;
        const int side = node->order.side;
        const int remainingQuantity = node->order.quantity;
        const double price = level->price;

        if (!level->orders.erase(node, level)) {
            return false;
        }
        orderIndex.erase(it);

        level->totalVolume -= remainingQuantity;
        if (auditLogger) auditLogger->logCancellation(orderId);
        if (verbose) {
            cout << ">>> Order #" << orderId << " CANCELLED from " << tickerSymbol
                 << " (" << remainingQuantity << " shares removed @ " << price << " Rs)\n";
        }

        if (level->orders.empty()) {
            if (side == BUY) {
                bids.removePriceLevel(price);
            } else {
                asks.removePriceLevel(price);
            }
        }
        return true;
    }

    bool containsOrder(int orderId) const {
        return orderIndex.find(orderId) != orderIndex.end();
    }

    int getRestingQuantity(int orderId) const {
        const auto it = orderIndex.find(orderId);
        return it == orderIndex.end() ? 0 : it->second->order.quantity;
    }

    size_t restingOrderCount() const noexcept { return orderIndex.size(); }

    bool validateInvariants() const {
        if (!bids.validate() || !asks.validate()) return false;

        size_t countedOrders = 0;
        if (!validateLevels(bids.root, BUY, countedOrders) ||
            !validateLevels(asks.root, SELL, countedOrders)) {
            return false;
        }
        return countedOrders == orderIndex.size();
    }
};

// =========================================================
// INTERACTIVE CLI MENU (ALL 4 MODULES INTEGRATED)
// =========================================================

int main() {
    AuditTrail auditLogger("logs/audit_log.csv");
    Trie tickerDirectory;
    vector<unique_ptr<OrderBook>> exchangeBooks;
    unordered_map<int, int> orderToBookMap; // Maps global orderId -> exchangeBooks index
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
        cout << " [4] Cancel Resting Order by ID (Jatin's DLL + Hash Map)\n";
        cout << " [5] View Order Book & Verify Invariants (Hardik + Jatin)\n";
        cout << " [6] Query Market Depth in Price Range (Hardik's Range Query)\n";
        cout << " [7] Add Currency Exchange Rate (Sahib's Graph)\n";
        cout << " [8] Find Shortest Forex Conversion Path (Sahib's BFS)\n";
        cout << " [9] Detect Forex Arbitrage Cycle (Sahib's Bellman-Ford)\n";
        cout << "[10] Add & Process Priority Orders (Ayush's Max-Heap)\n";
        cout << "[11] View Top-K Traded Stocks by Volume (Ayush's Min-Heap)\n";
        cout << "[12] Run High-Load Benchmark Suite (Ayush's Benchmark)\n";
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
                    exchangeBooks.push_back(make_unique<OrderBook>(ticker, &auditLogger, true));
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

                int assignedId = nextOrderId++;
                Order ord(assignedId, sideInput, price, qty, logicalClock++);
                if (exchangeBooks[bookId]->processOrder(ord)) {
                    if (exchangeBooks[bookId]->containsOrder(assignedId)) {
                        orderToBookMap[assignedId] = bookId;
                    }
                }
                break;
            }
            case 4: {
                int cancelId;
                cout << "Enter Order ID to cancel: ";
                cin >> cancelId;

                auto it = orderToBookMap.find(cancelId);
                if (it == orderToBookMap.end()) {
                    cout << "Order #" << cancelId << " is not currently resting in any Order Book.\n";
                    break;
                }

                int bookId = it->second;
                if (exchangeBooks[bookId]->cancelOrder(cancelId)) {
                    orderToBookMap.erase(it);
                } else {
                    cout << "Order #" << cancelId << " was already filled or cancelled.\n";
                    orderToBookMap.erase(it);
                }
                break;
            }
            case 5: {
                string ticker;
                cout << "Enter stock ticker to view Order Book: ";
                cin >> ticker;
                int bookId = tickerDirectory.getOrderBookId(ticker);
                if (bookId == -1) {
                    cout << "Ticker not found!\n";
                    break;
                }
                cout << "\n--- " << exchangeBooks[bookId]->tickerSymbol << " ASKS (SELL) BOOK ---\n";
                exchangeBooks[bookId]->asks.printBook();
                cout << "\n--- " << exchangeBooks[bookId]->tickerSymbol << " BIDS (BUY) BOOK ---\n";
                exchangeBooks[bookId]->bids.printBook();
                cout << "\nResting Orders Indexed: " << exchangeBooks[bookId]->restingOrderCount()
                     << " | Structural Invariant Check: "
                     << (exchangeBooks[bookId]->validateInvariants() ? "PASS" : "FAIL") << "\n";
                break;
            }
            case 6: {
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
                    ? exchangeBooks[bookId]->bids.getVolumeInRange(lowP, highP)
                    : exchangeBooks[bookId]->asks.getVolumeInRange(lowP, highP);

                cout << "Total Volume between " << lowP << " Rs and " << highP << " Rs: " << vol << " shares\n";
                break;
            }
            case 7: {
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
            case 8: {
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
            case 9: {
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
            case 10: {
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
            case 11: {
                if (exchangeBooks.empty()) {
                    cout << "No stocks registered yet!\n";
                    break;
                }
                int k;
                cout << "Enter K (number of top traded stocks to display): ";
                cin >> k;

                vector<StockVolume> liveVolumes;
                for (const auto& book : exchangeBooks) {
                    liveVolumes.push_back(StockVolume(book->tickerSymbol, book->totalTradedVolume));
                }

                vector<StockVolume> topK = getTopKVolumes(liveVolumes, k);
                cout << "\n--- Top " << topK.size() << " Stocks by Executed Trade Volume ---\n";
                for (const auto& sv : topK) {
                    cout << "  " << sv.ticker << " : " << sv.volume << " shares traded\n";
                }
                break;
            }
            case 12: {
                int n;
                cout << "Enter number of items to stress-test (e.g., 10000 or 100000): ";
                cin >> n;
                if (n <= 0) break;

                Benchmark::measureMilliseconds("OrderBook Insert + Cancel (" + to_string(n) + " orders)", [n]() {
                    OrderBook benchBook("BENCH", nullptr, false);
                    for (int i = 1; i <= n; i++) {
                        Order ord(i, BUY, 100.0 + (i % 500) * 0.05, 10, i);
                        benchBook.processOrder(ord);
                    }
                    for (int i = 1; i <= n; i += 2) {
                        benchBook.cancelOrder(i);
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
                cout << "Invalid choice. Please select [0-12].\n";
        }
    }

    return 0;
}