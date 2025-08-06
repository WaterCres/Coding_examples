#ifndef GRAPH_ADJACENCY_LIST_HPP
#define GRAPH_ADJACENCY_LIST_HPP

#include "tags.hpp"
#include "traits.hpp"
#include "properties.hpp"

#include <boost/iterator/counting_iterator.hpp>
#include <boost/iterator/filter_iterator.hpp>
#include <boost/iterator/iterator_adaptor.hpp>

#include <cassert>
#include <list>
#include <vector>
#include <concepts>
#include <type_traits>
#include <iostream>
namespace graph {

template<typename DirectedCategoryT,
         typename VertexPropT = NoProp,
         typename EdgePropT = NoProp>
struct AdjacencyList {
private:

	struct StoredEdge {
		// edges in the graph these might have weights or other stuff stored on them
		// init an edge without a property, requires a property to be default constructible
		StoredEdge(std::size_t &src, std::size_t &tar, std::size_t &id) : src(src), tar(tar), id(id), prop() {}
		// init an edge with a property
		StoredEdge(std::size_t &src, std::size_t &tar, EdgePropT &p, std::size_t &id) : src(src), tar(tar), id(id), prop(p) {}

		// source and target of the edge
		std::size_t src, tar;
		// an id to find this one
		std::size_t id;
		// the property to be stored		
		EdgePropT prop;
	};
	
	struct OutEdge {
		// an outgoing edge has a target
		std::size_t tar;
		// and an id matching a stored edge
		std::size_t id;
	};
	struct InEdge {
		// an ingoing edge has a source
		std::size_t src;
		// and an id matching a stored edge
		std::size_t id;
	};
	// use vectors to store edges
	using InEdgeList = std::vector<InEdge>;
	using OutEdgeList = std::vector<OutEdge>;
	
	struct StoredVertexDirected {
		// A vertex in a directed graph
		// do not use the top one for properties which is not default constructible
		StoredVertexDirected() = default;
		StoredVertexDirected(VertexPropT &val): prop(val), eOut() {}
		// some stored property
		VertexPropT prop;
		// a list of outgoing edges
		OutEdgeList eOut;
	};
	struct StoredVertexBid : StoredVertexDirected {
		// a vertex in a bidirectional graph
		// do not use the top one for properties which is not default constructible
		StoredVertexBid() = default;
		StoredVertexBid(VertexPropT &val) : StoredVertexDirected(val), eIn() {}
		// in addition to the previous it also has incomming edges
		InEdgeList eIn;
	};
	// make a decision at compile time what kind of vertex this is supposed to be
	constexpr static bool isBi = std::derived_from<DirectedCategoryT, graph::tags::Bidirectional>;
	// apply that decision
	using StoredVertex = std::conditional_t<isBi , StoredVertexBid, StoredVertexDirected>;
	// vertex lists are actually vectors
	using VList = std::vector<StoredVertex>;
	using EList = std::vector<StoredEdge>;
public: // Graph
	using DirectedCategory = DirectedCategoryT;
	using VertexDescriptor = std::size_t;

	struct EdgeDescriptor {
		// an edge descriptor
		// source, target and index in its list, what else could one need
		EdgeDescriptor() = default;
		EdgeDescriptor(std::size_t src, std::size_t tar,
		               std::size_t storedEdgeIdx)
			: src(src), tar(tar), storedEdgeIdx(storedEdgeIdx) {}
	public:
		std::size_t src, tar;
		std::size_t storedEdgeIdx;
	public:
		// compare edgedescriptors
		friend bool operator==(const EdgeDescriptor &a,
		                       const EdgeDescriptor &b) {
			return a.storedEdgeIdx == b.storedEdgeIdx;
		}
	};
public: // PropertyGraph
	// property graphs might have to store something on those edges and vertices
	using VertexProp = VertexPropT;
	using EdgeProp = EdgePropT;
public: // VertexListGraph
	struct VertexRange {
		// the iterator is simply a counter that returns its value when
		// dereferenced
		using iterator = boost::counting_iterator<VertexDescriptor>;
	public:
		VertexRange(std::size_t n) : n(n) {}
		iterator begin() const { return iterator(0); }
		iterator end()   const { return iterator(n); }
	private:
		std::size_t n;
	};
public: // EdgeListGraph
	struct EdgeRange {
		// We want to adapt the edge list,
		// so it dereferences to EdgeDescriptor instead of StoredEdge
		using EListIterator = typename EList::const_iterator;

		struct iterator : boost::iterator_adaptor<
				iterator, // because we use CRTP
				EListIterator, // the iterator we adapt
				// we want to convert the StoredEdge into an EdgeDescriptor:
				EdgeDescriptor,
				// we can use RA as the underlying iterator supports it:
				std::random_access_iterator_tag,
				// when we dereference we return by value, not by reference
				EdgeDescriptor
			> {
			using Base = boost::iterator_adaptor<
				iterator, EListIterator, EdgeDescriptor,
				std::random_access_iterator_tag, EdgeDescriptor>;
		public:
			iterator() = default;
			iterator(EListIterator i, EListIterator first) : Base(i), first(first) {}
		private:
			// let the Boost machinery use our methods:
			friend class boost::iterator_core_access;

			EdgeDescriptor dereference() const {
				// get our current position stored in the
				// boost::iterator_adaptor base class
				const EListIterator &i = this->base_reference();
				return EdgeDescriptor{i->src, i->tar,
					static_cast<std::size_t>(i->id)};
			}
		private:
			EListIterator first;
		};
	public:
		EdgeRange(const AdjacencyList &g) : g(&g) {}

		iterator begin() const {
			return iterator(g->eList.begin(), g->eList.begin());
		}

		iterator end() const {
			return iterator(g->eList.end(), g->eList.begin());
		}
	private:
		const AdjacencyList *g;
	};
public: // IncidenceGraph
	struct OutEdgeRange {
		// looks like an edge range but the edgelist
		// is restricted to the eOut of a vertex
		using EListIterator = typename OutEdgeList::const_iterator;
		struct iterator : boost::iterator_adaptor<
				iterator,
				EListIterator,
				EdgeDescriptor,
				std::bidirectional_iterator_tag,
				EdgeDescriptor
			>{
				using Base = boost::iterator_adaptor<
					iterator, EListIterator, EdgeDescriptor,
					std::bidirectional_iterator_tag, EdgeDescriptor>;

			public:
				iterator() = default;
				iterator(EListIterator i, EListIterator first, VertexDescriptor v) : Base(i), first(first), ref(v) {}

			private:
				friend class boost::iterator_core_access;
				
				EdgeDescriptor dereference() const {
					// dereffing an out edge should give a description of the original edge
					const EListIterator &i = this->base_reference();
					return EdgeDescriptor{ref, i->tar,
						static_cast<std::size_t>(i->id)};
				}
			private:
				EListIterator first;
				VertexDescriptor ref;
			};// OutEdgerange::iterator
		
		public:
		// get a range of outgoing edges
		OutEdgeRange( const VertexDescriptor v, const AdjacencyList &g) : 
							v(v),
							g(&g) {}
		// startig here
		iterator begin() const { 
			return iterator(g->vList[v].eOut.begin(), 
							g->vList[v].eOut.begin(),
							v); 
		}
		// ending here
		iterator end() const {
			return iterator(g->vList[v].eOut.end(),
							g->vList[v].eOut.begin(),
							v);
		}

		private:
			const VertexDescriptor v;
			const AdjacencyList *g;
	}; // OutEdgeRange

public: // BidirectionalGraph
	struct InEdgeRange {
		// looks like an out edge range but the edgelist
		// is restricted to the eIn of a vertex
		using EListIterator = typename InEdgeList::const_iterator;
		struct iterator : boost::iterator_adaptor<
				iterator,
				EListIterator,
				EdgeDescriptor,
				std::bidirectional_iterator_tag,
				EdgeDescriptor
			>{
				using Base = boost::iterator_adaptor<
					iterator, EListIterator, EdgeDescriptor,
					std::bidirectional_iterator_tag, EdgeDescriptor>;

			public:
				iterator() = default;
				iterator(EListIterator i, EListIterator first, VertexDescriptor v) : Base(i), first(first), ref(v) {}

			private:
				friend class boost::iterator_core_access;
				
				EdgeDescriptor dereference() const {
					// dereffing an in edge should give a description of the original edge
					const EListIterator &i = this->base_reference();
					return EdgeDescriptor{ref, i->src,
						static_cast<std::size_t>(i->id)};
				};
			private:
				EListIterator first;
				VertexDescriptor ref;
			};// OutEdgerange::iterator
		
		public:
		// get a range of ingoing edges
		InEdgeRange( const VertexDescriptor v, const AdjacencyList &g) : 
							v(v),
							g(&g) {}
		// start here
		iterator begin() const { 
			return iterator(g->vList[v].eIn.begin(), 
							g->vList[v].eIn.begin(),
							v); 
		}
		// end here
		iterator end() const {
			return iterator(g->vList[v].eIn.end(),
							g->vList[v].eIn.begin(),
							v);
		};

		private:
			const VertexDescriptor v;
			const AdjacencyList *g;

	}; // InEdgeRange

public:
	// construct an adjacency list of size 0 or n
	AdjacencyList() = default;
	AdjacencyList(std::size_t n) : vList(n) {}
private:
	VList vList;
	EList eList;
public: // Graph
	friend VertexDescriptor source(EdgeDescriptor e, const AdjacencyList&) {
		// get the source of an edge
		return e.src;
	}

	friend VertexDescriptor target(EdgeDescriptor e, const AdjacencyList&) {
		// get the target of an edge
		return e.tar;
	}
public: // VertexListGraph
	friend std::size_t numVertices(const AdjacencyList &g) {
		// get the number of vertices in the graph
		return g.vList.size();
	}

	friend VertexRange vertices(const AdjacencyList &g) {
		// get a range of all vertices
		return VertexRange(numVertices(g));
	}
public: // EdgeListGraph
	friend std::size_t numEdges(const AdjacencyList &g) {
		// get the number of edges in the graph
		return g.eList.size();
	}

	friend EdgeRange edges(const AdjacencyList &g) {
		// get a range of all edges
		return EdgeRange(g);
	}

public: // Other
	friend std::size_t getIndex(VertexDescriptor v, const AdjacencyList &) {
		// get the index of a vertex
		return v;
	}
	
public: // IncidenceGraph
	
	friend OutEdgeRange outEdges(const VertexDescriptor &v, const AdjacencyList &g) {
		// get a range of edges originating from this vertex
		return OutEdgeRange(v,g);
	}

	friend std::size_t outDegree(const VertexDescriptor &v, const AdjacencyList &g) {
		// get the out degree of this vertex
		return g.vList[v].eOut.size();
	}

public: // BidirectionalGraph
	
	friend InEdgeRange inEdges(const VertexDescriptor &v, const AdjacencyList &g) 
	requires(isBi)
	{
		// get a range of edges going into vertex v, the graph must be at least bidirectional
		return InEdgeRange(v,g);
	}

	friend std::size_t inDegree(const VertexDescriptor &v, const AdjacencyList &g)
	requires (isBi)
	{
		// get the in degree of a vertex, the graph must be atleast bidirectional
		return g.vList[v].eIn.size();
	}
	friend EdgeDescriptor addEdge(VertexDescriptor &u, VertexDescriptor &v, AdjacencyList &g) 
	requires (std::is_default_constructible<EdgeProp>::value && isBi)
	{
		// add an edge in a graph which is atleast bidirectional,
		// and has default constructible edge properties
		size_t id = numEdges(g);
		g.eList.push_back(StoredEdge(u,v,id));
		g.vList[u].eOut.push_back(OutEdge(v,id));
		g.vList[v].eIn.push_back(InEdge(u,id));
		return EdgeDescriptor(u,v,id);
	}

public: // MutableGraph
	friend VertexDescriptor addVertex(AdjacencyList &g) 
	requires (std::is_default_constructible<VertexProp>::value)
	{
		// add a vertex to g if vertex properties are default constructible
		g.vList.push_back(StoredVertex());
		// zero indexed
		return numVertices(g)-1;
	}

	friend EdgeDescriptor addEdge(VertexDescriptor &u, VertexDescriptor &v, AdjacencyList &g)
	requires (std::is_default_constructible<EdgeProp>::value && !isBi)
	{
		// add an edge between(from) u and(to) v
		// if edge properties are default constructible
		// and the graph is (un)directed
		size_t id = numEdges(g);
		g.eList.push_back(StoredEdge(u,v, id));
		g.vList[u].eOut.push_back(OutEdge(v, id));
		return EdgeDescriptor(u,v,id);
	}

public: // MutablePropertyGraph
	friend VertexDescriptor addVertex(VertexProp &vp, AdjacencyList &g) 
	requires (std::copyable<VertexPropT>) 
	{
		// add a vertex with a property
		g.vList.push_back(StoredVertex(vp));
		// zero indexed
		return numVertices(g)-1;
	}

	friend VertexDescriptor addVertex(VertexProp &&vp, AdjacencyList &g) 
	requires (std::movable<VertexPropT>) 
	{
		// add a non-copyable vertex with a property
		g.vList.push_back(StoredVertex(std::move(vp)));
		// zero indexed
		return numVertices(g)-1;
	}

	friend EdgeDescriptor addEdge( VertexDescriptor &u, VertexDescriptor &v, EdgePropT &ep, AdjacencyList &g)
	requires (std::copyable<EdgePropT> && !isBi) 
	{
		// add an edge in a (un)directed graph
		// this edge has some property
		size_t id = numEdges(g);
		g.eList.push_back(StoredEdge(u,v,ep,id));
		g.vList[u].eOut.push_back(OutEdge(v,id));
		return EdgeDescriptor(u,v,id);
	}

	friend EdgeDescriptor addEdge( VertexDescriptor &u, VertexDescriptor &v, EdgePropT &ep, AdjacencyList &g)
	requires (std::copyable<EdgePropT> && isBi)
	{
		// add an edge in a bidirectional graph
		// this edge has some property
		size_t id = numEdges(g);
		g.eList.push_back(StoredEdge(u,v,ep,id));
		g.vList[u].eOut.push_back(OutEdge(v,id));
		g.vList[v].eIn.push_back(InEdge(u,id));
		return EdgeDescriptor(u,v,id);
	}
	
	friend EdgeDescriptor addEdge( VertexDescriptor &u, VertexDescriptor &v, EdgePropT &&ep, AdjacencyList &g)
	requires (std::movable<EdgePropT> && !isBi)
	{
		// move a property onto a new edge in a (un)directed graph
		size_t id = numEdges(g);
		g.eList.push_back(StoredEdge(u,v,std::move(ep),id));
		g.vList[u].eOut.push_back(OutEdge(v,id));
		g.vList[v].eIn.push_back(InEdge(u,id));
		return EdgeDescriptor(u,v,id);
	}

	friend EdgeDescriptor addEdge( VertexDescriptor &u, VertexDescriptor &v, EdgePropT &&ep, AdjacencyList &g)
	requires (std::movable<EdgePropT> && isBi)
	{
		// move a property onto a new edge in a bidirectional graph
		size_t id = numEdges(g);
		g.eList.push_back(StoredEdge(u,v,std::move(ep),id));
		g.vList[u].eOut.push_back(OutEdge(v,id));
		g.vList[v].eIn.push_back(InEdge(u,id));
		return EdgeDescriptor(u,v,id);
	}

public: // PropertyGraph
	VertexProp &operator[] (const VertexDescriptor idx) {
		// return a ref to the property of a given vertex
		return this->vList[idx].prop;
	}
	
	const VertexProp &operator[] (const VertexDescriptor idx) const {
		// return a const ref to the property of a given const vertex in a const way i guess
		return const_cast<VertexProp*>(this[idx]);
	}

	EdgeProp &operator[] (const EdgeDescriptor e) {
		// return a ref to the property of a given edge
		size_t idx = e.storedEdgeIdx;
		return this->eList[idx].prop;
	}

	const EdgeProp &operator[] (const EdgeDescriptor e) const {
		// return a const ref to the property of a given const edge in a const way i guess
		return const_cast<EdgeProp*>(this[e]);
	}
};
} // namespace graph

#endif // GRAPH_ADJACENCY_LIST_HPP
