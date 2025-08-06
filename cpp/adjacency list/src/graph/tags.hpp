#ifndef GRAPH_TAGS_HPP
#define GRAPH_TAGS_HPP

namespace graph::tags {

// The directed-tag to derive from for undirected graphs.
struct Undirected {};

// The directed-tag to derive from for directed graphs,
// when only out-edges must be available.
struct Directed {};

// The directed tag to derive from for directed graphs that additionally
// provide access to in-edges of vertices.
struct Bidirectional : Directed {};

} // namespace graph::tags

#endif // GRAPH_TAGS_HPP
