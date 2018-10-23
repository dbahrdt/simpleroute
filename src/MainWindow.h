#ifndef SIMPLE_ROUTE_MAIN_WINDOW_H
#define SIMPLE_ROUTE_MAIN_WINDOW_H
#include <QMainWindow>
#include <memory>
#include "State.h"

class QTableView;
class QComboBox;
class QLabel;

namespace simpleroute {

class MarbleMap;
class GraphNodesTableModel;

class GraphEdgesTable;
class GraphEdgesTableModel;

namespace detail {

class BackgroundRouter: public QObject {
	Q_OBJECT
public:
	BackgroundRouter(QObject * parent, const StatePtr & state);
	~BackgroundRouter();
public slots:
	void reroute(int rt, int accessType);
	void route(uint32_t srcNode, uint32_t tgtNode, int rt, int accessType);
signals:
	void routeCalculated(const Graph::Route & route, double duration);
private:
	StatePtr m_state;
	uint32_t m_srcNode;
	uint32_t m_tgtNode;
};

}//end namespace detail

class MainWindow: public QMainWindow {
	Q_OBJECT
public:
	MainWindow(const StatePtr & state);
	virtual ~MainWindow();
public slots:
	void calculateRoute(double latSrc, double lonSrc, double latTgt, double lonTgt);
signals:
	void routeCalculated(const Graph::Route & route);
	void shownEdgesChanged();
	void shownNodesChanged();
private Q_SLOTS:
	void routeCalculated(const Graph::Route & route, double duration);
	void scrollToNodeEdges(uint32_t nodeId);
	void scrollToNode(uint32_t nodeId);
	void toggleNode(uint32_t nodeId);
	void toggleEdge(uint32_t edgeId);
	void clearShownNodes();
	void clearShownEdges();
	void routerConfigChanged();
private://data stuff
	StatePtr m_state;
	detail::BackgroundRouter m_br;
private://gui stuff
	MarbleMap * m_map;
	
	QTableView * m_nodesTableView;
	GraphEdgesTable * m_edgesTableView;
	
	GraphNodesTableModel * m_nodesTableModel;
	GraphEdgesTableModel * m_edgesTableModel;

	QComboBox * m_routerSelection;
	QComboBox * m_accessType;
	
	QLabel * m_statsLabel;
};

};

#endif