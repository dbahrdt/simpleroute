#ifndef SIMPLE_ROUTE_CH_GRAPH_H
#define SIMPLE_ROUTE_CH_GRAPH_H
#include <vector>
#include <limits>
#include <stdint.h>
#include <string>
#include "Graph.h"

namespace simpleroute {

class CHConstructor;

class CHInfo {
private:
	friend class CHConstructor;
public:
	typedef uint32_t WeightType;

	struct Node {
		uint32_t level;
		uint32_t shortCutsBegin;
		uint32_t shortCutsEnd;
	};
	
	struct CHEdge {
		WeightType weight;
	};
public:
	CHInfo();
	~CHInfo();
	const Node & node(uint32_t id) const;
	const WeightType & weight(uint32_t id) const;
	//in the real graph, these point into the other direction
	const Graph::Edge reverseEdge(uint32_t id) const;
	Graph::ConstEdgeIterator reverseEdgesBegin(uint32_t nodeId) const;
	Graph::ConstEdgeIterator reverseEdgesEnd(uint32_t nodeId) const;
	const Graph::Edge shortcut(uint32_t id) const;
private:
	typedef std::vector<Node> NodesContainer;
	typedef std::vector<WeightType> WeightTypeContainer;
	typedef std::vector<Graph::Edge> EdgesContainer;
private:
	NodesContainer m_nodes;
	WeightType m_weights;
	EdgesContainer m_edges;
};

}//end namespace simpleroute

#endif
