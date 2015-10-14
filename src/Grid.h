#ifndef SIMPLE_ROUTE_GRID_H
#define SIMPLE_ROUTE_GRID_H
#include <vector>
#include <stdint.h>
#include <ostream>

namespace simpleroute {

class Graph;

//only usefull in conjcuntion with the graph
class Grid {
public:
	typedef std::vector<uint32_t>::const_iterator ConstNodeRefIterator; 
public:
	Grid();
	Grid(Grid && other);
	Grid(const Grid & other);
	Grid(const Graph * g, uint32_t latCount, uint32_t lonCount);
	virtual ~Grid() {}
	Grid & operator=(const Grid & other);
	Grid & operator=(Grid && other);
	
	///return id of the closest node, does not consider wrap-around, returns std::numeric_limits<uint32_t>::max() if no node was found
	uint32_t closest(double lat, double lon) const;
	
	inline uint32_t binCount() const { return m_bins.size(); }
	ConstNodeRefIterator binNodesBegin(uint32_t bin) const { return m_nodeRefs.cbegin() + m_bins.at(bin).begin; }
	ConstNodeRefIterator binNodesEnd(uint32_t bin) const { return m_nodeRefs.cbegin() + m_bins.at(bin).end; }
	
	void printStats(std::ostream & out);
	
	bool selfCheck();
	
private:
	uint32_t bin(uint32_t latBin, uint32_t lonBin) const;
	void bin(double lat, double lon, uint32_t & latBin, uint32_t & lonBin) const;
	
	void binCorners(uint32_t latBin, uint32_t lonBin, double & minLat, double & maxLat, double & minLon, double & maxLon) const;
	
	double binDistance(uint32_t latBin, uint32_t lonBin, double lat, double lon) const;
private:
	struct Bin {
		uint32_t begin;
		uint32_t end;
		Bin(uint32_t begin, uint32_t end) : begin(begin), end(end) {}
		inline uint32_t size() const { return end-begin; }
	};
private:
	double m_minLat;
	double m_maxLat;
	double m_minLon;
	double m_maxLon;
	uint32_t m_latCount;
	uint32_t m_lonCount;
	std::vector<Bin> m_bins;
	std::vector<uint32_t> m_nodeRefs;
	const Graph * m_g;
};


}//end namespace simpleroute


#endif