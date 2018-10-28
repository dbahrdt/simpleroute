#include "State.h"
#include <iostream>

namespace simpleroute {

State::State(const Config& cfg) {
	std::cout << "Parsing graph from " << cfg.graphFileName << std::endl;
	graph = Graph::fromPBF(cfg.graphFileName, cfg.doSpatialSort, cfg.at);
	graph.printStats(std::cout);
	std::cout << std::endl;
	std::cout << "Creating grid" << std::endl;
	grid = Grid(&graph, cfg.latCount, cfg.lonCount);
	grid.printStats(std::cout);
	std::cout << std::endl;
}



}//end namespace simpleroute
