#include "Router.h"
#include "Graph.h"
#include <vector>
#include <unordered_map>
#include <set>
#include <queue>


namespace simpleroute {

bool Router::AccessAllowanceEdgePreferences::accessAllowed(const Graph::Edge& e) const {
	return e.access & accessTypeMask;
}

namespace detail {

struct NodeHopDistInfo {
	uint32_t parentNodeId;
	uint32_t distance;
	NodeHopDistInfo() : parentNodeId(0xFFFFFFFF), distance(0xFFFFFFFF) {}
	NodeHopDistInfo(uint32_t parentNodeId, uint32_t distance) : parentNodeId(parentNodeId), distance(distance) {}
};

HopDistanceRouter::HopDistanceRouter(const Graph* g) :
Router(g),
m_ep(0)
{}

HopDistanceRouter::~HopDistanceRouter() {
	delete m_ep;
}

void HopDistanceRouter::setEP(Router::AccessAllowanceEdgePreferences* ep) {
	if (m_ep)
		delete m_ep;
	m_ep = ep;
}

void HopDistanceRouter::route(uint32_t startNode, uint32_t endNode, Router::PathVisitor* pathVisitor) {
	if (startNode == endNode) {
		pathVisitor->visit(startNode);
		return;
	}
	std::vector<uint32_t>  nodeQueue;
	std::unordered_map<uint32_t, NodeHopDistInfo> visitedNodes;
	
	visitedNodes[startNode] = NodeHopDistInfo(startNode, 0);
	nodeQueue.emplace_back(startNode);
	for(uint32_t i(0); i < nodeQueue.size() && !visitedNodes.count(endNode); ++i) {
		uint32_t curNodeId = nodeQueue.at(i);
		const NodeHopDistInfo & ni = visitedNodes.at(curNodeId);
		for (Graph::ConstEdgeIterator childIt(graph().edgesBegin(curNodeId)), childEnd(graph().edgesEnd(curNodeId)); childIt != childEnd; ++childIt) {
			if (!m_ep->accessAllowed(*childIt)) {
				continue;
			}
			if (!visitedNodes.count(childIt->target)) {
				nodeQueue.push_back(childIt->target);
				visitedNodes[childIt->target] = NodeHopDistInfo(curNodeId, ni.distance);
			}
		}
	}
	if (!visitedNodes.count(endNode)) {
		return;
	}
	std::vector<uint32_t> tmp;
	//backtrack
	uint32_t curNodeId = endNode;
	while(curNodeId != startNode) {
		tmp.push_back(curNodeId);
		curNodeId = visitedNodes.at(curNodeId).parentNodeId;
	}
	tmp.push_back(startNode);
	
	//let pathVisitor know of the path
	for(std::vector<uint32_t>::reverse_iterator it(tmp.rbegin()), end(tmp.rend()); it != end; ++it) {
		pathVisitor->visit(*it);
	}
}

namespace DijkstraRouterImp {

	class NodeDistanceInfoSet;

	struct BorderSmaller {
		NodeDistanceInfoSet * ni;
		BorderSmaller(NodeDistanceInfoSet * ni) : ni(ni) {}
		bool operator()(uint32_t a, uint32_t b) const;
	};
	
	struct BorderEqual {
		NodeDistanceInfoSet * ni;
		BorderEqual(NodeDistanceInfoSet * ni) : ni(ni) {}
		bool operator()(uint32_t a, uint32_t b);
	};

	typedef std::multiset<uint32_t, BorderSmaller> BorderSet;
	typedef BorderSet::iterator BorderIterator;

	struct DijkstraNodeInfo {
		uint32_t parentNodeId;
		double weight;
		DijkstraNodeInfo(uint32_t parentNodeId, double weight) : parentNodeId(parentNodeId), weight(weight) {}
	};
	
	struct DijkstraNodeInfoSet: DijkstraNodeInfo {
		BorderIterator borderIt;
		bool inBorder() const;
		void removeFromBorder();
		void setBorderIt(const BorderIterator & bIt);
		DijkstraNodeInfoSet(uint32_t parentNodeId, double weight) : DijkstraNodeInfo(parentNodeId, weight), borderIt() {}
	};

	struct NodeDistanceInfo {
		std::unordered_map<uint32_t, DijkstraNodeInfo> d;
	};

	struct NodeDistanceInfoSet {
		std::unordered_map<uint32_t, DijkstraNodeInfoSet> d;
	};
	
	bool BorderSmaller::operator()(uint32_t a, uint32_t b) const {
		return ni->d.at(a).weight < ni->d.at(b).weight;
	}

	bool BorderEqual::operator()(uint32_t a, uint32_t b) {
		const DijkstraNodeInfoSet & ai = ni->d.at(a);
		const DijkstraNodeInfoSet & bi = ni->d.at(b);
		return (ai.weight == bi.weight ? a < b : ai.weight < bi.weight);
	}
	
	bool DijkstraNodeInfoSet::inBorder() const {
		return borderIt != BorderIterator();
	}
	
	void DijkstraNodeInfoSet::removeFromBorder() {
		borderIt = BorderIterator();
	}

	void DijkstraNodeInfoSet::setBorderIt(const BorderIterator& bIt) {
		borderIt = bIt;
	}
}

double DijkstraRouter::DistanceEdgePreferences::weight(const Graph::Edge& e) const {
	return e.distance;
}


DijkstraRouter::TimeEdgePreferences::TimeEdgePreferences(uint32_t accessTypeMask, double vehicleMaxSpeed) :
AccessAllowanceWeightEdgePreferences(accessTypeMask),
vehicleMaxSpeed((vehicleMaxSpeed*1000.0)/3.6)
{}

double DijkstraRouter::TimeEdgePreferences::weight(const Graph::Edge& e) const {
	return (double)e.distance / std::min<double>(e.speed, vehicleMaxSpeed);
}

DijkstraRouter::DijkstraRouter(const Graph* g) :
Router(g),
m_ep(new DistanceEdgePreferences(Graph::Edge::AT_ALL))
{}

DijkstraRouter::~DijkstraRouter() {
	delete m_ep;
}

void DijkstraRouter::setEP(Router::AccessAllowanceWeightEdgePreferences* ep) {
	if (m_ep)
		delete m_ep;
	m_ep = ep;
}

void DijkstraRouter::route(uint32_t startNode, uint32_t endNode, Router::PathVisitor* pathVisitor) {
	if (m_heapRoute) {
		routeHeap(startNode, endNode, pathVisitor);
	}
	else {
		routeSet(startNode, endNode, pathVisitor);
	}
}

void DijkstraRouter::routeHeap(uint32_t startNode, uint32_t endNode, Router::PathVisitor* pathVisitor) {
	using namespace DijkstraRouterImp;
	
	struct BorderInfo {
		uint32_t nodeId;
		double distance;
		BorderInfo(uint32_t nodeId, double distance) : nodeId(nodeId), distance(distance) {}
		bool operator<(const BorderInfo & other) const {
			return (distance == other.distance ? nodeId < other.nodeId : distance >= other.distance);
		}
	};
	
	typedef std::priority_queue<BorderInfo> BorderQueue;
	
	NodeDistanceInfo discoveredNodes;
	BorderQueue border;

	
	discoveredNodes.d.emplace(startNode, DijkstraNodeInfoSet(startNode, 0));
	
	//first insert all neighbors of startNode into discovered nodes and into the border
	{
		for(Graph::ConstEdgeIterator eIt(graph().edgesBegin(startNode)), eEnd(graph().edgesEnd(startNode)); eIt != eEnd; ++eIt) {
			const Graph::Edge & e = *eIt;
			if (!m_ep->accessAllowed(e)) {
				continue;
			}
			double weight = m_ep->weight(e);
			if (discoveredNodes.d.count(e.target)) {
				DijkstraNodeInfo & ni = discoveredNodes.d.at(e.target);
				ni.weight = std::min(ni.weight, weight);
			}
			else {
				discoveredNodes.d.emplace(e.target, DijkstraNodeInfoSet(startNode, weight));
			}
			border.emplace(e.target, weight);
		}
	}
	//now get the node on the border that is closest to startNode
	//remove it from the border and
	//relax its neighbors if its distance is equal to the recorded in discoveredNodes
	while (border.size()) {
		BorderInfo binfo = border.top();
		border.pop();
		
		uint32_t curNodeId = binfo.nodeId;
		DijkstraNodeInfo & ni = discoveredNodes.d.at(curNodeId);
		
		//check if we have to expand it
		if (binfo.distance > ni.weight) {
			continue;
		}
		
		if (curNodeId == endNode) {
			border = BorderQueue();
			break;
		}
		
		for(Graph::ConstEdgeIterator eIt(graph().edgesBegin(curNodeId)), eEnd(graph().edgesEnd(curNodeId)); eIt != eEnd; ++eIt) {
			const Graph::Edge & e = *eIt;
			if (!m_ep->accessAllowed(e)) {
				continue;
			}
			if (discoveredNodes.d.count(e.target)) {//already there, update the distance if necessary
				DijkstraNodeInfo & nni = discoveredNodes.d.at(e.target);
				double edgeWeight = m_ep->weight(e);
				if (nni.weight > ni.weight+edgeWeight) {
					//this also means that e.target musst be in the border
					nni.weight = ni.weight+edgeWeight;
					nni.parentNodeId = curNodeId;
					//just push it, don't do a decrease key
					border.emplace(e.target, nni.weight);
				}
			}
			else {
				double nw = ni.weight+m_ep->weight(e);
				discoveredNodes.d.emplace(e.target, DijkstraNodeInfoSet(curNodeId, nw));
				border.emplace(e.target, nw);
			}
		}
	}
	
	if (!discoveredNodes.d.count(endNode)) {
		return;
	}
	
	std::vector<uint32_t> tmp;
	//backtrack
	uint32_t curNodeId = endNode;
	while(curNodeId != startNode) {
		tmp.push_back(curNodeId);
		curNodeId = discoveredNodes.d.at(curNodeId).parentNodeId;
	}
	tmp.push_back(startNode);
	
	//let pathVisitor know of the path
	for(std::vector<uint32_t>::reverse_iterator it(tmp.rbegin()), end(tmp.rend()); it != end; ++it) {
		pathVisitor->visit(*it);
	}
}

void DijkstraRouter::routeSet(uint32_t startNode, uint32_t endNode, Router::PathVisitor* pathVisitor) {
	using namespace DijkstraRouterImp;

	
	NodeDistanceInfoSet discoveredNodes;
	BorderSmaller bs(&discoveredNodes);
	BorderSet border(bs); //this should actually be a heap, but C++ heap does not support decrease-key operation

	
	discoveredNodes.d.emplace(startNode, DijkstraNodeInfoSet(startNode, 0));
	
	//first insert all neighbors of startNode into discovered nodes and into the border
	{
		for(Graph::ConstEdgeIterator eIt(graph().edgesBegin(startNode)), eEnd(graph().edgesEnd(startNode)); eIt != eEnd; ++eIt) {
			const Graph::Edge & e = *eIt;
			if (!m_ep->accessAllowed(e)) {
				continue;
			}
			if (discoveredNodes.d.count(e.target)) {
				DijkstraNodeInfoSet & ni = discoveredNodes.d.at(e.target);
				ni.weight = std::min(ni.weight, m_ep->weight(e));
			}
			else {
				discoveredNodes.d.emplace(e.target, DijkstraNodeInfoSet(startNode, m_ep->weight(e)));
			}
		}
		for(Graph::ConstEdgeIterator eIt(graph().edgesBegin(startNode)), eEnd(graph().edgesEnd(startNode)); eIt != eEnd; ++eIt) {
			const Graph::Edge & e = *eIt;
			if (m_ep->accessAllowed(e)) {
				discoveredNodes.d.at(e.target).setBorderIt( border.insert(eIt->target) );
			}
		}
	}
	//now get the node on the border that is closest to startNode
	//remove it from the border and relax its neighbors
	while (border.size()) {
		BorderIterator bIt = border.begin();
		
		uint32_t curNodeId = *bIt;
		DijkstraNodeInfoSet & ni = discoveredNodes.d.at(curNodeId);
		
		border.erase(bIt);
		ni.removeFromBorder();
		
		if (curNodeId == endNode) {
			border.clear();
			break;
		}
		
		for(Graph::ConstEdgeIterator eIt(graph().edgesBegin(curNodeId)), eEnd(graph().edgesEnd(curNodeId)); eIt != eEnd; ++eIt) {
			const Graph::Edge & e = *eIt;
			if (!m_ep->accessAllowed(e)) {
				continue;
			}
			if (discoveredNodes.d.count(e.target)) {//already there, update the distance if necessary
				DijkstraNodeInfoSet & nni = discoveredNodes.d.at(e.target);
				if (nni.weight > ni.weight+m_ep->weight(e)) {
					//this also means that e.target musst be in the border
					
					//we FIRST have to remove this from the border to preserve the ordering in it
					border.erase(nni.borderIt); //decrease-key operation part-1
					
					nni.weight = ni.weight+m_ep->weight(e);
					nni.parentNodeId = curNodeId;
					
					nni.setBorderIt( border.insert(e.target) );  //decrease-key operation part-2
				}
			}
			else {
				auto x = discoveredNodes.d.emplace(e.target, DijkstraNodeInfoSet(curNodeId, ni.weight+m_ep->weight(e)));
				x.first->second.setBorderIt( border.insert(e.target) );
			}
		}
	}
	
	if (!discoveredNodes.d.count(endNode)) {
		return;
	}
	
	std::vector<uint32_t> tmp;
	//backtrack
	uint32_t curNodeId = endNode;
	while(curNodeId != startNode) {
		tmp.push_back(curNodeId);
		curNodeId = discoveredNodes.d.at(curNodeId).parentNodeId;
	}
	tmp.push_back(startNode);
	
	//let pathVisitor know of the path
	for(std::vector<uint32_t>::reverse_iterator it(tmp.rbegin()), end(tmp.rend()); it != end; ++it) {
		pathVisitor->visit(*it);
	}
}

void AStarRouter::route(uint32_t /*startNode*/, uint32_t /*endNode*/, Router::PathVisitor* /*pathVisitor*/) {
}



}}//end namespace