#ifndef SIMPLE_ROUTE_GRAPH_H
#define SIMPLE_ROUTE_GRAPH_H

#include <memgraph/Graph.h>

namespace simpleroute {

struct Graph: memgraph::Graph {
	Graph() {}
	Graph(const memgraph::Graph & g) : memgraph::Graph(g) {}
	Graph(const Graph & g) : memgraph::Graph(g) {}
	Graph(memgraph::Graph && g) : memgraph::Graph(std::move(g)) {}
	Graph(Graph && g) : memgraph::Graph(std::move(g)) {}
	
	Graph & operator=(const Graph & g) {
		memgraph::Graph::operator=(g);
		return *this;
	}
	Graph & operator=(const memgraph::Graph & g) {
		memgraph::Graph::operator=(g);
		return *this;
	}
	Graph & operator=(Graph && g) {
		memgraph::Graph::operator=(std::move(g));
		return *this;
	}
	Graph & operator=(memgraph::Graph && g) {
		memgraph::Graph::operator=(std::move(g));
		return *this;
	}
	virtual ~Graph() {}
	
	static Graph fromPBF(const std::string & path, bool spatialSort, int accessTypes = Edge::AT_ALL);
};
	
}

#endif
