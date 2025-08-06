#include <graph/adjacency_list.hpp>
#include <graph/concepts.hpp>
#include <iostream>
#include <utility>
#include <cassert>
#include <typeinfo>
#include <concepts>
#include <vector>

using namespace graph;
using tag = tags::Bidirectional;
// using tag = tags::Directed;

struct nondef {

        nondef(int a) : val(a) {}
        int val;

        nondef(const nondef &old) :val(old.val) {}

        nondef(nondef &&old) :val(std::exchange(old.val,0)) {}

        ~nondef() {}

        nondef &operator=(const nondef &old) {
            val = old.val;
            return *this;
        }

        nondef &operator=( nondef &&old) {
            if (this != &old) {
                val = std::exchange(old.val, 0);
            }
            return *this;
        }

        nondef operator++(int i) {
            this->val += i;
            return *this;
        }

        nondef &operator++() {
            this->val++;
            return *this;
        }

        friend bool operator==(nondef a, nondef b) {
            return a.val == b.val;
        }

        friend bool operator!=(nondef a, nondef b) {
            return !(a == b);
        }

        int operator*() const {
            return val;
        }


    };

int main()
{
    /**
     * type combos to test
     * noprop,noprop
     * nondef,noprop
     * noprop,nondef
     * nondef,nondef
     * int,noprop
     * noprop,int
     * int,int
     * int,nondef
     * nondef,int
    */

    using edgt = int;
    using vert = int;
    
    enum struct DFSColour {
	    White, Grey, Black
    };



    std::vector<DFSColour> vec(7, DFSColour::White);

    vec[3] = DFSColour::Grey;

    for (auto d : vec) {
        switch(d)
        {
            case DFSColour::White  : std::cout << "White\n";   break;
            case DFSColour::Grey : std::cout << "Grey\n"; break;
            case DFSColour::Black : std::cout << "Black\n";  break;
        }
    }

    AdjacencyList<tag, vert, edgt> al;

    // add two vertices and an edge between them
    // nondef i(12);
    // nondef e(42);

    vert i = 12;
    edgt e = 42;

    size_t u = addVertex(i,al);
    i++;
    size_t v = addVertex(i,al);
    // these fail with nondef but work with int
    // size_t x = addVertex(al);
    // size_t z = addVertex(al);
    AdjacencyList<tag, vert, edgt>::EdgeDescriptor d = addEdge(u,v,e,al);
    AdjacencyList<tag, vert, edgt>::VertexDescriptor f(1);

    // out edges
    for (auto v: vertices(al)) {
        if (f != v){
            e++;
            addEdge(f,v,e,al);
            addEdge(v,f,e,al);
        }
        std::cout << v<< std::endl;
    }

    // nondef t = al[f];
    // test return of operator[]
    // for vertex
    // const nondef &t1 = al[f];
    // nondef &t2 = al[f];

    using G = graph::AdjacencyList<graph::tags::Bidirectional, vert, edgt>;
    static_assert(graph::MutablePropertyGraph<G>);

    const vert &t1 = al[f];
    vert &t2 = al[f];

    static_assert(std::is_same<decltype(t1),const vert&>::value);
    static_assert(std::is_same<decltype(t2),vert&>::value);
    // for edge
    const edgt &t3 = al[d];
    edgt &t4 = al[d];
    static_assert(std::is_same<decltype(t3),const edgt&>::value);
    static_assert(std::is_same<decltype(t4),edgt&>::value);


    std::cout << "vertex 1: " << al[f] << std::endl;
    al[f] = 42;
    std::cout << "vertex 1: " << al[f] << std::endl;
    std::cout << "edge 1: " << al[d] << std::endl;
    std::cout << "inedges from 1:" << std::endl << inDegree(f,al) << std::endl;
    for (auto e : inEdges(f,al))
    {
        std::cout << e.src << ", "<< e.tar << "," << e.storedEdgeIdx << std::endl;
    }
    

    std::cout << "outedges from 1:" << std::endl << outDegree(f,al) << std::endl;
    
    for (auto e: outEdges(f,al))
        std::cout << e.src << ", "<< e.tar << "," << e.storedEdgeIdx << std::endl;

    std::cout << "all edges:" << std::endl;

    for (auto e : edges(al))
        std::cout << e.src << ", "<< e.tar << "," << e.storedEdgeIdx << std::endl;


    return 0;
}
