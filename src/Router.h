#ifndef SIMPLE_ROUTE_ROUTER_H
#define SIMPLE_ROUTE_ROUTER_H
#include "Graph.h"

#include <unordered_set>

namespace simpleroute {

class Router {
public:
	struct PathVisitor {
		virtual void visit(uint32_t nodeRef) = 0;
	};
	
	struct AccessAllowanceEdgePreferences {
		//set the allowed access types
		uint32_t accessTypeMask;
		AccessAllowanceEdgePreferences(uint32_t accessTypeMask = Graph::Edge::AT_ALL) : accessTypeMask(accessTypeMask) {}
		virtual ~AccessAllowanceEdgePreferences() {}
		virtual bool accessAllowed(const Graph::Edge & e) const;
	};
	
	struct AccessAllowanceWeightEdgePreferences: AccessAllowanceEdgePreferences {
		AccessAllowanceWeightEdgePreferences(uint32_t accessTypeMask) : AccessAllowanceEdgePreferences(accessTypeMask) {}
		virtual ~AccessAllowanceWeightEdgePreferences() {}
		virtual double weight(const Graph::Edge & e) const = 0;
	};
	
	typedef enum {
		HOP_DISTANCE,
		DIJKSTRA_SET_DISTANCE, DIJKSTRA_SET_TIME,
		DIJKSTRA_PRIO_QUEUE_DISTANCE, DIJKSTRA_PRIO_QUEUE_TIME,
		A_STAR_DISTANCE, A_STAR_TIME
	} RouterTypes;
public:
	Router(const Graph * g) : m_g(g) {}
	virtual ~Router() {}
	virtual void route(uint32_t startNode, uint32_t endNode, PathVisitor * pathVisitor) = 0;
protected:
	inline const Graph & graph() const { return *m_g; }
private:
	const Graph * m_g;
};

namespace detail {

class HopDistanceRouter: public Router {
public:
	HopDistanceRouter(const Graph * g);
	virtual ~HopDistanceRouter();
	///takes ownership of ep
	void setEP(AccessAllowanceEdgePreferences * ep);
	virtual void route(uint32_t startNode, uint32_t endNode, PathVisitor * pathVisitor) override;
private:
	AccessAllowanceEdgePreferences * m_ep;
};


class DijkstraRouter: public Router {
public:
	struct DistanceEdgePreferences: AccessAllowanceWeightEdgePreferences {
		DistanceEdgePreferences(uint32_t accessTypeMask) : AccessAllowanceWeightEdgePreferences(accessTypeMask) {}
		virtual ~DistanceEdgePreferences() {}
		virtual double weight(const Graph::Edge& e) const override;
	};

	struct TimeEdgePreferences: AccessAllowanceWeightEdgePreferences {
		///@param vehicleMaxSpeed in km/h
		TimeEdgePreferences(uint32_t accessTypeMask, double vehicleMaxSpeed);
		virtual ~TimeEdgePreferences() {}
		virtual double weight(const Graph::Edge& e) const override;
		double vehicleMaxSpeed;
	};
public:
	DijkstraRouter(const Graph * g);
	virtual ~DijkstraRouter();
	
	///takes ownership of ep
	void setEP(AccessAllowanceWeightEdgePreferences * ep);
	void setHeapType(bool usePrioQueue) { m_heapRoute = usePrioQueue; }
	virtual void route(uint32_t startNode, uint32_t endNode, PathVisitor * pathVisitor) override;
protected:
	void routeSet(uint32_t startNode, uint32_t endNode, PathVisitor * pathVisitor);
	void routeHeap(uint32_t startNode, uint32_t endNode, PathVisitor * pathVisitor);
private:
	AccessAllowanceWeightEdgePreferences * m_ep;
	bool m_heapRoute;
};

class AStarRouter: public Router {
public:
	AStarRouter(const Graph * g) : Router(g) {}
	virtual ~AStarRouter() {}
	virtual void route(uint32_t startNode, uint32_t endNode, PathVisitor * pathVisitor);
};

class CHRouter: public Router {
public:
	CHRouter(const Graph * g, const CHInfo * chinfo, Graph::Edge::AccessTypes at);
	virtual ~CHRouter() {}
	virtual void route(uint32_t startNode, uint32_t endNode, PathVisitor * pathVisitor);
private:
	const CHInfo * m_chInfo;
	Graph::Edge::AccessTypes at;
};

}}//end namespace

#endif