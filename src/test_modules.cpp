#include <iostream>
#include "Trie.h"
#include "Graph.h"
using namespace std;

int failures = 0;

void check(bool cond, const string& label) {
    cout << (cond ? "[PASS] " : "[FAIL] ") << label << endl;
    if (!cond) failures++;
}

void printList(const string& title, const vector<string>& v) {
    cout << title << ": ";
    if (v.empty()) cout << "(none)";
    for (size_t i = 0; i < v.size(); i++) cout << v[i] << (i + 1 < v.size() ? " " : "");
    cout << endl;
}

void testTrie() {
    cout << "\n=== TRIE (ticker search) ===" << endl;
    Trie t;
    t.insert("RELIANCE", 1);
    t.insert("REC", 2);
    t.insert("RELINFRA", 3);
    t.insert("TCS", 4);
    t.insert("TATAMOTORS", 5);

    check(t.search("RELIANCE"), "search RELIANCE (exact)");
    check(t.search("reliance"), "search is case-insensitive");
    check(!t.search("REL"), "REL is only a prefix, not a ticker");
    check(!t.search("INFY"), "INFY was never inserted");
    check(t.startsWith("REL"), "startsWith REL");
    check(t.getOrderBookId("TCS") == 4, "orderBookId of TCS is 4");
    check(t.getOrderBookId("INFY") == -1, "orderBookId of missing ticker is -1");

    vector<string> r = t.autocomplete("RE");
    printList("autocomplete(RE)", r);
    check(r.size() == 3 && r[0] == "REC" && r[1] == "RELIANCE" && r[2] == "RELINFRA", "autocomplete RE gives REC, RELIANCE, RELINFRA in order");

    vector<string> r2 = t.autocomplete("T", 2);
    printList("autocomplete(T, limit 2)", r2);
    check(r2.size() == 2, "limit respected");
    check(t.autocomplete("ZZ").empty(), "unknown prefix gives empty list");

    check(t.remove("REC"), "remove REC");
    check(!t.search("REC"), "REC gone after removal");
    check(t.search("RELIANCE"), "RELIANCE still there after removing REC");
    check(!t.remove("REC"), "removing REC again returns false");
    check(t.remove("TCS"), "remove TCS");
    check(t.autocomplete("TC").empty(), "TC branch pruned");
    check(t.search("TATAMOTORS"), "TATAMOTORS unaffected");
}

void testGraphBfs() {
    cout << "\n=== GRAPH: BFS routing ===" << endl;
    Graph g;
    g.addPair("INR", "USD", 0.0125);
    g.addPair("USD", "EUR", 0.9);
    g.addPair("EUR", "GBP", 0.85);

    vector<string> p = g.bfsPath("INR", "GBP");
    printList("path INR -> GBP", p);
    check(p.size() == 4 && p.front() == "INR" && p.back() == "GBP", "INR to GBP takes 3 hops via USD, EUR");
    cout << "rate along path: " << g.pathRate(p) << endl;
    check(fabs(g.pathRate(p) - 0.0125 * 0.9 * 0.85) < 1e-12, "pathRate multiplies hop rates");

    g.addEdge("BTC", "ETH", 15.0); // BTC reachable to ETH only; nothing leads into BTC
    check(g.bfsPath("INR", "BTC").empty(), "no path into BTC");
    check(g.bfsPath("INR", "XYZ").empty(), "unknown node gives empty path");
    check(g.bfsPath("USD", "USD").size() == 1, "path from a node to itself is just that node");
}

void testGraphArbitrage() {
    cout << "\n=== GRAPH: arbitrage detection ===" << endl;

    // Case 1: arbitrage exists (USD -> EUR -> INR -> USD multiplies to 1.0125)
    Graph a;
    a.addEdge("USD", "EUR", 0.9);
    a.addEdge("EUR", "INR", 90.0);
    a.addEdge("INR", "USD", 0.0125);
    a.addEdge("USD", "INR", 80.0);
    a.addEdge("INR", "EUR", 0.011);
    a.addEdge("EUR", "USD", 1.1);
    double profit = 0;
    vector<string> cyc = a.findArbitrage(&profit);
    printList("arbitrage cycle", cyc);
    cout << "profit factor: " << profit << endl;
    check(!cyc.empty(), "arbitrage found");
    check(!cyc.empty() && cyc.front() == cyc.back(), "cycle starts and ends at the same node");
    check(profit > 1.0, "profit factor > 1");

    // Case 2: consistent rates, no arbitrage
    Graph b;
    b.addPair("USD", "EUR", 0.9);
    b.addPair("EUR", "INR", 90.0);
    b.addPair("INR", "USD", 0.0125 * 0.0 + 1.0 / (0.9 * 90.0));
    check(b.findArbitrage().empty(), "no arbitrage when rates are consistent");

    // Case 3: empty graph
    Graph c;
    check(c.findArbitrage().empty(), "empty graph has no arbitrage");
}

int main() {
    testTrie();
    testGraphBfs();
    testGraphArbitrage();
    cout << "\n" << (failures == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED") << " (" << failures << " failures)" << endl;
    return failures == 0 ? 0 : 1;
}
