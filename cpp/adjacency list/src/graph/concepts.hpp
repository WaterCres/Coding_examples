#ifndef GRAPH_CONCEPTS_HPP
#define GRAPH_CONCEPTS_HPP

#include "tags.hpp"
#include "traits.hpp"

#include <concepts>
#include <iterator>

namespace graph {

template<typename R, typename E>
concept Range = requires(R r) {
	typename R::iterator;
	// One could argue it should be std::forward_iterator, but
	// there is a quirk with the boost::filter_iterator that makes
	// it an input iterator.
	requires std::input_iterator<typename R::iterator>;
	requires std::same_as<E,
		typename std::iterator_traits<typename R::iterator>::value_type>;
	{ r.begin() } -> std::same_as<typename R::iterator>;
	{ r.end() } -> std::same_as<typename R::iterator>;
};

template<typename G>
concept Graph = requires {
	// A light-weight object that represents a vertex.
	typename Traits<G>::VertexDescriptor;
	requires std::regular<typename Traits<G>::VertexDescriptor>;

	// A light-weigh object that represents an edge.
	typename Traits<G>::EdgeDescriptor;
	requires std::regular<typename Traits<G>::EdgeDescriptor>;

	// A tag type that indicates whether the graph is undirected or directed,
	// and if directed, whether only out-edges or both out- and in-edges are avilable.
	typename Traits<G>::DirectedCategory;
	requires
	   std::derived_from<typename Traits<G>::DirectedCategory, tags::Undirected>
	|| std::derived_from<typename Traits<G>::DirectedCategory, tags::Directed>;
} && requires(const G &g, typename Traits<G>::EdgeDescriptor e) {
	{ source(e, g) } -> std::same_as<typename Traits<G>::VertexDescriptor>;
	{ target(e, g) } -> std::same_as<typename Traits<G>::VertexDescriptor>;
	// If two edge descriptors compare equal they must,
	// - for DirectedCategory being convertible to tags::Undirected,
	//   have either the same source and target, or swapped source and target.
	// - for DirectedCategory being convertible to tags::Directed,
	//   have the same source and target respectively.
};

template<typename G>
concept VertexListGraph =
	Graph<G>
&& requires(const G &g) {
	typename Traits<G>::VertexRange;
	requires Range<typename Traits<G>::VertexRange,
				   typename Traits<G>::VertexDescriptor>;
	// Returns a range of the vertices in the graph.
	{ vertices(g) } -> std::same_as<typename Traits<G>::VertexRange>;
	// Returns the number of vertices in the graph.
	{ numVertices(g) } -> std::integral;
};

template<typename G>
concept EdgeListGraph =
	Graph<G>
&& requires(const G &g) {
	typename Traits<G>::EdgeRange;
	requires Range<typename Traits<G>::EdgeRange,
				   typename Traits<G>::EdgeDescriptor>;
	// Returns a range of the edges in the graph.
	{ edges(g) } -> std::same_as<typename Traits<G>::EdgeRange>;
	// Returns the number of edges in the graph.
	{ numEdges(g) } -> std::integral;
};

template<typename G>
concept IncidenceGraph =
	Graph<G>
&& requires(const G &g) {
	typename Traits<G>::OutEdgeRange;
	requires Range<typename Traits<G>::OutEdgeRange,
				   typename Traits<G>::EdgeDescriptor>;
} && requires(const G &g, typename Traits<G>::VertexDescriptor v) {
	// - for DirectedCategory being convertible to tags::Undirected,
	//   returns a range of the out-edges of v in g.
	// - for DirectedCategory being convertible to tags::Directed,
	//   returns a range of the incident edges of v in g.
	{ outEdges(v, g) } -> std::same_as<typename Traits<G>::OutEdgeRange>;
	// - for DirectedCategory being convertible to tags::Undirected,
	//   returns the number of the out-edges of v in g.
	// - for DirectedCategory being convertible to tags::Directed,
	//   returns the number of incident edges of v in g.
	{ outDegree(v, g) } -> std::integral;
};

template<typename G>
concept BidirectionalGraph =
	IncidenceGraph<G>
&& requires(const G &g) {
	typename Traits<G>::InEdgeRange;
	requires Range<typename Traits<G>::InEdgeRange,
				   typename Traits<G>::EdgeDescriptor>;
} && requires(const G &g, typename Traits<G>::VertexDescriptor v) {
	// - for DirectedCategory being convertible to tags::Undirected,
	//   returns a range of the in-edges of v in g.
	// - for DirectedCategory being convertible to tags::Directed,
	//   returns a range of the incident edges of v in g.
	{ inEdges(v, g) } -> std::same_as<typename Traits<G>::InEdgeRange>;
	// - for DirectedCategory being convertible to tags::Undirected,
	//   returns the number of the in-edges of v in g.
	// - for DirectedCategory being convertible to tags::Directed,
	//   returns the number of incident edges of v in g.
	{ inDegree(v, g) } -> std::integral;
};

template<typename G>
concept MutableGraph =
	Graph<G>
&& requires(G &g, typename Traits<G>::VertexDescriptor u,
                  typename Traits<G>::VertexDescriptor v) {
	// Adds a vertex to g.
	{ addVertex(g) } -> std::same_as<typename Traits<G>::VertexDescriptor>;
	// Adds an edge to g between vertices u and v.
	{ addEdge(u, v, g) } -> std::same_as<typename Traits<G>::EdgeDescriptor>;
};

template<typename G>
concept PropertyGraph =
	Graph<G>
&& requires(const G &g, typename Traits<G>::VertexDescriptor v,
                        typename Traits<G>::EdgeDescriptor e) {
	// The type of data associated with each vertex in the graph.
	typename Traits<G>::VertexProp;
	// The type of data associated with each edge in the graph.
	typename Traits<G>::EdgeProp;
	// Access the associated data of a vertex, read-only.
	{ g[v] } -> std::same_as<const typename Traits<G>::VertexProp&>;
	// Access the associated data of an edg, read-only..
	{ g[e] } -> std::same_as<const typename Traits<G>::EdgeProp&>;
} && requires(G &g, typename Traits<G>::VertexDescriptor v,
                    typename Traits<G>::EdgeDescriptor e) {
	// Access the associated data of a vertex, read/write.
	{ g[v] } -> std::same_as<typename Traits<G>::VertexProp&>;
	// Access the associated data of an edge, read/write.
	{ g[e] } -> std::same_as<typename Traits<G>::EdgeProp&>;
};

template<typename G>
concept MutablePropertyGraph =
	MutableGraph<G>
&&	PropertyGraph<G>
&& requires(G &g, typename Traits<G>::VertexDescriptor u,
                  typename Traits<G>::VertexDescriptor v,
            typename Traits<G>::VertexProp vp,
            typename Traits<G>::EdgeProp ep) {
	// Adds a vertex to g, with property vp.
	{ addVertex(std::move(vp), g) }
		-> std::same_as<typename Traits<G>::VertexDescriptor>;
	// Adds an edge to g between vertices u and v, with property ep.
	{ addEdge(u, v, std::move(ep), g) }
		-> std::same_as<typename Traits<G>::EdgeDescriptor>;
};

template<typename G>
// a graph should match at least these concepts
// in order to be dfs'ed
concept Godfs = 
	graph::IncidenceGraph<G>
&&  graph::EdgeListGraph<G>
&&  graph::VertexListGraph<G>;


} // namespace graph

#endif // GRAPH_CONCEPTS_HPP
