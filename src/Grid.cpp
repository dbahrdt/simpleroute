#include "Grid.h"
#include "util.h"
#include <unordered_set>
#include <algorithm>
#include <assert.h>
#include <iostream>
#include <cmath>

#include "Graph.h"
#define GRID_PADDING 0.001

namespace simpleroute {

Grid::Grid(const Grid& other) :
m_minLat(other.m_minLat),
m_maxLat(other.m_maxLat),
m_minLon(other.m_minLon),
m_maxLon(other.m_maxLon),
m_latCount(other.m_latCount),
m_lonCount(other.m_lonCount),
m_bins(other.m_bins),
m_nodeRefs(other.m_nodeRefs),
m_g(other.m_g)
{}

Grid::Grid(Grid&& other) :
m_minLat(other.m_minLat),
m_maxLat(other.m_maxLat),
m_minLon(other.m_minLon),
m_maxLon(other.m_maxLon),
m_latCount(other.m_latCount),
m_lonCount(other.m_lonCount),
m_bins( std::move(other.m_bins) ),
m_nodeRefs( std::move(other.m_nodeRefs) ),
m_g(other.m_g)
{}

Grid::Grid() :
m_minLat(-1337.0),
m_maxLat(-1337.0),
m_minLon(-1337.0),
m_maxLon(-1337.0),
m_latCount(0),
m_lonCount(0),
m_g(0)
{}

Grid& Grid::operator=(Grid&& other) {
	m_minLat = other.m_minLat;
	m_maxLat = other.m_maxLat;
	m_minLon = other.m_minLon;
	m_maxLon = other.m_maxLon;
	m_latCount = other.m_latCount;
	m_lonCount = other.m_lonCount;
	m_bins = std::move(other.m_bins);
	m_nodeRefs = std::move(other.m_nodeRefs);
	m_g = other.m_g;
	return *this;
}

Grid& Grid::operator=(const Grid& other) {
	m_minLat = other.m_minLat;
	m_maxLat = other.m_maxLat;
	m_minLon = other.m_minLon;
	m_maxLon = other.m_maxLon;
	m_latCount = other.m_latCount;
	m_lonCount = other.m_lonCount;
	m_bins = other.m_bins;
	m_nodeRefs = other.m_nodeRefs;
	m_g = other.m_g;
	return *this;
}

uint32_t Grid::bin(uint32_t latBin, uint32_t lonBin) const {
	assert(latBin < m_latCount && lonBin < m_lonCount);
	return latBin*m_lonCount+lonBin;
}

void Grid::bin(double lat, double lon, uint32_t & latBin, uint32_t & lonBin) const {
	latBin = ((lat-m_minLat)*m_latCount)/(m_maxLat-m_minLat);
	lonBin = ((lon-m_minLon)*m_lonCount)/(m_maxLon-m_minLon);
}

Grid::Grid(const Graph * g, uint32_t latCount, uint32_t lonCount) :
m_minLat(-1337.0),
m_maxLat(-1337.0),
m_minLon(-1337.0),
m_maxLon(-1337.0),
m_latCount(latCount),
m_lonCount(lonCount),
m_g(g)
{
	if (! (m_latCount*m_lonCount)) {
		throw std::runtime_error("Can not create a grid with 0 grid cells");
	}

	m_g->bbox(m_minLat, m_maxLat, m_minLon, m_maxLon);
	//add some offset so that nodes on the border are still in the grid
	m_minLat -= GRID_PADDING;
	m_minLon -= GRID_PADDING;
	m_maxLat += GRID_PADDING;
	m_maxLon += GRID_PADDING;
	
// 	double latStep = (maxLat-minLat)/latCount;
// 	double lonStep = (maxLon-minLon)/lonCount;
	
	//now let's insert all those nodes into our grid
	//we first have to find the bounds for our bins
	m_bins.resize(m_latCount*m_lonCount, Bin(0, 0));
	m_nodeRefs.resize(m_g->nodeCount());
	for(uint32_t i(0), s(m_g->nodeCount()); i < s; ++i) {
		const Graph::NodeInfo & ni = m_g->nodeInfo(i);
		uint32_t latBin, lonBin;
		bin(ni.lat, ni.lon, latBin, lonBin);
		Bin & b = m_bins.at(bin(latBin, lonBin));
		b.end += 1;
	}
	//now adjust the offsets accordingly,
	//we will use the end-pointer as temporary offset to place the node id into the m_nodeRefs array
	uint32_t curOff = 0;
	for(uint32_t i(0), s(m_bins.size()); i < s; ++i) {
		Bin & b = m_bins.at(i);
		b.begin = curOff;
		curOff += b.end;
		b.end = b.begin;
	}
	
	for(uint32_t i(0), s(m_g->nodeCount()); i < s; ++i) {
		const Graph::NodeInfo & ni = m_g->nodeInfo(i);
		uint32_t latBin, lonBin;
		bin(ni.lat, ni.lon, latBin, lonBin);
		Bin & b = m_bins.at(bin(latBin, lonBin));
		m_nodeRefs.at(b.end) = i;
		b.end += 1;
	}
}

struct BinInfo {
	uint32_t latBin;
	uint32_t lonBin;
	double distance;
	BinInfo(uint32_t latBin, uint32_t lonBin, double distance) :
	latBin(latBin), lonBin(lonBin), distance(distance)
	{}
};

struct SearchBorder {
	int32_t latCount;
	int32_t lonCount;
	int32_t latCenter;
	int32_t lonCenter;
	int32_t radius;
	SearchBorder(uint32_t latCount, uint32_t lonCount, uint32_t latCenter, uint32_t lonCenter) :
	latCount(latCount),
	lonCount(lonCount),
	latCenter(latCenter),
	lonCenter(lonCenter),
	radius(0)
	{}
	
	void grow() {
		++radius;
	}
	
	bool valid() {
		return !(latCenter + radius >= latCount && latCenter - radius < 0 && lonCenter + radius >= lonCount && lonCenter - radius < 0);
	}
	
	//circle around the border
	//TP(uint32_t latBin, uint32_t lonBin)
	template<typename TP>
	void visit(TP p) const {
		if (!radius) {
			p(latCenter, lonCenter);
			return;
		}
		if (lonCenter - radius >= 0) { //the bottom lons
			int32_t lonBin = lonCenter - radius;
			int32_t latBin = std::max<int32_t>(0, latCenter-radius);
			int32_t latBinEnd = std::min<int32_t>(latCenter+radius, latCount);
			for(; latBin < latBinEnd; ++latBin) {
				p(latBin, lonBin);
			}
		}
		//the top lons
		if (lonCenter + radius < lonCount) {
			int32_t lonBin = lonCenter + radius;
			int32_t latBin = std::max<int32_t>(0, latCenter-radius);
			int32_t latBinEnd = std::min<int32_t>(latCenter+radius, latCount);
			for(; latBin < latBinEnd; ++latBin) {
				p(latBin, lonBin);
			}
		}
		if (latCenter - radius >= 0) {
			int32_t latBin = latCenter - radius;
			int32_t lonBin = std::max<int32_t>(0, lonCenter-radius-1);
			int32_t lonBinEnd = std::min<int32_t>(lonCenter+radius-1, latCount);
			for(; lonBin < lonBinEnd; ++lonBin) {
				p(latBin, lonBin);
			}
		}
		if (latCenter + radius < latCount) {
			int32_t latBin = latCenter + radius;
			int32_t lonBin = std::max<int32_t>(0, lonCenter-radius-1);
			int32_t lonBinEnd = std::min<int32_t>(lonCenter+radius-1, latCount);
			for(; lonBin < lonBinEnd; ++lonBin) {
				p(latBin, lonBin);
			}
		}
	}
};

void Grid::binCorners(uint32_t latBin, uint32_t lonBin, double& minLat, double& maxLat, double& minLon, double& maxLon) const {
	double latStep = (m_maxLat-m_minLat)/m_latCount;
	double lonStep = (m_maxLon-m_minLon)/m_lonCount;
	minLat = m_minLat+latStep*latBin;
	maxLat = minLat+latStep;
	minLon = m_minLon+lonStep*lonBin;
	maxLon = minLon+lonStep;
}


// There are 9 possible cases to calculate the distance
//-----------------------------
//<p,tl>=1|<p, t>=2|<p,tr>=3
//-----------------------------
//<p, l>=4|  0=5   |<p, r>=6
//-----------------------------
//<p,bl>=7|<p, b>=8|<p,br>=9
//
// ^
// |
//lon
// |
//-lat->
//
// First find the case, then do the math
//
//
//


double Grid::binDistance(uint32_t latBin, uint32_t lonBin, double lat, double lon) const {
	double minLat, maxLat, minLon, maxLon;
	binCorners(latBin, lonBin, minLat, maxLat, minLon, maxLon);
	
	if (lat < minLat) {//cases 1,4,7
		if (lon < minLon) { //case 7
			return std::fabs( distanceTo(lat, lon, minLat, minLon) );
		}
		else if (lon < maxLon) { //case 4
			return std::fabs( crossTrackDistance(minLat, minLon, minLat, maxLon, lat, lon) );
		}
		else { //case 1
			return std::fabs( distanceTo(lat, lon, minLat, maxLon) );
		}
	}
	else if (lat < maxLat) { //cases 2,5,8
		if (lon < minLon) { //case 8
			return std::fabs( crossTrackDistance(minLat, minLon, maxLat, minLon, lat, lon) );
		}
		else if (lon < maxLon) { //case 5
			return 0.0;
		}
		else { //case 2
			return std::fabs( crossTrackDistance(minLat, maxLon, maxLat, maxLon, lat, lon) );
		}
	}
	else {//cases 3,6,9
		if (lon < minLon) { //case 9
			return std::fabs( distanceTo(lat, lon, maxLat, minLon) );
		}
		else if (lon < maxLon) { //case 6
			return std::fabs( crossTrackDistance(maxLat, minLon, maxLat, maxLon, lat, lon) );
		}
		else { //case 3
			return std::fabs( distanceTo(lat, lon, maxLat, maxLon) );
		}
	}
}

uint32_t Grid::closest(double lat, double lon) const {
	if (!m_nodeRefs.size()) {
		return std::numeric_limits<uint32_t>::max();
	}

	uint32_t latBin, lonBin;
	{
		//clip coordinates to find the first bin, will cost more than doing it correctly, but should work
		double myLat(lat), myLon(lon);
		if (myLat < m_minLat) {
			myLat = m_minLat+GRID_PADDING;
		}
		else if (myLat >= m_maxLat) {
			myLat = m_maxLat-GRID_PADDING;
		}
		if (myLon < m_minLon) {
			myLon = m_minLon+GRID_PADDING;
		}
		else if (myLon >= m_maxLon) {
			myLon = m_maxLon-GRID_PADDING;
		}
		bin(myLat, myLon, latBin, lonBin);
	}
	
	SearchBorder sb(m_latCount, m_lonCount, latBin, lonBin);
	
	uint32_t bestMatch = std::numeric_limits<uint32_t>::max();
	double bestMatchDistance = std::numeric_limits<double>::max();
	
	std::vector<BinInfo> wq;
	wq.push_back(BinInfo(latBin, lonBin, binDistance(latBin, lonBin, lat, lon)));
	while(sb.valid()) { //there always is at least one node closest
		while(wq.size()) {
			BinInfo bi = wq.back();
			if (bi.distance >= bestMatchDistance) { //all other bins are even further away, so we found our node
				assert(bestMatch != std::numeric_limits<uint32_t>::max());
				return bestMatch;
			}
			const Bin & b = m_bins.at(bin(bi.latBin, bi.lonBin));
			wq.pop_back();
			for(uint32_t i(b.begin); i < b.end; ++i) {
				uint32_t nr = m_nodeRefs.at(i);
				const Graph::NodeInfo & ni = m_g->nodeInfo(nr);
				double nDist = std::fabs( distanceTo(lat, lon, ni.lat, ni.lon) );
				if (nDist < bestMatchDistance) {
					bestMatchDistance = nDist;
					bestMatch = nr;
				}
			}
		}
		sb.grow();
		
		//check sourroundig for alternatives
		sb.visit([&wq, &bestMatch, &bestMatchDistance, lat, lon, this](uint32_t latBin, uint32_t lonBin) {
			double binDist = this->binDistance(latBin, lonBin, lat, lon);
			if (binDist < bestMatchDistance && this->m_bins.at(this->bin(latBin, lonBin)).size()) {
				wq.push_back(BinInfo(latBin, lonBin, binDist));
			}
		});
		assert(bestMatchDistance != 0.0 || wq.size() == 0);
		//sort wq descending, so the smallest is at the back
		std::sort(wq.begin(), wq.end(), [](const BinInfo & a, const BinInfo & b) {
			return a.distance >= b.distance;
		});
	}
	assert(bestMatch != std::numeric_limits<uint32_t>::max());
	return bestMatch;
}

void Grid::printStats(std::ostream & out) {
	uint32_t maxNC = 0;
	uint32_t minNC = 0xFFFFFFFF;
	
	for(const Bin & bin : m_bins) {
		maxNC = std::max(maxNC, bin.size());
		minNC = std::min(minNC, bin.size());
	}

	out << "Grid::stats {\n";
	out << "\t#Latcount: " << m_latCount << "\n";
	out << "\t#Loncount: " << m_lonCount << "\n";
	out << "\tavg bin size: " << (double)m_nodeRefs.size()/(m_latCount*m_lonCount)<< "\n";
	out << "\tmax bin size: " << maxNC << "\n";
	out << "\tmin bin size: " << minNC << "\n";
	out << "}";
}

bool Grid::selfCheck() {
	double minLat, maxLat, minLon, maxLon;
	//check that every bin has only nodes that are within
	for(uint32_t latBin(0); latBin < m_latCount; ++latBin) {
		for(uint32_t lonBin(0); lonBin < m_lonCount; ++lonBin) {
			uint32_t binId = bin(latBin, lonBin);
			binCorners(latBin, lonBin, minLat, maxLat, minLon, maxLon);
			for(ConstNodeRefIterator it(binNodesBegin(binId)), end(binNodesEnd(binId)); it != end; ++it) {
				const Graph::NodeInfo & ni = m_g->nodeInfo(*it);
				if (ni.lat < minLat || ni.lat > maxLat || ni.lon < minLon || ni.lon > maxLon) {
					return false;
				}
			}
		}
	}
	//check that every node is in a bin
	std::vector<bool> nodeIsInBin(m_g->nodeCount(), false);
	
	for(uint32_t latBin(0); latBin < m_latCount; ++latBin) {
		for(uint32_t lonBin(0); lonBin < m_lonCount; ++lonBin) {
			uint32_t binId = bin(latBin, lonBin);
			for(ConstNodeRefIterator it(binNodesBegin(binId)), end(binNodesEnd(binId)); it != end; ++it) {
				if (nodeIsInBin.at(*it)) {
					return false;
				}
				nodeIsInBin[*it] = true;
			}
		}
	}
	for(bool x : nodeIsInBin) {
		if (!x) {
			return false;
		}
	}
	//now check if the find method always returns the correct node if we feed it node-coordinates
	for(uint32_t i(0), s(m_g->nodeCount()); i < s; ++i) {
		const Graph::NodeInfo & ni = m_g->nodeInfo(i);
		if (closest(ni.lat, ni.lon) != i) {
			uint32_t oi = closest(ni.lat, ni.lon);
			const Graph::NodeInfo & noi = m_g->nodeInfo(oi);
			double dist = std::fabs( distanceTo(ni.lat, ni.lon, noi.lat, noi.lon) );
			if (dist != 0.0) {
				std::cout << "Failed for node " << i << "; got " << oi << " with distance=" << dist << std::endl;
				return false;
			}
		}
	}
	return true;
}

}//end namespace
