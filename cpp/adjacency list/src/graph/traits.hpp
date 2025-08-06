#ifndef GRAPH_TRAITS_HPP
#define GRAPH_TRAITS_HPP

#include <type_traits>

// This file is based on boost::graph_traits

namespace graph {
namespace detail {

#define GRAPH_TRAITS_DEF_MEMBER_OR_VOID(Name) \
	template<typename, typename = void> \
	struct TraitsGetMember_ ## Name { \
		using type = void; \
	}; \
	\
	template<typename T> \
	struct TraitsGetMember_ ## Name <T, std::void_t<typename T::Name>> { \
		using type = typename T::Name; \
	}; \
	\
	template<typename G> \
	struct TraitsMember_ ## Name { \
		using type = typename detail::TraitsGetMember_ ## Name <G>::type; \
	}
GRAPH_TRAITS_DEF_MEMBER_OR_VOID(VertexRange);
GRAPH_TRAITS_DEF_MEMBER_OR_VOID(EdgeRange);
GRAPH_TRAITS_DEF_MEMBER_OR_VOID(OutEdgeRange);
GRAPH_TRAITS_DEF_MEMBER_OR_VOID(InEdgeRange);
GRAPH_TRAITS_DEF_MEMBER_OR_VOID(VertexProp);
GRAPH_TRAITS_DEF_MEMBER_OR_VOID(EdgeProp);

#define GRAPH_TRAITS_RANGE(Name) \
	using Name ## Range = typename detail::TraitsMember_ ## Name ## Range <G>::type;
	
} // namespace detail


// Access to associated types for a graph `G`.
// Specialise this template if the graph type does not provide the types as
// nested types.
template<typename G>
struct Traits {
public: // Graph
	using VertexDescriptor = typename G::VertexDescriptor;
	using EdgeDescriptor = typename G::EdgeDescriptor;
	using DirectedCategory = typename G::DirectedCategory;
public: // VertexListGraph
	GRAPH_TRAITS_RANGE(Vertex);
public: // EdgeListGraph
	GRAPH_TRAITS_RANGE(Edge);
public: // IncidenceGraph
	GRAPH_TRAITS_RANGE(OutEdge);
public: // BidirectionalGraph
	GRAPH_TRAITS_RANGE(InEdge);
public: // PropertyGraph
	using VertexProp = typename detail::TraitsMember_VertexProp<G>::type;
	using EdgeProp = typename detail::TraitsMember_EdgeProp<G>::type;
};

} // namespace graph

#endif // GRAPH_TRAITS_HPP
