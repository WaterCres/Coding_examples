#ifndef GRAPH_ADJACENCY_MATRIX_HPP
#define GRAPH_ADJACENCY_MATRIX_HPP

#include "tags.hpp"
#include "traits.hpp"
#include "properties.hpp"

#include <boost/iterator/counting_iterator.hpp>
#include <boost/iterator/filter_iterator.hpp>
#include <boost/iterator/iterator_adaptor.hpp>

#include <cassert>
#include <tuple>
#include <vector>

namespace graph {

struct AdjacencyMatrix {
private:
	struct StoredEdge {
		bool exists = false;
	};
	using Matrix = std::vector<StoredEdge>;
	using MatrixIterator = typename Matrix::const_iterator;
public: // Graph
	using VertexDescriptor = std::size_t;

	struct EdgeDescriptor {
		std::size_t src, tar;
		bool exists; // so we can represent both edges and non-edges
	public:
		friend bool operator==(const EdgeDescriptor &a, const EdgeDescriptor &b) {
			return std::tie(a.src, a.tar, a.exists)
				== std::tie(b.src, b.tar, b.exists);
		}
	};

	using DirectedCategory = tags::Directed;
public: // VertexList
	struct VertexRange {
		// the iterator is simply a counter that returns its value when dereferenced
		using iterator = boost::counting_iterator<VertexDescriptor>;
	public:
		VertexRange(std::size_t n) : n(n) {}
		iterator begin() const { return iterator(0); }
		iterator end()   const { return iterator(n); }
	private:
		std::size_t n;
	};
public: // EdgeList
	struct EdgeRange {
		// In the iterator, we want first adapt the MatrixIterator
		// (which iterates through StoredEdges) to give EdgeDescirptors,
		// and then skip those that don't exist in the graph.
		// First: adapt the iterator for the matrix (which is a vector::iterator).
		struct MatrixAdaptorIterator : boost::iterator_adaptor<
				MatrixAdaptorIterator, // because we use CRTP
				MatrixIterator, // the iterator we adapt
				// we want to convert the StoredEdge into an EdgeDescriptor:
				EdgeDescriptor,
				// we can use RA as the underlying iterator supports it:
				std::random_access_iterator_tag,
				// when we dereference we return by value, not by reference:
				EdgeDescriptor 
		> {
			using Base = boost::iterator_adaptor<
				MatrixAdaptorIterator, MatrixIterator, EdgeDescriptor,
				std::random_access_iterator_tag, EdgeDescriptor>;
		public:
			MatrixAdaptorIterator(const MatrixIterator &i,
			                      const MatrixIterator &first,
			                      std::size_t n)
				: Base(i), first(first), n(n) {}
		private:
			// let the Boost machinery use our methods
			friend class boost::iterator_core_access;

			EdgeDescriptor dereference() const {
				// get our current position stored in the
				// boost::iterator_adaptor base class
				const MatrixIterator &i = this->base_reference();
				const auto idx = i - first;
				// then calculate the row/src and column/tar
				const auto src = idx / n;
				const auto tar = idx % n;
				return EdgeDescriptor{src, tar, i->exists};
			}
		public:
			MatrixIterator first;
			std::size_t n;
		};

		// Now wrap the matrix iterator such that entries
		// without an edge are skipped.
		struct EdgeExistPred {
			bool operator()(const EdgeDescriptor &e) const {
				return e.exists;
			}
		};
		using iterator = boost::filter_iterator<EdgeExistPred, MatrixAdaptorIterator>;
	public:
		EdgeRange(const AdjacencyMatrix *g) : g(g) { }

		iterator begin() const {
			return iterator(
				MatrixAdaptorIterator(g->matrix.begin(), g->matrix.begin(), g->n),
				MatrixAdaptorIterator(g->matrix.end(), g->matrix.begin(), g->n)
			);
		}

		iterator end() const {
			return iterator(
				MatrixAdaptorIterator(g->matrix.end(), g->matrix.begin(), g->n),
				MatrixAdaptorIterator(g->matrix.end(), g->matrix.begin(), g->n)
			);
		}
	private:
		const AdjacencyMatrix *g;
	};
public: // Incidence
	struct OutEdgeRange {
		// we can reuse the EdgeRange::iterator
		// as the out-edges are simply a sub-range of the edges.
		// For example, in the following adj. matrix (. means no edge, e means edge)
		// stored in row-major in the underlying vector, then a row is the out-edges
		// for that vertex, which is a subrange of the underlying vector
		//   0 1 2 3 4 ... n-1
		// 0 . e . e e          entires  0 through  n-1 of the vector
		// 1 e . e e e          entries  n through 2n-1 of the vector
		// 2 . e . . .          entries 2n through 3n-1 of the vector
		// ...
		using iterator = typename EdgeRange::iterator;
	public:
		OutEdgeRange(VertexDescriptor v, const AdjacencyMatrix &g) : src(v), g(&g) { }

		iterator begin() const {
			// src is the row number, each row has n entries
			auto first = g->matrix.begin() + src * g->n;
			auto last = g->matrix.begin() + (src + 1) * g->n;
			return iterator(
				typename EdgeRange::MatrixAdaptorIterator(
					first, g->matrix.begin(), g->n),
				typename EdgeRange::MatrixAdaptorIterator(
					last,  g->matrix.begin(), g->n)
			);
		}	
		
		iterator end() const {
			// src is the row number, each row has n entries
			auto last = g->matrix.begin() + (src + 1) * g->n;
			return iterator(
				typename EdgeRange::MatrixAdaptorIterator(
					last, g->matrix.begin(), g->n),
				typename EdgeRange::MatrixAdaptorIterator(
					last, g->matrix.begin(), g->n)
			);
		}
	private:
		std::size_t src;
		const AdjacencyMatrix *g;
	};	
public:
	AdjacencyMatrix(std::size_t n) : n(n), matrix(n * n) {}
private:
	std::size_t n;
	std::size_t m = 0;
	Matrix matrix;
public: // Graph
	friend VertexDescriptor source(const EdgeDescriptor &e,
	                               const AdjacencyMatrix&) {
		return e.src;
	}

	friend VertexDescriptor target(const EdgeDescriptor &e,
	                               const AdjacencyMatrix&) {
		return e.tar;
	}
public: // VertexList
	friend std::size_t numVertices(const AdjacencyMatrix &g) {
		return g.n;
	}

	friend VertexRange vertices(const AdjacencyMatrix &g) {
		return VertexRange(g.n);
	}
public: // EdgeList
	friend std::size_t numEdges(const AdjacencyMatrix &g) {
		return g.m;
	}

	friend EdgeRange edges(const AdjacencyMatrix &g) {
		return EdgeRange(&g);
	}
public: // Incidence
	friend std::size_t outDegree(VertexDescriptor v, const AdjacencyMatrix &g) {
		auto oe = outEdges(v, g);
		return std::distance(oe.begin(), oe.end());
	}

	friend OutEdgeRange outEdges(VertexDescriptor v, const AdjacencyMatrix &g) {
		return OutEdgeRange(v, g);
	}
public: // Mutable
	friend EdgeDescriptor addEdge(VertexDescriptor src, VertexDescriptor tar,
	                              AdjacencyMatrix &g) {
		if(g.matrix[src * g.n + tar].exists) assert(false);
		++g.m;
		g.matrix[src * g.n + tar].exists = true;
		return EdgeDescriptor{src, tar, true};
	}
public: // Other
	friend std::size_t getIndex(VertexDescriptor v, const AdjacencyMatrix&) {
		return v;
	}
};

} // namespace graph

#endif // GRAPH_ADJACENCY_MATRIX_HPP
