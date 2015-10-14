#include "Graph.h"
#include <osmpbf/parsehelpers.h>
#include <osmpbf/filter.h>
#include <osmpbf/iway.h>
#include <osmpbf/inode.h>
#include <unordered_map>
#include <algorithm>
#include "util.h"
#include "Grid.h"
#include <iostream>

namespace simpleroute {

constexpr const char* Graph::Edge::edge_type_2_osm_highway_value[];
constexpr const int Graph::Edge::edge_type_2_speed[];
constexpr const int Graph::Edge::edge_type_2_access_allowance[];

Graph::Graph(const Graph& other) :
m_nodeInfo(other.m_nodeInfo),
m_nodes(other.m_nodes),
m_edges(other.m_edges)
{}

Graph::Graph(Graph&& other) :
m_nodeInfo(std::move(other.m_nodeInfo)),
m_nodes(std::move(other.m_nodes)),
m_edges(std::move(other.m_edges))
{}

Graph::Graph() {}

Graph::~Graph() {}

Graph& Graph::operator=(Graph && other) {
	m_nodeInfo = std::move(other.m_nodeInfo);
	m_nodes = std::move(other.m_nodes);
	m_edges = std::move(other.m_edges);
	return *this;
}

Graph& Graph::operator=(const Graph& other) {
	m_nodeInfo = other.m_nodeInfo;
	m_nodes = other.m_nodes;
	m_edges = other.m_edges;
	return *this;
}

Graph::Route Graph::routeInfo(std::vector< uint32_t >&& route, double vehicleMaxSpeed, int accessType) {
	vehicleMaxSpeed = (vehicleMaxSpeed*1000.0)/3.6;

	Route r;
	r.nodes = std::move(route);
	r.distance = 0.0;
	r.time = 0.0;
	r.at = accessType;
	if (r.nodes.size() < 1) {
		return r;
	}
	
	for(std::vector<uint32_t>::const_iterator prev(r.nodes.cbegin()), it(r.nodes.cbegin()+1), end(r.nodes.cend()); it != end; ++it, ++prev) {
		//find the edge
		for(ConstEdgeIterator eIt(edgesBegin(*prev)), eEnd(edgesEnd(*prev)); eIt != eEnd; ++eIt) {
			if (eIt->target == *it) {
				r.distance += eIt->distance;
				r.time += (double)eIt->distance / std::min<double>(eIt->speed, vehicleMaxSpeed);
				break;
			}
		}
	}
	
	return r;
}

void Graph::bbox(double& minLat, double& maxLat, double& minLon, double& maxLon) const {
	minLat = std::numeric_limits<double>::max();
	minLon = std::numeric_limits<double>::max();
	maxLat = std::numeric_limits<double>::min();
	maxLon = std::numeric_limits<double>::min();
	
	for(uint32_t i(0), s(nodeCount()); i < s; ++i) {
		const Graph::NodeInfo & ni = nodeInfo(i);
		minLat = std::min<double>(minLat, ni.lat);
		minLon = std::min<double>(minLon, ni.lon);
		
		maxLat = std::max<double>(maxLat, ni.lat);
		maxLon = std::max<double>(maxLon, ni.lon);
	}
}

void Graph::printStats(std::ostream& out) const {
	double minLat, maxLat, minLon, maxLon;
	bbox(minLat, maxLat, minLon, maxLon);
	
	uint32_t summedDeg = 0;
	uint32_t maxDeg = 0;
	uint32_t minDeg = 0xFFFFFFFF;
	for(const Node & n : m_nodes) {
		uint32_t nd = n.edgeCount();
		summedDeg += nd;
		maxDeg = std::max(maxDeg, nd);
		minDeg = std::min(minDeg, nd);
	}

	out << "Graph::stats {\n";
	out << "\t#nodes: " << nodeCount() << "\n";
	out << "\t#edges: " << edgeCount() << "\n";
	out << "\tLat extent: " << minLat << "->" << maxLat << "\n";
	out << "\tLon extent: " << minLon << "->" << maxLon << "\n";
	out << "\tavg deg: " << (double)summedDeg/nodeCount() << "\n";
	out << "\tmax deg: " << maxDeg << "\n";
	out << "\tmin deg: " << minDeg << "\n";
	out << "}";
}

//max speed in mm/s
bool parseMaxSpeed(const std::string & str, double & maxspeed) {
	std::string num;
	std::string::const_iterator it(str.cbegin()), end(str.cend());
	for(; it != end; ++it) {
		if (*it >= '0' && *it <= '9') {
			num += *it;
		}
		else {
			break;
		}
	}
	maxspeed = ::atof(num.c_str());
	//km/h
	if (str.find_first_of("km", it-str.cbegin()) != std::string::npos || str.find_first_of("m", it-str.cbegin()) == std::string::npos) {
		maxspeed = 1000*maxspeed/3.6;
	}
	else { //mp/h
		maxspeed = 1000*maxspeed*(1.6/3.6);
	}
	return true;
}

struct PBFParseState {
	struct SharedData {
		typedef std::unordered_map<int64_t, uint32_t> OsmNid2GraphNIdMap;
	
		std::mutex nodeIdLock;
		OsmNid2GraphNIdMap osmNId2GraphNId;
		
		std::mutex nodeInfoLock;
		Graph::NodeInfoContainer nodeInfo;
		
		std::atomic<uint32_t> candidateWayCount;
		
		//read only access
		std::unordered_map<std::string, Graph::Edge::EdgeTypes> hw2EdgeType;
		std::unordered_map<std::string, int> hw2AccessAllowance;
		SharedData() : candidateWayCount(0) {
			for(int i(Graph::Edge::ET_BEGIN); i < Graph::Edge::ET_END; ++i) {
				std::string hwValue(Graph::Edge::edge_type_2_osm_highway_value[i]);
				hw2EdgeType[hwValue] = (Graph::Edge::EdgeTypes)i;
				hw2AccessAllowance[hwValue] = Graph::Edge::edge_type_2_access_allowance[i];
			}
		}
	};
	struct ThreadLocalData {
		generics::RCPtr<osmpbf::KeyOnlyTagFilter> hwFilter;
		generics::RCPtr<osmpbf::KeyOnlyTagFilter> msFilter;
		
		osmpbf::RCFilterPtr onewayFilter;
		osmpbf::RCFilterPtr footDeniedFilter;
		osmpbf::RCFilterPtr bikeDeniedFilter;
		ThreadLocalData() :
			hwFilter( new osmpbf::KeyOnlyTagFilter("highway") ),
			msFilter( new osmpbf::KeyOnlyTagFilter("maxspeed") ),
			onewayFilter( new osmpbf::OrTagFilter({
												new osmpbf::BoolTagFilter("oneway", true),
												new osmpbf::KeyMultiValueTagFilter("highway", {"motorway", "motorway_link"}),
												new osmpbf::KeyMultiValueTagFilter("junction", {"roundabout"})
												})
											),
			footDeniedFilter( new osmpbf::OrTagFilter({
												new osmpbf::BoolTagFilter("motorroad", true),
												new osmpbf::AndTagFilter({
													new osmpbf::KeyOnlyTagFilter("foot"),
													new osmpbf::BoolTagFilter("foot", false)
												})
											})
										),
			bikeDeniedFilter( new osmpbf::OrTagFilter({
												new osmpbf::BoolTagFilter("motorroad", true),
												new osmpbf::AndTagFilter({
													new osmpbf::KeyOnlyTagFilter("bicycle"),
													new osmpbf::BoolTagFilter("bicycle", false)
												})
											})
										)
		{}
	};
	std::shared_ptr<SharedData> sd;
	ThreadLocalData ld;

	PBFParseState() :
	sd(new SharedData())
	{}
	
	PBFParseState(const PBFParseState & other) : sd(other.sd) {}
};


Graph Graph::fromPBF(const std::string & path, bool spatialSort) {
	
	osmpbf::OSMFileIn inFile(path);
	if (!inFile.open()) {
		throw std::runtime_error("Could not open file: " + path);
		return Graph();
	}
	
	PBFParseState ps;
	
	PBFParseState::SharedData::OsmNid2GraphNIdMap tlNIdMap;
	NodeInfoContainer tlNI;

	//fetch the relevant node ids
	std::cout << "Graph::fromPBF: fetching relevant node refs..." << std::flush;
	inFile.reset();
	osmpbf::parseFileCPPThreads(inFile, [ps, tlNIdMap](osmpbf::PrimitiveBlockInputAdaptor & pbi) mutable -> void {
		ps.ld.hwFilter->assignInputAdaptor(&pbi);
		if (!ps.ld.hwFilter->rebuildCache()) {
			return;
		}
		for(osmpbf::IWayStream way(pbi.getWayStream()); !way.isNull(); way.next()) {
			if (ps.ld.hwFilter->matches(way) && way.refsSize() >= 2) {
				ps.sd->candidateWayCount += 1;
				for(osmpbf::IWayStream::RefIterator refIt(way.refBegin()), refEnd(way.refEnd()); refIt != refEnd; ++refIt) {
					tlNIdMap[*refIt] = 0xFFFFFFFF;
				}
			}
		}
		{
			std::unique_lock<std::mutex> lck(ps.sd->nodeIdLock);
			ps.sd->osmNId2GraphNId.insert(tlNIdMap.begin(), tlNIdMap.end());
		}
		tlNIdMap.clear();
	}, std::min<uint32_t>(std::thread::hardware_concurrency(), 4), 1, true);
	std::cout << "done" << std::endl;
	
	//fetch the nodes and translate them into graph nodes
	std::cout << "Graph::fromPBF: trying to fetch " << ps.sd->osmNId2GraphNId.size() << " nodes..." << std::flush;
	inFile.reset();
	osmpbf::parseFileCPPThreads(inFile, [&ps, tlNI](osmpbf::PrimitiveBlockInputAdaptor & pbi) mutable -> void {
		for(osmpbf::INodeStream node(pbi.getNodeStream()); !node.isNull(); node.next()) {
			if (ps.sd->osmNId2GraphNId.count(node.id())) {
				tlNI.emplace_back(node.id(), node.latd(), node.lond());
			}
		}
		uint32_t nodeInfoOffset = 0xFFFFFFFF;
		{
			std::unique_lock<std::mutex> lck(ps.sd->nodeInfoLock);
			nodeInfoOffset = ps.sd->nodeInfo.size();
			ps.sd->nodeInfo.insert(ps.sd->nodeInfo.end(), tlNI.begin(), tlNI.end());
		}
		for(uint32_t i(0), s(tlNI.size()); i < s; ++i) {
			ps.sd->osmNId2GraphNId[tlNI[i].osmId] = nodeInfoOffset+i;
		}
		tlNI.clear();
	}, std::min<uint32_t>(std::thread::hardware_concurrency(), 4), 1, true);
	{
		uint32_t count = 0;
		for(const auto & x : ps.sd->osmNId2GraphNId) {
			if (x.second != 0xFFFFFFFF) {
				++count;
			}
		}
		std::cout << "got " << count << std::endl;
	}
	
	struct MyEdge {
		Graph::Edge ge;
		void reverse() {
			std::swap(ge.source, ge.target);
		}
	};
	
	std::vector<MyEdge> graphEdges, tlGE;
	std::mutex graphEdgesMtx;
	
	
	//build the edges
	std::cout << "Graph::fromPBF: fetching edges from " << ps.sd->candidateWayCount << " ways..." << std::flush;
	ps.sd->candidateWayCount = 0;
	inFile.reset();
	osmpbf::parseFileCPPThreads(inFile, 
		[ps, &graphEdges, &graphEdgesMtx, tlGE] (osmpbf::PrimitiveBlockInputAdaptor & pbi) mutable -> void
	{
		ps.ld.hwFilter->assignInputAdaptor(&pbi);
		if (!ps.ld.hwFilter->rebuildCache()) {
			return;
		}
		ps.ld.msFilter->assignInputAdaptor(&pbi);
		ps.ld.msFilter->rebuildCache();
		ps.ld.onewayFilter->assignInputAdaptor(&pbi);
		ps.ld.onewayFilter->rebuildCache();

		ps.ld.bikeDeniedFilter->assignInputAdaptor(&pbi);
		ps.ld.bikeDeniedFilter->rebuildCache();
		
		ps.ld.footDeniedFilter->assignInputAdaptor(&pbi);
		ps.ld.footDeniedFilter->rebuildCache();
		for(osmpbf::IWayStream way(pbi.getWayStream()); !way.isNull(); way.next()) {
			if (ps.ld.hwFilter->matches(way) && way.refsSize() >= 2) {
				ps.sd->candidateWayCount += 1;
				const std::string & hwValue = way.value(ps.ld.hwFilter->matchingTag());
				Graph::Edge::EdgeTypes et = (ps.sd->hw2EdgeType.count(hwValue) ? ps.sd->hw2EdgeType.at(hwValue) : Graph::Edge::ET_INVALID);
				int at = (ps.sd->hw2AccessAllowance.count(hwValue) ? ps.sd->hw2AccessAllowance.at(hwValue) : Graph::Edge::AT_ALL);
				bool isOneWay = ps.ld.onewayFilter->matches(way);
				double maxspeed = 0.0;
				if (ps.ld.msFilter->matches(way)) {
					parseMaxSpeed(way.value(ps.ld.msFilter->matchingTag()), maxspeed);
				}
				if (maxspeed == 0.0) {
					maxspeed = 1000*Graph::Edge::edge_type_2_speed[et]/3.6;
				}
				
				if (ps.ld.footDeniedFilter->matches(way)) {
					at = at & (~Graph::Edge::AT_FOOT);
				}
				
				if (ps.ld.bikeDeniedFilter->matches(way)) {
					at = at & (~Graph::Edge::AT_BIKE);
				}
				
				for(osmpbf::IWayStream::RefIterator refIt(way.refBegin()+1), refPrev(way.refBegin()), refEnd(way.refEnd()); refIt != refEnd; ++refIt, ++refPrev) {
					int64_t osmSource = *refPrev;
					int64_t osmTarget = *refIt;
					if (!(ps.sd->osmNId2GraphNId.count(osmSource) && ps.sd->osmNId2GraphNId.at(osmSource) != 0xFFFFFFFF && 
							ps.sd->osmNId2GraphNId.count(osmTarget) && ps.sd->osmNId2GraphNId.at(osmTarget) != 0xFFFFFFFF)) {
						continue;
					}
					MyEdge me;
					me.ge.source = ps.sd->osmNId2GraphNId.at(osmSource);
					me.ge.target = ps.sd->osmNId2GraphNId.at(osmTarget);
					me.ge.oneway = (isOneWay ? 1 : 0);
					me.ge.type = et;
					me.ge.access = at;
					me.ge.speed = maxspeed;
					me.ge.distance = 1000*distanceTo(ps.sd->nodeInfo.at(me.ge.source).lat, ps.sd->nodeInfo.at(me.ge.source).lon, ps.sd->nodeInfo.at(me.ge.target).lat, ps.sd->nodeInfo.at(me.ge.target).lon);
					tlGE.push_back(me);
					if (!isOneWay) {
						me.reverse();
						tlGE.push_back(me);
					}
				}
			}
		}
		{
			std::unique_lock<std::mutex> lck(graphEdgesMtx);
			graphEdges.insert(graphEdges.end(), tlGE.begin(), tlGE.end());
		}
		tlGE.clear();
	}, std::min<uint32_t>(std::thread::hardware_concurrency(), 4), 1, true);
	std::cout << "got " << graphEdges.size() << " edges from " << ps.sd->candidateWayCount << " ways" << std::endl;
	
	assert(graphEdges.size() <= std::numeric_limits<uint32_t>::max());
	
	std::sort(graphEdges.begin(), graphEdges.end(), [](const MyEdge & a, const MyEdge & b) {
		return (a.ge.source == b.ge.source ? a.ge.target < b.ge.target : a.ge.source < b.ge.source);
	});
	
	Graph g;
	{//assemble the graph
		g.m_edges.resize(graphEdges.size());
		g.m_nodeInfo = std::move(ps.sd->nodeInfo);
		g.m_nodes.resize(g.m_nodeInfo.size());
		
		Graph::Node gn;
		gn.begin = 0;
		gn.end = 0;
		uint32_t curNodeId = 0;
		for(uint32_t i(0), s(graphEdges.size()); i < s; ++i) {
			MyEdge tmpE = graphEdges.at(i);
			g.m_edges.at(i) = tmpE.ge;
			if (tmpE.ge.source != curNodeId) {
				gn.end = i;
				g.m_nodes.at(curNodeId) = gn;
				gn.begin = i;
				curNodeId = tmpE.ge.source;
			}
		}
		gn.end = graphEdges.size();
		g.m_nodes.at(curNodeId) = gn;
	}
	assert(g.m_nodes.size() == g.m_nodeInfo.size());
	
	//now sort the nodes/edges according to their closesnes for more access-locality during dijkstra runs
	//We use the grid for this. One grid cell should have no more than about 1K nodes.
	//Let's assume the nodes are evenly distributed
	//(which is not the case, but locality should still be preserved for densly populaterd areas)
	//only do this if there are enoug nodes
	uint32_t binCount = g.m_nodes.size()/1024;
	
	if (spatialSort && binCount > 100) {
		std::cout << "Clustering graph nodes" << std::endl;
		g.printStats(std::cout);
		std::cout << std::endl;
		
		uint32_t latCount, lonCount;
		double minLat, maxLat, minLon, maxLon;
		g.bbox(minLat, maxLat, minLon, maxLon);
		
// 		double area = (maxLat-minLat)*(maxLon-minLon);
		
		//scale both axis proprionaly to their extent
		latCount = (binCount/(maxLat-minLat))/2;//(maxLat-minLat)/(area*2)*binCount;
		lonCount = (binCount/(maxLon-minLon))/2;//(maxLon-minLon)/(area*2)*binCount;
		
		//create a new graph with sorted nodes
		Graph myG;
		myG.m_edges.reserve(g.m_edges.size());
		myG.m_nodeInfo.reserve(g.m_nodeInfo.size());
		myG.m_nodes.reserve(g.m_nodes.size());
		{
			Grid grid(&g, latCount, lonCount);
			grid.printStats(std::cout);
			std::vector<uint32_t> oldNodeId2newNodeId;
			oldNodeId2newNodeId.resize(g.m_nodes.size());
			for(uint32_t i(0), s(grid.binCount()); i < s; ++i) {
				for(Grid::ConstNodeRefIterator refIt(grid.binNodesBegin(i)), refEnd(grid.binNodesEnd(i)); refIt != refEnd; ++refIt) {
					oldNodeId2newNodeId.at(*refIt) = myG.m_nodes.size();
					const Node & on = g.m_nodes.at(*refIt);
					
					Node nn;
					nn.begin = myG.m_edges.size();
					nn.end = nn.begin + on.edgeCount();
					
					myG.m_nodes.push_back(nn);
					myG.m_nodeInfo.push_back( g.m_nodeInfo.at(*refIt) );
					myG.m_edges.insert(myG.m_edges.end(), g.m_edges.begin()+on.begin, g.m_edges.begin()+on.end);
				}
			}
			//adjust the nodeIds in the edges since these are still the old ids
			for(uint32_t i(0), s(myG.edgeCount()); i < s; ++i) {
				Graph::Edge & e = myG.m_edges.at(i);
				e.source = oldNodeId2newNodeId.at(e.source);
				e.target = oldNodeId2newNodeId.at(e.target);
			}
			//sort the target edges for every node
			for(uint32_t i(0), s(myG.nodeCount()); i < s; ++i) {
				Graph::Node & n = myG.m_nodes.at(i);
				std::sort(myG.m_edges.begin()+n.begin, myG.m_edges.begin()+n.end, [](const Graph::Edge & a, const Graph::Edge & b) {
					return a.target < b.target;
				});
			}
		}
		assert(myG.edgeCount() == g.edgeCount());
		assert(myG.m_nodes.size() == g.m_nodes.size());
		assert(myG.m_nodeInfo.size() == g.m_nodeInfo.size());
		
		g = std::move(myG);
		std::cout << "Clustering completed" << std::endl;
	}
	
	return g;
}

bool Graph::selfCheck() {
	//check existence of all nodes/edges
	uint32_t nc = nodeCount();
	uint32_t ec = edgeCount();
	for(uint32_t i(0); i < nc; ++i) {
		const Node & n = node(i);
		if (n.begin == 0xFFFFFFFF || n.end == 0xFFFFFFFF) {
			if (n.begin != n.end) {
				return false;
			}
		}
		else if (n.begin >= ec || n.end < n.begin) {
			return false;
		}
		for(ConstEdgeIterator eIt(edgesBegin(i)), eEnd(edgesEnd(i)); eIt != eEnd; ++eIt) {
			const Edge & e = *eIt;
			if (e.source != i) {
				return false;
			}
			if (e.target >= nc) {
				return false;
			}
			if (!e.oneway) {
				bool found = false;
				for(ConstEdgeIterator oeIt(edgesBegin(e.target)), oeEnd(edgesEnd(e.target)); oeIt != oeEnd; ++oeIt) {
					if (oeIt->target == e.source) {
						found = true;
						break;
					}
				}
				if (!found) {
					return false;
				}
			}
		}
	}
	return true;
}

}//end namespace