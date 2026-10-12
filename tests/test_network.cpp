// Simple tests with no framework: each check() prints PASS or FAIL.
// The program exits with code 1 if anything failed.

#include <cstdio>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../src/Network.h"

int failures = 0;

void check(bool condition, const std::string& name) {
    std::cout << (condition ? "  PASS  " : "  FAIL  ") << name << "\n";
    if (!condition) ++failures;
}

// Builds the same network as examples/network.txt.
Network exampleNetwork() {
    Network n;
    n.addLink("R1", "R2", 2);
    n.addLink("R1", "R3", 5);
    n.addLink("R2", "R3", 1);
    n.addLink("R2", "R4", 7);
    n.addLink("R3", "R5", 3);
    n.addLink("R4", "R5", 1);
    n.addLink("R4", "R6", 2);
    n.addLink("R5", "R6", 6);
    return n;
}

// Finds the route to `dest` in a routing table (cost -1 if missing).
Route findRoute(const std::vector<Route>& table, const std::string& dest) {
    for (const Route& r : table) {
        if (r.destination == dest) return r;
    }
    return {dest, "", -1};
}

void testShortestPaths() {
    std::cout << "Shortest paths\n";
    Network n = exampleNetwork();
    std::vector<Route> table = n.routingTable("R1");

    check(table.size() == 5, "R1 has a route to all 5 other routers");
    // Direct link R1-R3 costs 5, but R1-R2-R3 costs only 2 + 1 = 3.
    check(findRoute(table, "R3").cost == 3, "R1 -> R3 costs 3 (via R2, not the direct link)");
    check(findRoute(table, "R3").nextHop == "R2", "R1 -> R3 next hop is R2");
    check(findRoute(table, "R6").cost == 9, "R1 -> R6 costs 9");
    check(n.shortestPath("R1", "R6") == std::vector<std::string>{"R1", "R2", "R3", "R5", "R4", "R6"},
          "R1 -> R6 path is R1 R2 R3 R5 R4 R6");
    check(n.shortestPath("R1", "R1") == std::vector<std::string>{"R1"}, "path to yourself is just yourself");
}

void testLinkFailure() {
    std::cout << "Link failure and recovery\n";
    Network n = exampleNetwork();

    check(n.failLink("R3", "R5"), "failing an existing link succeeds");
    check(findRoute(n.routingTable("R1"), "R6").cost == 11, "after R3-R5 fails, R1 -> R6 costs 11");
    check(n.shortestPath("R1", "R6") == std::vector<std::string>{"R1", "R2", "R4", "R6"},
          "after R3-R5 fails, traffic reroutes via R2 R4");

    check(!n.failLink("R3", "R5"), "failing an already-down link returns false");
    check(!n.failLink("R1", "R6"), "failing a link that never existed returns false");

    check(n.restoreLink("R5", "R3"), "restore works with the names in either order");
    check(findRoute(n.routingTable("R1"), "R6").cost == 9, "after restore, R1 -> R6 costs 9 again");
    check(!n.restoreLink("R3", "R5"), "restoring a link that is up returns false");
}

void testUnreachable() {
    std::cout << "Unreachable routers\n";
    Network n = exampleNetwork();
    n.failLink("R1", "R2");
    n.failLink("R1", "R3");

    check(n.hasRouter("R1"), "R1 still exists after losing all its links");
    check(n.routingTable("R1").empty(), "isolated R1 has an empty routing table");
    check(n.shortestPath("R1", "R6").empty(), "no path from isolated R1");
    check(findRoute(n.routingTable("R6"), "R1").cost == -1, "other routers have no route to R1");
}

void testLoadFromFile() {
    std::cout << "Loading topology files\n";
    {
        std::ofstream f("test_good.txt");
        f << "# comment\n\nA B 1\nB C 2\n";
    }
    Network n = Network::loadFromFile("test_good.txt");
    check(n.routers() == std::vector<std::string>{"A", "B", "C"}, "loads routers A, B, C");
    check(findRoute(n.routingTable("A"), "C").cost == 3, "A -> C costs 3");

    auto errorFor = [](const std::string& content) -> std::string {
        {
            std::ofstream f("test_bad.txt");
            f << content;
        }
        try {
            Network::loadFromFile("test_bad.txt");
        } catch (const std::runtime_error& e) {
            return e.what();
        }
        return "";
    };
    check(errorFor("A B 1\nA B\n").find("line 2") != std::string::npos, "missing cost reports line 2");
    check(errorFor("A B -4\n").find("positive") != std::string::npos, "negative cost is rejected");
    check(errorFor("A A 1\n").find("itself") != std::string::npos, "self-link is rejected");
    check(errorFor("A B 1 extra\n").find("line 1") != std::string::npos, "extra words are rejected");

    bool threw = false;
    try {
        Network::loadFromFile("does_not_exist.txt");
    } catch (const std::runtime_error&) {
        threw = true;
    }
    check(threw, "missing file throws an error");

    std::remove("test_good.txt");
    std::remove("test_bad.txt");
}

int main() {
    testShortestPaths();
    testLinkFailure();
    testUnreachable();
    testLoadFromFile();

    std::cout << "\n" << (failures == 0 ? "All tests passed." : std::to_string(failures) + " test(s) FAILED.")
              << "\n";
    return failures == 0 ? 0 : 1;
}
