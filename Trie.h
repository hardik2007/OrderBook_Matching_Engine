#ifndef TRIE_H
#define TRIE_H

#include <string>
#include <vector>
#include <map>
#include <cctype>
using namespace std;

// Ticker search module for the exchange.
// Standalone: no dependency on OrderBook. Each ticker can carry an orderBookId
// so it can be linked to an OrderBook during integration.

class TrieNode {
public:
    map<char, TrieNode*> children; // ordered map => autocomplete returns alphabetical order
    bool isEnd;
    int orderBookId;               

    TrieNode() {
        isEnd = false;
        orderBookId = -1;
    }
};

class Trie {
private:
    TrieNode* root;

    static string normalize(const string& s) {
        string r = s;
        for (size_t i = 0; i < r.size(); i++) {
            r[i] = toupper((unsigned char)r[i]);
        }
        return r;
    }

    void destroy(TrieNode* node) {
        if (node == nullptr) return;
        for (auto& kv : node->children) {
            destroy(kv.second);
        }
        delete node;
    }

    TrieNode* findNode(const string& s) const {
        TrieNode* cur = root;
        for (size_t i = 0; i < s.size(); i++) {
            auto it = cur->children.find(s[i]);
            if (it == cur->children.end()) return nullptr;
            cur = it->second;
        }
        return cur;
    }

    void collect(TrieNode* node, string& buffer, vector<string>& out, int limit) const {
        if (node == nullptr) return;
        if (limit > 0 && (int)out.size() >= limit) return;
        if (node->isEnd) out.push_back(buffer);
        for (auto& kv : node->children) {
            buffer.push_back(kv.first);
            collect(kv.second, buffer, out, limit);
            buffer.pop_back();
            if (limit > 0 && (int)out.size() >= limit) return;
        }
    }

    // Returns true if 'node' should be deleted by its parent.
    bool removeHelper(TrieNode* node, const string& s, size_t depth, bool& removed) {
        if (depth == s.size()) {
            if (!node->isEnd) return false;
            node->isEnd = false;
            node->orderBookId = -1;
            removed = true;
            return node->children.empty();
        }
        auto it = node->children.find(s[depth]);
        if (it == node->children.end()) return false;
        if (removeHelper(it->second, s, depth + 1, removed)) {
            delete it->second;
            node->children.erase(it);
            return !node->isEnd && node->children.empty();
        }
        return false;
    }

public:
    Trie() { root = new TrieNode(); }
    ~Trie() { destroy(root); }

    Trie(const Trie&) = delete;
    Trie& operator=(const Trie&) = delete;

    // O(L). Re-inserting an existing ticker just updates its orderBookId.
    void insert(const string& ticker, int orderBookId = -1) {
        string s = normalize(ticker);
        TrieNode* cur = root;
        for (size_t i = 0; i < s.size(); i++) {
            auto it = cur->children.find(s[i]);
            if (it == cur->children.end()) {
                it = cur->children.insert({s[i], new TrieNode()}).first;
            }
            cur = it->second;
        }
        cur->isEnd = true;
        cur->orderBookId = orderBookId;
    }

    // O(L). True only if this exact ticker was inserted.
    bool search(const string& ticker) const {
        TrieNode* node = findNode(normalize(ticker));
        return node != nullptr && node->isEnd;
    }

    // O(L). Returns -1 if the ticker is not present or has no id.
    int getOrderBookId(const string& ticker) const {
        TrieNode* node = findNode(normalize(ticker));
        if (node == nullptr || !node->isEnd) return -1;
        return node->orderBookId;
    }

    bool startsWith(const string& prefix) const {
        return findNode(normalize(prefix)) != nullptr;
    }

    // O(L + output). limit <= 0 means no limit. Results are alphabetical.
    vector<string> autocomplete(const string& prefix, int limit = 0) const {
        vector<string> out;
        string s = normalize(prefix);
        TrieNode* node = findNode(s);
        if (node == nullptr) return out;
        string buffer = s;
        collect(node, buffer, out, limit);
        return out;
    }

    // O(L). Removes the ticker and prunes nodes that are no longer needed.
    bool remove(const string& ticker) {
        string s = normalize(ticker);
        bool removed = false;
        removeHelper(root, s, 0, removed);
        return removed;
    }
};

#endif
