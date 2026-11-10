#ifndef GRAPH_H
#define GRAPH_H

#include <string>
#include <vector>
#include <map>
#include <queue>
#include <cmath>
#include <algorithm>
using namespace std;

// Currency / asset network for the exchange.
// Nodes are currencies (USD, EUR, INR ...). A directed edge A -> B with weight
// 'rate' means 1 unit of A converts into 'rate' units of B.
// Standalone: no dependency on OrderBook.

class Graph {
private:
    struct Edge {
        int from;
        int to;
        double rate;
    };

    map<string, int> idOf;
    vector<string> names;
    vector<vector<pair<int, double>>> adj; // adjacency list: (neighbour, rate)
    vector<Edge> edges;                    // flat edge list for Bellman-Ford

    int getId(const string& name) {
        auto it = idOf.find(name);
        if (it != idOf.end()) return it->second;
        int id = names.size();
        idOf[name] = id;
        names.push_back(name);
        adj.push_back({});
        return id;
    }

    // Best (highest) direct rate from a to b, or 0 if no edge.
    double directRate(int a, int b) const {
        double best = 0.0;
        for (size_t i = 0; i < adj[a].size(); i++) {
            if (adj[a][i].first == b) best = max(best, adj[a][i].second);
        }
        return best;
    }

public:
    // O(1). Rate must be > 0. Adds a directed edge from -> to.
    bool addEdge(const string& from, const string& to, double rate) {
        if (rate <= 0) return false;
        int u = getId(from);
        int v = getId(to);
        adj[u].push_back({v, rate});
        edges.push_back({u, v, rate});
        return true;
    }

    // Convenience: adds from->to at 'rate' and to->from at 1/rate.
    bool addPair(const string& a, const string& b, double rate) {
        if (rate <= 0) return false;
        addEdge(a, b, rate);
        addEdge(b, a, 1.0 / rate);
        return true;
    }

    bool hasNode(const string& name) const { return idOf.count(name) > 0; }
    int nodeCount() const { return names.size(); }
    int edgeCount() const { return edges.size(); }

    // BFS: path with the fewest hops from src to dst. O(V + E).
    // Returns an empty vector if there is no path.
    vector<string> bfsPath(const string& src, const string& dst) const {
        vector<string> path;
        auto s = idOf.find(src);
        auto d = idOf.find(dst);
        if (s == idOf.end() || d == idOf.end()) return path;

        int n = names.size();
        vector<int> parent(n, -1);
        vector<bool> visited(n, false);
        queue<int> q;
        q.push(s->second);
        visited[s->second] = true;

        while (!q.empty()) {
            int u = q.front();
            q.pop();
            if (u == d->second) break;
            for (size_t i = 0; i < adj[u].size(); i++) {
                int v = adj[u][i].first;
                if (!visited[v]) {
                    visited[v] = true;
                    parent[v] = u;
                    q.push(v);
                }
            }
        }

        if (!visited[d->second]) return path;
        for (int cur = d->second; cur != -1; cur = parent[cur]) {
            path.push_back(names[cur]);
        }
        reverse(path.begin(), path.end());
        return path;
    }

    // Multiplies the best direct rates along 'path'. Returns 0 if any hop has no edge.
    double pathRate(const vector<string>& path) const {
        if (path.size() < 2) return path.empty() ? 0.0 : 1.0;
        double total = 1.0;
        for (size_t i = 0; i + 1 < path.size(); i++) {
            auto a = idOf.find(path[i]);
            auto b = idOf.find(path[i + 1]);
            if (a == idOf.end() || b == idOf.end()) return 0.0;
            double r = directRate(a->second, b->second);
            if (r == 0.0) return 0.0;
            total *= r;
        }
        return total;
    }

    // Arbitrage detection with Bellman-Ford on weights w = -log(rate).
    // A cycle whose rates multiply to > 1 is a negative-weight cycle.
    // Time O(V * E). Returns the cycle as a node list whose first and last
    // entries are the same (e.g. USD EUR INR USD). Empty if no arbitrage.
    // If profitFactor is given, it receives the product of rates around the
    // cycle (> 1 means profit).
    vector<string> findArbitrage(double* profitFactor = nullptr) const {
        int n = names.size();
        vector<string> result;
        if (n == 0) return result;

        // Starting every distance at 0 acts like a virtual source connected to all nodes,
        // so cycles anywhere in the graph are found, not only ones reachable from one node.
        vector<double> dist(n, 0.0);
        vector<int> parent(n, -1);
        int x = -1;

        for (int pass = 0; pass < n; pass++) {
            x = -1;
            for (size_t i = 0; i < edges.size(); i++) {
                double w = -log(edges[i].rate);
                if (dist[edges[i].from] + w < dist[edges[i].to] - 1e-12) {
                    dist[edges[i].to] = dist[edges[i].from] + w;
                    parent[edges[i].to] = edges[i].from;
                    x = edges[i].to;
                }
            }
            if (x == -1) return result; // converged early: no negative cycle
        }

        // Relaxation still happened on pass V, so x is on or leads back to a negative cycle.
        // Step back V times to land inside the cycle itself.
        for (int i = 0; i < n; i++) x = parent[x];

        vector<int> cyc;
        int cur = x;
        do {
            cyc.push_back(cur);
            cur = parent[cur];
        } while (cur != x);
        cyc.push_back(x);
        reverse(cyc.begin(), cyc.end());

        double product = 1.0;
        for (size_t i = 0; i < cyc.size(); i++) {
            result.push_back(names[cyc[i]]);
            if (i + 1 < cyc.size()) product *= directRate(cyc[i], cyc[i + 1]);
        }
        if (profitFactor != nullptr) *profitFactor = product;
        return result;
    }
};

#endif
