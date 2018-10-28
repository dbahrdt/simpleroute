#ifndef SIMPLE_ROUTE_STATE_H
#define SIMPLE_ROUTE_STATE_H
#include "Grid.h"
#include "Graph.h"
#include "Router.h"
#include "MultiReaderSingleWriterLock.h"

#include <memory>
#include <unordered_set>

namespace simpleroute {

struct Config {
	Config() : latCount(100), lonCount(100), doSpatialSort(false), at(0) {}
	std::string graphFileName;
	uint32_t latCount;
	uint32_t lonCount;
	bool doSpatialSort;
	int at;
};

struct State {
	Graph graph;
	Grid grid;
	
	std::unordered_set<uint32_t> enabledNodes;
	MultiReaderSingleWriterLock enabledNodesLock;
	
	std::unordered_set<uint32_t> enabledEdges;
	MultiReaderSingleWriterLock enabledEdgesLock;
	State(const Config & cfg);
};

typedef std::shared_ptr<State> StatePtr;


}//end namespace simpleroute

#endif
