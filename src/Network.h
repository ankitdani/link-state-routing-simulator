#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

// One row of a router's routing table.
struct Route {
    std::string destination;  // the router we want to reach
    std::string nextHop;      // the neighbor we hand the packet to first
    int cost;                 // total cost of the whole path
};

// A network of routers connected by links that each have a cost.
// In graph terms: routers are nodes, links are weighted, undirected edges.
class Network {
public:
    // Adds a two-way link between routers a and b.
    void addLink(const std::string& a, const std::string& b, int cost);

    // Takes a link down, like a cut cable. Returns false if the link doesn't exist.
    bool failLink(const std::string& a, const std::string& b);

    // Brings a failed link back up with its original cost. Returns false if it wasn't failed.
    bool restoreLink(const std::string& a, const std::string& b);

    bool hasRouter(const std::string& name) const;
    std::vector<std::string> routers() const;

    // Builds the routing table for `source` using Dijkstra's algorithm.
    std::vector<Route> routingTable(const std::string& source) const;

    // The hop-by-hop path from source to dest, e.g. {"R1", "R2", "R5"}.
    // Empty if dest can't be reached.
    std::vector<std::string> shortestPath(const std::string& source, const std::string& dest) const;

    // Reads a topology file with one link per line: "R1 R2 10".
    // Blank lines and lines starting with '#' are ignored.
    // Throws std::runtime_error (with the line number) on bad input.
    static Network loadFromFile(const std::string& filename);

private:
    // What Dijkstra produces for one source router.
    struct ShortestPaths {
        std::map<std::string, int> distance;          // best known cost to each router
        std::map<std::string, std::string> previous;  // router just before it on the best path
    };
    ShortestPaths dijkstra(const std::string& source) const;

    // links_[a][b] is the cost of the link between a and b.
    // Every link is stored twice (a->b and b->a) because links work both ways.
    std::map<std::string, std::map<std::string, int>> links_;

    // Links that are currently down, so restoreLink() knows their cost.
    // The key is the two router names in alphabetical order.
    std::map<std::pair<std::string, std::string>, int> failedLinks_;
};
