#ifndef SIMPLEROUTE_CHCONSTRUCTOR_H
#define SIMPLEROUTE_CHCONSTRUCTOR_H
#include "Graph.h"
#include <algorithm>
#include <unordered_set>

namespace simpleroute {
namespace detail {

template<typename T_EDGE_WEIGHT>
class CHGraph {
public:
	typedef T_EDGE_WEIGHT EdgeWeight;
	struct Edge {
		static constexpr uint32_t is_shortcut_marker = 0xFFFFFFFF;
		uint32_t baseEdgeId;
		uint32_t target;
		EdgeWeight edgeWeight;
		bool isShortcunt() const { return baseEdgeId == is_shortcut_marker; }
	};
	struct Node {
		uint32_t baseNodeId;
		uint32_t incomingEdgesBegin;
		uint32_t outgoingEdgesBegin;
	};
	
	typedef std::vector<Edge> EdgesContainer;
	typedef std::vector<Node> NodesContainer;
	typedef EdgesContainer::const_iterator const_edge_iterator;
	
	
public:
	CHGraph(Graph * g);
	virtual ~CHGraph() {}
	Node const & node(uint32_t id) const { return m_nodes.at(id); }
	NodesContainer const & nodes() const { return m_nodes; }
	NodesContainer & nodes() { return m_nodes; }
	
	Edge const & incomingEdge(uint32_t id) const {
		return incomingEdges().at(id);
	}
	const_edge_iterator incomingEdgesBegin(uint32_t nodeId) const {
		return incomingEdges().begin() + node(nodeId).incomingEdgesBegin;
	}
	const_edge_iterator incomingEdgesEnd(uint32_t nodeId) const {
		return (nodeId+1 < m_nodes.size() ? incomingEdgesBegin(nodeId+1) : m_incomingEdges.end());
	}
	EdgesContainer const & incomingEdges() const {
		return m_incomingEdges;
	}
	EdgesContainer & incomingEdges() const {
		return m_incomingEdges;
	}
	
	Edge const & outgoingEdge(uint32_t id) const {
		return m_outgoingEdges.at(id);
	}
	const_edge_iterator outgoingEdgesBegin(uint32_t nodeId) const {
		return outgoingEdges().begin() + node(nodeId).outgoingEdgesBegin;
	}
	const_edge_iterator outgoingEdgesEnd(uint32_t nodeId) const {
		return (nodeId+1 < m_nodes.size() ? outgoingEdgesBegin(nodeId+1): outgoingEdges().end());
	}
	EdgesContainer const & outgoingEdges() const {
		return m_outgoingEdges;
	}
	EdgesContainer & outgoingEdges() const {
		return m_outgoingEdges;
	}
	
	void contract(const std::unordered_set<uint32_t> & contractNodes, std::vector< std::pair<uint32_t, Edge> > & shortcuts);
	
private:
	EdgesContainer m_incomingEdges;
	EdgesContainer m_outgoingEdges;
	NodesContainer m_nodes;
	Graph * m_g;
};

CHGraph::CHGraph(Graph* g) :
m_g(g)
{
	m_nodes.reserve(m_g->nodeCount());
	m_outgoingEdges.reserve(m_g->edgeCount());
	Node n;
	Edge e;
	for(uint32_t nodeId(0), s(m_g->nodeCount()); i < s; ++i) {
		n.baseId = nodeId;
		n.incomingEdgesBegin = 0;
		n.outgoingEdgesBegin = m_outgoingEdges.size();
		m_nodes.emplace_back(n);
		for(auto it(m_g->edgesBegin(nodeId)), its(m_g->edgesEnd(nodeId)); it != its; ++it) {
			e.
			m_outgoingEdges.emplace_back(e);
		}
	}
	
}


void CHGraph::contract(const std::unordered_set< uint32_t >& contractNodes, std::vector< std::pair< uint32_t, Edge > > & shortcuts) {

}


}

class CHContstructor {
public:
	static constexpr uint32_t initial_node_level = 0xFFFFFFFF;
public:
	CHContstructor();
	virtual CHContstructor();
	
protected:
	template<typename T_WEIGHT_PRED>
	void getIndependentSet(T_WEIGHT_PRED wp);
protected:
	Graph * m_g;
	CHInfo * m_ch;
};

/**
  * struct Traits {
  *   typedef <> Weight;
  *   struct Weighter {
  *     Weight operator()(Graph * g, CHInfo * ch, uint32_t nodeId);
  *   };
  *   struct WeightCompareLess {
  *     bool operator(const Weight & a, const Weight & b) const;
  *   };
  *   Weighter weighter();
  *   WeightCompareLess weightCompareLess();
  * }
  */
template<typename T_TRAITS, typename T_OUTPUT_ITERATOR>
void CHContstructor::getIndependentSet(T_TRAITS traits, T_OUTPUT_ITERATOR out) {
	typedef T_TRAITS Traits;
	typedef typename Traits::Weight Weight;
	typedef typename Traits::Weighter Weighter;
	typedef typename Traits::WeighterCompareLess WeighterCompareLess;
	
	Weighter weighter( traits.weighter() );
	WeighterCompareLess weightCompareLess( traits.weightCompareLess() );
	
	std::vector<std::pair<Weight, uint32_t> > nodeWeights;
	
	for(uint32_t nodeId(0), s(m_g->nodeCount()); nodeId < s; ++nodeId) {
		if (m_ch->node(nodeId).level == initial_node_level) {
			nodeWeights.emplace_back(weighter(m_g, m_ch, nodeId), nodeId);
		}
	}
	
	using std::sort;
	sort(nodeWeights.begin(), nodeWeights.end(), weightCompareLess);
	
	std::unordered_set<uint32_t> removedNodes;
	for(auto it(nodeWeights.begin()), s(nodeWeights.end()); it != s; ++it) {
		if (removedNodes.count(it->second)) { //neighbor already picked
			continue;
		}
		Graph::Node n(m_g->node(it->second));
		//first the outgoing edges
		for(auto eIt(m_g->edgesBegin(it->second)), eS(m_g->edgesEnd(it->second)); eIt != eS; ++eIt) {
			const Graph::Edge & e = *eIt;
			if (m_ch->node(e.target).level == initial_node_level) {
				
			}
		}
	}
}

}//end namespace

#endif