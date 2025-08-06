#ifndef GRAPH_TOPOLOGICAL_SORT_HPP
#define GRAPH_TOPOLOGICAL_SORT_HPP

#include "depth_first_search.hpp"

namespace graph {
namespace detail {

template<typename OIter>
struct TopoVisitor : DFSNullVisitor {
	TopoVisitor(OIter iter) : iter(iter) {}
		
	template<typename G, typename V>
	void finishVertex(const V &v, const G &g) {
		// when a vertex has been explored add it to the container
		// insert 1 element of type vertex into position
		// the way vertex descriptors are made this is sufficient, no real need to get the index
		// might as well do it however, since someone else might have other descriptors
		std::fill_n(iter,1,getIndex(v,g));
		// go to next position
		iter++;
	}
private:
	OIter iter;
};

} // namespace detail

template<typename Graph, typename OutputIterator>
void topoSort(const Graph &g, OutputIterator oIter) 
requires (graph::Godfs<Graph>)
{
	// use dfs to visit vertices and sort in reverse topological order
	dfs(g, detail::TopoVisitor(oIter));
}

} // namespace graph

#endif // GRAPH_TOPOLOGICAL_SORT_HPP
