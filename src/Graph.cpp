#include "Graph.h"

#include <iostream>
#include <algorithm>
#include <assert.h>

#include "Grid.h"

namespace simpleroute {
	
Graph Graph::fromPBF(const std::string & path, bool spatialSort, int accessTypes) {
	
	Graph g( memgraph::Graph::fromPBF(path, accessTypes) );
	
	//now sort the nodes/edges according to their closesnes for more access-locality during dijkstra runs
	//We use the grid for this. One grid cell should have no more than about 1K nodes.
	//Let's assume the nodes are evenly distributed
	//(which is not the case, but locality should still be preserved for densly populaterd areas)
	//only do this if there are enoug nodes
	uint32_t binCount = g.nodes().size()/1024;
	
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
		myG.edges().reserve(g.edges().size());
		myG.nodeInfos().reserve(g.nodeInfos().size());
		myG.nodes().reserve(g.nodes().size());
		{
			Grid grid(&g, latCount, lonCount);
			grid.printStats(std::cout);
			std::vector<uint32_t> oldNodeId2newNodeId;
			oldNodeId2newNodeId.resize(g.nodes().size());
			for(uint32_t i(0), s(grid.binCount()); i < s; ++i) {
				for(Grid::ConstNodeRefIterator refIt(grid.binNodesBegin(i)), refEnd(grid.binNodesEnd(i)); refIt != refEnd; ++refIt) {
					oldNodeId2newNodeId.at(*refIt) = myG.nodes().size();
					const Node & on = g.nodes().at(*refIt);
					
					Node nn;
					nn.begin = myG.edges().size();
					nn.end = nn.begin + on.edgeCount();
					
					myG.nodes().push_back(nn);
					myG.nodeInfos().push_back( g.nodeInfos().at(*refIt) );
					myG.edges().insert(myG.edges().end(), g.edges().begin()+on.begin, g.edges().begin()+on.end);
				}
			}
			//adjust the nodeIds in the edges since these are still the old ids
			for(uint32_t i(0), s(myG.edgeCount()); i < s; ++i) {
				Graph::Edge & e = myG.edges().at(i);
				e.source = oldNodeId2newNodeId.at(e.source);
				e.target = oldNodeId2newNodeId.at(e.target);
			}
			//sort the target edges for every node
			for(uint32_t i(0), s(myG.nodeCount()); i < s; ++i) {
				Graph::Node & n = myG.nodes().at(i);
				std::sort(myG.edges().begin()+n.begin, myG.edges().begin()+n.end, [](const Graph::Edge & a, const Graph::Edge & b) {
					return a.target < b.target;
				});
			}
		}
		assert(myG.edgeCount() == g.edgeCount());
		assert(myG.nodes().size() == g.nodes().size());
		assert(myG.nodeInfos().size() == g.nodeInfos().size());
		
		g = std::move(myG);
		std::cout << "Clustering completed" << std::endl;
	}
	
	return g;
}

}//end namespace
