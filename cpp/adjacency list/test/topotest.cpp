#include <cassert>
#include <iostream>
#include <vector>
#include <graph/adjacency_matrix.hpp>
#include <graph/topological_sort.hpp>
#include <array>
#include <graph/io.hpp>
#include <sstream>
#include <algorithm>

std::string getDefaultGraph() {
    return
    "p edge 10 14\n"
    "e 1 3\n"
    "e 1 4\n"
    "\n"
    "e 2 5\n"
    "e 2 6\n"
    "\n"
    "e 3 7\n"
    "e 3 10\n"
    "\n"
    "e 4 8\n"
    "e 4 9\n"
    "\n"
    "e 5 7\n"
    "e 5 9\n"
    "\n"
    "e 6 8\n"
    "e 6 10\n"
    "\n"
    "e 7 9\n"
    "\n"
    "e 8 10\n";
}

template<typename Graph>
Graph loadGraph() {
	std::istringstream ss(getDefaultGraph());
	return graph::loadDimacs<Graph>(ss);
}

int main() {
    using Graph = graph::AdjacencyMatrix;
    Graph g = loadGraph<Graph>();
    // vector
    std::vector<typename graph::Traits<Graph>::VertexDescriptor> ves;
	ves.reserve(numVertices(g));
	graph::topoSort(g, std::back_inserter(ves));
	std::reverse(ves.begin(), ves.end());
	for(const auto &v: ves)
		std::cout << getIndex(v, g) << ", ";
	std::cout << std::endl;
    // array
    std::array<typename graph::Traits<Graph>::VertexDescriptor, 10> vs;
    vs.fill(42);
    graph::topoSort(g, vs.begin());
    std::reverse(vs.begin(), vs.end());
    for(const auto &v: vs)
        std::cout << getIndex(v, g) << ", ";
    std::cout << std::endl;
}