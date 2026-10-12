// Link-state routing simulator.
//
// Usage:
//   router_sim <topology file>                  type commands interactively
//   router_sim <topology file> <commands file>  run commands from a file

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "Network.h"

void printHelp() {
    std::cout << "Commands:\n"
                 "  table <router>         show one router's routing table\n"
                 "  tables                 show every router's routing table\n"
                 "  path <from> <to>       show the hop-by-hop best path\n"
                 "  fail <router> <router>     take a link down\n"
                 "  restore <router> <router>  bring a failed link back up\n"
                 "  help                   show this list\n"
                 "  quit                   exit\n";
}

void printTable(const Network& network, const std::string& router) {
    std::cout << "Routing table for " << router << "\n";
    std::cout << "  " << std::left << std::setw(14) << "Destination" << std::setw(10) << "Next hop"
              << "Cost\n";
    for (const Route& route : network.routingTable(router)) {
        std::cout << "  " << std::setw(14) << route.destination << std::setw(10) << route.nextHop
                  << route.cost << "\n";
    }
}

void printPath(const Network& network, const std::string& from, const std::string& to) {
    std::vector<std::string> path = network.shortestPath(from, to);
    if (path.empty()) {
        std::cout << to << " is unreachable from " << from << "\n";
        return;
    }
    // The total cost is already in the source router's routing table.
    int cost = 0;
    for (const Route& route : network.routingTable(from)) {
        if (route.destination == to) cost = route.cost;
    }
    std::cout << "Path " << from << " -> " << to << ": ";
    for (size_t i = 0; i < path.size(); ++i) {
        std::cout << (i > 0 ? " -> " : "") << path[i];
    }
    std::cout << "  (cost " << cost << ")\n";
}

// Runs one command. Returns false when the user wants to quit.
bool runCommand(Network& network, const std::string& line) {
    std::istringstream words(line);
    std::string command, a, b;
    words >> command >> a >> b;

    if (command.empty() || command[0] == '#') {
        return true;  // blank line or comment
    }
    if (command == "quit" || command == "exit") {
        return false;
    }
    if (command == "help") {
        printHelp();
        return true;
    }
    if (command == "tables") {
        for (const std::string& router : network.routers()) {
            printTable(network, router);
            std::cout << "\n";
        }
        return true;
    }

    // "table" takes one router name; "path", "fail" and "restore" take two.
    int namesNeeded = 0;
    if (command == "table") {
        namesNeeded = 1;
    } else if (command == "path" || command == "fail" || command == "restore") {
        namesNeeded = 2;
    } else {
        std::cout << "Unknown command '" << command << "'. Type 'help' for commands.\n";
        return true;
    }

    if (a.empty() || (namesNeeded == 2 && b.empty())) {
        std::cout << "'" << command << "' needs " << namesNeeded << " router name(s). Type 'help'.\n";
        return true;
    }
    for (const std::string& name : {a, b}) {
        if (!name.empty() && !network.hasRouter(name)) {
            std::cout << "Unknown router '" << name << "'.\n";
            return true;
        }
    }

    if (command == "table") {
        printTable(network, a);
    } else if (command == "path") {
        printPath(network, a, b);
    } else if (command == "fail") {
        std::cout << (network.failLink(a, b) ? "Link " + a + "-" + b + " is DOWN. Routes recomputed.\n"
                                             : "There is no active link between " + a + " and " + b + ".\n");
    } else {
        std::cout << (network.restoreLink(a, b) ? "Link " + a + "-" + b + " is UP. Routes recomputed.\n"
                                                : "Link " + a + "-" + b + " was not down.\n");
    }
    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: router_sim <topology file> [commands file]\n";
        return 1;
    }

    Network network;
    try {
        network = Network::loadFromFile(argv[1]);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    std::cout << "Loaded " << network.routers().size() << " routers from " << argv[1] << "\n";

    if (argc == 3) {
        // Script mode: echo each command so the output reads like a transcript.
        std::ifstream script(argv[2]);
        if (!script) {
            std::cerr << "Error: cannot open file '" << argv[2] << "'\n";
            return 1;
        }
        std::string line;
        while (std::getline(script, line)) {
            if (line.empty() || line[0] == '#') continue;
            std::cout << "\n> " << line << "\n";
            if (!runCommand(network, line)) break;
        }
        return 0;
    }

    // Interactive mode.
    printHelp();
    std::string line;
    while (std::cout << "\n> " && std::getline(std::cin, line)) {
        if (!runCommand(network, line)) break;
    }
    return 0;
}
