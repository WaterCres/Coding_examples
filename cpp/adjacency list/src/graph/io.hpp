#ifndef GRAPH_IO_HPP
#define GRAPH_IO_HPP

#include "traits.hpp"

#include <iostream>

namespace graph {

// Parse a textual description of a graph and construct a `Graph` from it.
// The graph constructor will be called with an integer representing the number
// of vertices.
// The textual format is the so-called DIMACS format:
//
// - The first line has the form ``p edge <n> <m>`` where ``<n>`` and
//   ``<m>`` are integers denoting the number of vertices and edges
//   respectively of the graph.
// - The following ``<m>`` lines have the form ``e <src> <tar>``, where
//   ``<src>`` and ``<tar>`` are positive integers from 1 through ``<n>``,
//   denoting respectively the source and target of an edge.
template<typename Graph>
Graph loadDimacs(std::istream &s) {
	auto error = [](auto &&msg) {
		throw std::runtime_error(std::string("Parsing error: ") + msg);
	};
	char cmd;
	if(!(s >> cmd) || cmd != 'p') error("Expected 'p'.");
	std::string edgeKeyword;
	if(!(s >> edgeKeyword) || edgeKeyword != "edge") error("Expected 'edge'.");
	std::size_t n;
	if(!(s >> n)) error("Expected number of vertices.");
	std::size_t m;
	if(!(s >> m)) error("Expected number of edges.");

	Graph g(n);

	for(std::size_t i = 1; i <= m; ++i) {
		if(!(s >> cmd) || cmd != 'e') error("Expected 'e' for edge " + std::to_string(i) + ".");
		std::size_t src, tar;
		if(!(s >> src >> tar)) error("Expected source and target for edge " + std::to_string(i) + ".");
		if(src == 0 || src > n) error("Source " + std::to_string(src) + " for edge " + std::to_string(i) + " is out of bounds.");
		if(tar == 0 || tar > n) error("Target " + std::to_string(tar) + " for edge " + std::to_string(i) + " is out of bounds.");
		addEdge(src - 1, tar - 1, g);
	}
	return g;
}

// Print the given graph to the given output stream in the DOT format,
// http://www.graphviz.org.
// The given `VertexPrinter` and an `EdgePrinter` will be invoked inside the
// square brackets of each vertex and edge while printing. They will be called
// as `p(s, ve, g)` where `p` is the printer, `s` is the given output stream,
// `ve` is a vertex or edge descriptor, and `g` is the given graph.
template<typename Graph, typename VertexPrinter, typename EdgePrinter>
std::ostream &printDot(std::ostream &s, const Graph &g,
                       VertexPrinter vPrinter, EdgePrinter ePrinter) {
	s << "digraph g {\n";
	for(auto v : vertices(g)) {
		s << getIndex(v, g) << " [";
		vPrinter(s, v, g);
		s << "];\n";
	}
	for(auto e : edges(g)) {
		s << getIndex(source(e, g), g) << " -> " << getIndex(target(e, g), g);
		s << " [";
		ePrinter(s, e, g);
		s << "];\n";
	}
	s << "}\n";
	return s;
}

// See above, but without printing any additional text for vertices and edges.
template<typename Graph>
std::ostream &printDot(std::ostream &s, const Graph &g) {
	return printDot(s, g, [](auto&&...) {}, [](auto&&...) {});
}

} // namespace graph

#endif // GRAPH_IO_HPP
