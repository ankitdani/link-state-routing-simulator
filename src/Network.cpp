#include "Network.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <queue>
#include <sstream>
#include <stdexcept>

namespace {

// Puts two router names in a fixed order, so link "R2 R1" and "R1 R2"
// are recognized as the same link.
std::pair<std::string, std::string> linkKey(const std::string& a, const std::string& b) {
    return a < b ? std::make_pair(a, b) : std::make_pair(b, a);
}

}  // namespace

void Network::addLink(const std::string& a, const std::string& b, int cost) {
    links_[a][b] = cost;
    links_[b][a] = cost;
}

bool Network::failLink(const std::string& a, const std::string& b) {
    if (!hasRouter(a) || links_[a].count(b) == 0) {
        return false;
    }
    failedLinks_[linkKey(a, b)] = links_[a][b];
    // Remove the link in both directions, but keep both routers in the
    // network, even if they now have no links at all.
    links_[a].erase(b);
    links_[b].erase(a);
    return true;
}

bool Network::restoreLink(const std::string& a, const std::string& b) {
    auto it = failedLinks_.find(linkKey(a, b));
    if (it == failedLinks_.end()) {
        return false;
    }
    addLink(a, b, it->second);
    failedLinks_.erase(it);
    return true;
}

bool Network::hasRouter(const std::string& name) const {
    return links_.count(name) > 0;
}

std::vector<std::string> Network::routers() const {
    std::vector<std::string> names;
    for (const auto& entry : links_) {
        names.push_back(entry.first);
    }
    return names;  // already sorted, because std::map keeps keys in order
}

// Dijkstra's algorithm: finds the cheapest path from `source` to every router.
//
// Idea: always expand the closest router we haven't finished yet. Because all
// link costs are positive, once a router is the closest one in the queue, no
// later path can reach it more cheaply, so its distance is final.
Network::ShortestPaths Network::dijkstra(const std::string& source) const {
    ShortestPaths result;

    // Each queue entry is (distance so far, router name). std::greater turns
    // the default max-heap into a min-heap, so top() is the closest router.
    using Entry = std::pair<int, std::string>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> queue;

    result.distance[source] = 0;
    queue.push({0, source});

    while (!queue.empty()) {
        Entry closest = queue.top();
        queue.pop();
        int distSoFar = closest.first;
        const std::string& router = closest.second;

        // The same router can be in the queue more than once. If we already
        // found a cheaper way to it, this entry is out of date, so skip it.
        if (distSoFar > result.distance[router]) {
            continue;
        }

        // "Relax" each link: does going through `router` give a neighbor a cheaper path?
        for (const auto& link : links_.at(router)) {
            const std::string& neighbor = link.first;
            int newDist = distSoFar + link.second;

            bool neverSeen = result.distance.count(neighbor) == 0;
            if (neverSeen || newDist < result.distance[neighbor]) {
                result.distance[neighbor] = newDist;
                result.previous[neighbor] = router;
                queue.push({newDist, neighbor});
            }
        }
    }
    return result;
}

std::vector<Route> Network::routingTable(const std::string& source) const {
    std::vector<Route> table;
    if (!hasRouter(source)) {
        return table;
    }

    ShortestPaths paths = dijkstra(source);
    for (const auto& entry : paths.distance) {
        const std::string& dest = entry.first;
        if (dest == source) {
            continue;
        }
        // Walk backwards from the destination until we reach the router
        // right after the source. That router is the next hop.
        std::string hop = dest;
        while (paths.previous.at(hop) != source) {
            hop = paths.previous.at(hop);
        }
        table.push_back({dest, hop, entry.second});
    }
    return table;
}

std::vector<std::string> Network::shortestPath(const std::string& source, const std::string& dest) const {
    if (!hasRouter(source) || !hasRouter(dest)) {
        return {};
    }
    ShortestPaths paths = dijkstra(source);
    if (paths.distance.count(dest) == 0) {
        return {};  // unreachable
    }

    // Follow the "previous" links back from dest to source, then reverse.
    std::vector<std::string> path;
    for (std::string at = dest; at != source; at = paths.previous.at(at)) {
        path.push_back(at);
    }
    path.push_back(source);
    std::reverse(path.begin(), path.end());
    return path;
}

Network Network::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        throw std::runtime_error("cannot open file '" + filename + "'");
    }

    Network network;
    std::string line;
    int lineNumber = 0;
    while (std::getline(file, line)) {
        ++lineNumber;
        std::istringstream words(line);
        std::string a, b;
        int cost = 0;

        if (!(words >> a) || a[0] == '#') {
            continue;  // blank line or comment
        }
        std::string extra;
        if (!(words >> b >> cost) || (words >> extra)) {
            throw std::runtime_error(filename + " line " + std::to_string(lineNumber) +
                                     ": expected '<router> <router> <cost>'");
        }
        if (cost <= 0) {
            throw std::runtime_error(filename + " line " + std::to_string(lineNumber) +
                                     ": cost must be a positive number");
        }
        if (a == b) {
            throw std::runtime_error(filename + " line " + std::to_string(lineNumber) +
                                     ": a router can't link to itself");
        }
        network.addLink(a, b, cost);
    }
    return network;
}
