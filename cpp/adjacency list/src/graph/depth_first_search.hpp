#ifndef GRAPH_DEPTH_FIRST_SEARCH_HPP
#define GRAPH_DEPTH_FIRST_SEARCH_HPP

#include "traits.hpp"
#include "concepts.hpp"

#include <vector>
#include <concepts>


namespace graph {

struct DFSNullVisitor {
	template<typename G, typename V>
	void initVertex(const V&, const G&) { }	

	template<typename G, typename V>
	void startVertex(const V&, const G&) { }	

	template<typename G, typename V>
	void discoverVertex(const V&, const G&) { }	

	template<typename G, typename V>
	void finishVertex(const V&, const G&) { }	

	template<typename G, typename E>
	void examineEdge(const E&, const G&) { }	

	template<typename G, typename E>
	void treeEdge(const E&, const G&) { }	

	template<typename G, typename E>
	void backEdge(const E&, const G&) { }	

	template<typename G, typename E>
	void forwardOrCrossEdge(const E&, const G&) { }	

	template<typename G, typename E>
	void finishEdge(const E&, const G&) { }	
};


namespace detail {

enum struct DFSColour {
	White, Grey, Black
};

template<typename Graph, typename Visitor>
void dfsVisit(const Graph &g, Visitor &visitor, typename Traits<Graph>::VertexDescriptor u,
		std::vector<DFSColour> &colour) 
		requires (graph::Godfs<Graph>) // ensure the graph fulfills the required concept
		{
			visitor.discoverVertex(u, g);
			// set colour grey(being visited)
			colour[getIndex(u,g)] = DFSColour::Grey;
			// for each adjacent vertex
			for (auto e : outEdges(u, g)){
				visitor.examineEdge(e, g);
				switch (colour[getIndex(target(e,g),g)])
				{
				case DFSColour::White :
					// explore the edge
					visitor.treeEdge(e, g);
					dfsVisit(g, visitor, target(e,g), colour);
					break;

				case DFSColour::Grey :
					// being visited
					visitor.backEdge(e, g);
					break;

				case DFSColour::Black : 
					// already explored
					visitor.forwardOrCrossEdge(e, g);
					break;
				}
				visitor.finishEdge(e, g);
			}
			// set colour black(visited)
			colour[getIndex(u,g)] = DFSColour::Black;
			visitor.finishVertex(u, g);
		}

} // namespace detail

template<typename Graph, typename Visitor>
void dfs(const Graph &g, Visitor visitor) 
	requires (Godfs<Graph>) // ensure the graph fulfills the required concept
{
	// Initialise colour
	std::vector<detail::DFSColour> colours(numVertices(g), detail::DFSColour::White);
	// and init the vertices
	for (auto v : vertices(g)) {
		visitor.initVertex(v,g);
	}
	// start visiting
	for (auto v : vertices(g)) {
		if (colours[getIndex(v, g)] == detail::DFSColour::White)
		{
			visitor.startVertex(v, g);
			detail::dfsVisit<Graph, Visitor>(g, visitor, v, colours);
		}
	}
}

} // namespace graph

#endif // GRAPH_DEPTH_FIRST_SEARCH_HPP
