#include "MainWindow.h"
#include <QMessageBox>
#include <QTableView>
#include <QComboBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <iostream>

#include "MarbleMap.h"
#include "GraphNodesTableModel.h"
#include "GraphEdgesTableModel.h"
#include "TimeMeasurer.h"

namespace simpleroute {
namespace detail {

BackgroundRouter::BackgroundRouter(QObject * parent, const StatePtr& state) :
QObject(parent),
m_state(state),
m_srcNode(0xFFFFFFFF),
m_tgtNode(0xFFFFFFFF)
{}

BackgroundRouter::~BackgroundRouter() {}

struct MyPathVisitor: public simpleroute::Router::PathVisitor {
	std::vector<uint32_t> p;
	virtual void visit(uint32_t nodeRef) override {
		p.push_back(nodeRef);
	}
};

void BackgroundRouter::reroute(int rt, int accessType) {
	if (m_srcNode != 0xFFFFFFFF && m_tgtNode != 0xFFFFFFFF) {
		route(m_srcNode, m_tgtNode, rt, accessType);
	}
}

void BackgroundRouter::route(uint32_t srcNode, uint32_t tgtNode, int rt, int accessType) {

	double vehicleMaxSpeed = 0.0;
	if (Graph::Edge::AT_FOOT & accessType) {
		vehicleMaxSpeed = 5.0;
	}
	if (Graph::Edge::AT_BIKE & accessType) {
		vehicleMaxSpeed = 15.0;
	}
	if (Graph::Edge::AT_CAR & accessType) {
		vehicleMaxSpeed = 130.0;
	}
	
	Router * router;
	bool usePrioQueue = false;
	switch (rt) {
	case Router::DIJKSTRA_PRIO_QUEUE_DISTANCE:
		usePrioQueue = true;
	case Router::DIJKSTRA_SET_DISTANCE:
		{
			detail::DijkstraRouter * tmp = new detail::DijkstraRouter(&(m_state->graph));
			tmp->setEP( new detail::DijkstraRouter::DistanceEdgePreferences(accessType) );
			tmp->setHeapType(usePrioQueue);
			router = tmp;
		}
		break;
	case Router::DIJKSTRA_PRIO_QUEUE_TIME:
		usePrioQueue = true;
	case Router::DIJKSTRA_SET_TIME:
		{
			detail::DijkstraRouter * tmp = new detail::DijkstraRouter(&(m_state->graph));
			tmp->setEP( new detail::DijkstraRouter::TimeEdgePreferences(accessType, vehicleMaxSpeed) );
			tmp->setHeapType(usePrioQueue);
			router = tmp;
		}
		break;
	case Router::HOP_DISTANCE:
	default:
		{
			detail::HopDistanceRouter * tmp = new detail::HopDistanceRouter(&(m_state->graph));
			tmp->setEP( new Router::AccessAllowanceEdgePreferences(accessType) );
			router = tmp;
		}
		break;
	}

	m_srcNode = srcNode;
	m_tgtNode = tgtNode;
	std::cout << "Calculating route from " << srcNode << " to " << tgtNode << std::endl;
	MyPathVisitor pv;
	//make sure the router is valid during usage
	TimeMeasurer tm;
	tm.begin();
	router->route(srcNode, tgtNode, &pv);
	Graph::Route r = m_state->graph.routeInfo(std::move(pv.p), vehicleMaxSpeed, accessType);
	tm.end();
	std::cout << "Calculated route from " << srcNode << " to " << tgtNode << " with " << r.nodes.size() << " hops in " << tm.elapsedMilliSeconds() << " ms" << std::endl;
	delete router;
	emit routeCalculated(r, tm.elapsedMilliSeconds());
}

}//end namespace

MainWindow::MainWindow(const StatePtr & state ):
QMainWindow(),
m_state(state),
m_br(this, state)
{
	m_map = new MarbleMap(this, m_state);
	m_map->setMapThemeId("earth/openstreetmap/openstreetmap.dgml");
	
	m_nodesTableModel = new GraphNodesTableModel(this, m_state);
	m_edgesTableModel = new GraphEdgesTableModel(this, m_state);
	
	m_nodesTableView = new QTableView(this);
	m_nodesTableView->setModel(m_nodesTableModel);
	
	m_edgesTableView = new GraphEdgesTable(this);
	m_edgesTableView->setModel(m_edgesTableModel);
// 	m_edgesTableView->verticalHeader()->setVisible(false);

	m_nodesTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
	m_edgesTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Stretch);
	
	QHBoxLayout * cfgLayout = new QHBoxLayout(this);
	
	m_routerSelection = new QComboBox(this);
	m_routerSelection->addItem("Hop distance", QVariant(Router::HOP_DISTANCE));
	m_routerSelection->addItem("Dijkstra std::set distance", QVariant(Router::DIJKSTRA_SET_DISTANCE));
	m_routerSelection->addItem("Dijkstra std::set time", QVariant(Router::DIJKSTRA_SET_TIME));
	m_routerSelection->addItem("Dijkstra std::priority_queue distance", QVariant(Router::DIJKSTRA_PRIO_QUEUE_DISTANCE));
	m_routerSelection->addItem("Dijkstra std::priority_queue time", QVariant(Router::DIJKSTRA_PRIO_QUEUE_TIME));
	
	m_accessType = new QComboBox(this);
	m_accessType->addItem("Foot", Graph::Edge::AT_FOOT);
	m_accessType->addItem("Bike", Graph::Edge::AT_BIKE);
	m_accessType->addItem("Car", Graph::Edge::AT_CAR);
	
	cfgLayout->addWidget(m_routerSelection);
	cfgLayout->addWidget(m_accessType);
	
	QWidget * cfgWidget = new QWidget(this);
	cfgWidget->setLayout(cfgLayout);
	
	m_statsLabel = new QLabel(this);
	
	QHBoxLayout * cfgStatsLayout = new QHBoxLayout(this);
	cfgStatsLayout->addWidget(cfgWidget);
	cfgStatsLayout->addWidget(m_statsLabel);
	
	QWidget * cfgStatsWidget = new QWidget(this);
	cfgStatsWidget->setLayout(cfgStatsLayout);
	
	QVBoxLayout * leftLayout = new QVBoxLayout(this);
	leftLayout->addWidget(m_nodesTableView, 5);
	leftLayout->addWidget(m_edgesTableView, 5);
	leftLayout->addWidget(cfgStatsWidget, 1);
	
	QWidget * leftLayoutWidget = new QWidget(this);
	leftLayoutWidget->setLayout(leftLayout);
	
	QHBoxLayout * mainLayout = new QHBoxLayout(this);
	mainLayout->addWidget(leftLayoutWidget, 2);
	mainLayout->addWidget(m_map, 3);
	
	connect(m_nodesTableView, SIGNAL(doubleClicked(QModelIndex)), m_nodesTableModel, SLOT(doubleClicked(QModelIndex)));
	connect(m_nodesTableView->horizontalHeader(), SIGNAL(sectionClicked(int)), m_nodesTableModel, SLOT(headerClicked(int)));
	
	connect(m_edgesTableView, SIGNAL(doubleClicked(QModelIndex)), m_edgesTableModel, SLOT(doubleClicked(QModelIndex)));
	connect(m_edgesTableView->horizontalHeader(), SIGNAL(sectionClicked(int)), m_edgesTableModel, SLOT(headerClicked(int))); 
	
	connect(m_nodesTableModel, SIGNAL(startEdgeClicked(uint32_t)), this, SLOT(scrollToNodeEdges(uint32_t)));
	connect(m_nodesTableModel, SIGNAL(toggleNodeClicked(uint32_t)), this, SLOT(toggleNode(uint32_t)));
	connect(m_nodesTableModel, SIGNAL(clearShownNodesClicked()), this, SLOT(clearShownNodes()));
	connect(m_nodesTableModel, SIGNAL(nodeCoordinateClicked(uint32_t)), m_map, SLOT(zoomToNode(uint32_t)));
	connect(this, SIGNAL(shownNodesChanged()), m_nodesTableModel, SLOT(resetData()));
	
	connect(m_edgesTableModel, SIGNAL(sourceNodeClicked(uint32_t)), this, SLOT(scrollToNode(uint32_t)));
	connect(m_edgesTableModel, SIGNAL(targetNodeClicked(uint32_t)), this, SLOT(scrollToNode(uint32_t)));
	connect(m_edgesTableModel, SIGNAL(toggleEdgeClicked(uint32_t)), this, SLOT(toggleEdge(uint32_t)));
	connect(m_edgesTableModel, SIGNAL(clearShownEdgesClicked()), this, SLOT(clearShownEdges()));
	connect(this, SIGNAL(shownEdgesChanged()), m_edgesTableModel, SLOT(resetData()));
	
	connect(m_routerSelection, SIGNAL(currentIndexChanged(int)), this, SLOT(routerConfigChanged()));
	connect(m_accessType, SIGNAL(currentIndexChanged(int)), this, SLOT(routerConfigChanged()));
	
	//connect background router
	connect(&m_br, SIGNAL(routeCalculated(Graph::Route,double)), this, SLOT(routeCalculated(Graph::Route,double)));
	
	//connect marble map
	connect(m_map, SIGNAL(calculateRoute(double,double,double,double)), this, SLOT(calculateRoute(double,double,double,double)));
	connect(this, SIGNAL(routeCalculated(Graph::Route)), m_map, SLOT(displayRoute(Graph::Route)));
	connect(this, SIGNAL(shownEdgesChanged()), m_map, SLOT(shownEdgesChanged()));
	connect(this, SIGNAL(shownNodesChanged()), m_map, SLOT(shownNodesChanged()));
	
	//set the router, will also correctly initalize it
	m_routerSelection->setCurrentIndex(0);
	m_accessType->setCurrentIndex(0);
	routerConfigChanged();
	
	QWidget * centralWidget = new QWidget(this);
	centralWidget->setLayout(mainLayout);
	setCentralWidget(centralWidget);
	
}

MainWindow::~MainWindow() {}

void MainWindow::calculateRoute(double latSrc, double lonSrc, double latTgt, double lonTgt) {
	int algoSelection = m_routerSelection->currentIndex();
	int accessSelection = m_accessType->currentIndex();
	
	if (algoSelection < 0 || accessSelection < 0) {
		return;
	}

	uint32_t srcNode = m_state->grid.closest(latSrc, lonSrc);
	uint32_t tgtNode = m_state->grid.closest(latTgt, lonTgt);

	if (srcNode == std::numeric_limits<uint32_t>::max() || tgtNode == std::numeric_limits<uint32_t>::max()) {
		QMessageBox msgBox;
		msgBox.setText("Either source or target node could not be found");
		msgBox.exec();
	}
	else {
		algoSelection = m_routerSelection->itemData(algoSelection).toInt();
		accessSelection = m_accessType->itemData(accessSelection).toInt();
		m_br.route(srcNode, tgtNode, algoSelection, accessSelection); //will emit routeCalculated when done
	}
}

void MainWindow::routeCalculated(const Graph::Route& route, double duration) {
	uint32_t distance = route.distance/1000;
	uint32_t travelMinutes = route.time/60;
	uint32_t travelSeconds = ((uint32_t)route.time)%60;
	uint32_t calcTime = duration;

	m_statsLabel->setText(QString("Distance: %1m\nTraveltime: %2m %3s\nCalculationtime: %4ms").arg(distance).arg(travelMinutes).arg(travelSeconds).arg(calcTime));
	emit routeCalculated(route);
}

void MainWindow::clearShownNodes() {
	MultiReaderSingleWriterLocker lck(m_state->enabledNodesLock, MultiReaderSingleWriterLocker::WRITE_LOCK);
	m_state->enabledNodes.clear();
	emit shownNodesChanged();
}

void MainWindow::clearShownEdges() {
	MultiReaderSingleWriterLocker lck(m_state->enabledEdgesLock, MultiReaderSingleWriterLocker::WRITE_LOCK);
	m_state->enabledEdges.clear();
	emit shownEdgesChanged();
}

void MainWindow::scrollToNode(uint32_t nodeId) {
	m_nodesTableView->scrollTo(m_nodesTableModel->index(nodeId, 0));
}

void MainWindow::scrollToNodeEdges(uint32_t nodeId) {
	const Graph::Node & n = m_state->graph.node(nodeId);
	m_edgesTableView->scrollTo(m_edgesTableModel->index(n.begin, 0));
}

void MainWindow::toggleNode(uint32_t nodeId) {
	MultiReaderSingleWriterLocker lck(m_state->enabledNodesLock, MultiReaderSingleWriterLocker::WRITE_LOCK);
	if (m_state->enabledNodes.count(nodeId)) {
		m_state->enabledNodes.erase(nodeId);
	}
	else {
		m_state->enabledNodes.insert(nodeId);
	}
	emit shownNodesChanged();
}

void MainWindow::toggleEdge(uint32_t edgeId) {
	MultiReaderSingleWriterLocker lck(m_state->enabledEdgesLock, MultiReaderSingleWriterLocker::WRITE_LOCK);
	if (m_state->enabledEdges.count(edgeId)) {
		m_state->enabledEdges.erase(edgeId);
	}
	else {
		m_state->enabledEdges.insert(edgeId);
	}
	emit shownEdgesChanged();
}

void MainWindow::routerConfigChanged() {
	int algoSelection = m_routerSelection->currentIndex();
	int accessSelection = m_accessType->currentIndex();
	
	if (algoSelection < 0 || accessSelection < 0) {
		return;
	}
	
	algoSelection = m_routerSelection->itemData(algoSelection).toInt();
	accessSelection = m_accessType->itemData(accessSelection).toInt();
	
	m_br.reroute(algoSelection, accessSelection);
}

}//end namespace