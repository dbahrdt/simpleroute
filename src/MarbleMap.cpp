#include "MarbleMap.h"
#include <marble/GeoPainter.h>
#include <marble/MarbleWidgetPopupMenu.h>
#include <marble/MarbleWidgetInputHandler.h>
#include <QAction>

namespace simpleroute {

MarbleMap::MyBaseLayer::MyBaseLayer(const QStringList& renderPos, qreal zVal, const StatePtr& state) :
m_zValue(zVal),
m_renderPosition(renderPos),
m_state(state)
{}

QStringList MarbleMap::MyBaseLayer::renderPosition() const {
	return m_renderPosition;
}

qreal MarbleMap::MyBaseLayer::zValue() const {
    return m_zValue;
}

MarbleMap::MyNodesLayer::MyNodesLayer(const QStringList& renderPos, qreal zVal, const StatePtr& state):
MyLockableBaseLayer(renderPos, zVal, state)
{}

bool MarbleMap::MyNodesLayer::render(Marble::GeoPainter* painter, Marble::ViewportParams*, const QString&, Marble::GeoSceneLayer*) {
	MultiReaderSingleWriterLocker lck(lock(), MultiReaderSingleWriterLocker::READ_LOCK);
	MultiReaderSingleWriterLocker nodeLock(state()->enabledNodesLock, MultiReaderSingleWriterLocker::READ_LOCK);
	
	painter->setPen(Qt::green);
	painter->setBrush(Qt::BrushStyle::SolidPattern);

	for(uint32_t nodeId : state()->enabledNodes) {
		const Graph::NodeInfo & ni = state()->graph.nodeInfo(nodeId);
		Marble::GeoDataCoordinates gp(ni.lon, ni.lat, 0.0, Marble::GeoDataCoordinates::Degree);
		painter->drawEllipse(gp, 10, 10);
		painter->drawText(gp, QString::number(nodeId));
	}
	return true;
}


MarbleMap::MyEdgesLayer::MyEdgesLayer(const QStringList& renderPos, qreal zVal, const StatePtr& state) :
MyLockableBaseLayer(renderPos, zVal, state)
{}

bool MarbleMap::MyEdgesLayer::render(Marble::GeoPainter* painter, Marble::ViewportParams* /*viewport*/, const QString& /*renderPos*/, Marble::GeoSceneLayer* /*layer*/) {
	MultiReaderSingleWriterLocker lck(lock(), MultiReaderSingleWriterLocker::READ_LOCK);
	MultiReaderSingleWriterLocker edgeLock(state()->enabledEdgesLock, MultiReaderSingleWriterLocker::READ_LOCK);
	painter->setPen(QPen(QBrush(Qt::red, Qt::BrushStyle::SolidPattern), 3));
	painter->setBrush(Qt::BrushStyle::SolidPattern);
	Marble::GeoDataLineString l;
	for(uint32_t edgeId : state()->enabledEdges) {
		l.clear();
		const Graph::Edge & e = state()->graph.edge(edgeId);
		const Graph::NodeInfo & srcNi = state()->graph.nodeInfo(e.source);
		const Graph::NodeInfo & tgtNi = state()->graph.nodeInfo(e.target);
		
		l.append(Marble::GeoDataCoordinates(srcNi.lon, srcNi.lat, 0.0, Marble::GeoDataCoordinates::Degree));
		l.append(Marble::GeoDataCoordinates(tgtNi.lon, tgtNi.lat, 0.0, Marble::GeoDataCoordinates::Degree));
		painter->drawPolyline(l);
	}
	return true;
}


MarbleMap::MyRouteLayer::MyRouteLayer(const QStringList& renderPos, qreal zVal, const StatePtr& state) :
MyLockableBaseLayer(renderPos, zVal, state),
m_startNodeId(0xFFFFFFFF),
m_endNodeId(0xFFFFFFFF)
{}

void MarbleMap::MyRouteLayer::setRoute(const Graph::Route & route) {
	MultiReaderSingleWriterLocker lck(lock(), MultiReaderSingleWriterLocker::WRITE_LOCK);
	m_route.clear();
	for(uint32_t nodeRef : route.nodes) {
		const Graph::NodeInfo & ni = state()->graph.nodeInfo(nodeRef);
		m_route.append(Marble::GeoDataCoordinates(ni.lon, ni.lat, 0.0, Marble::GeoDataCoordinates::Degree));
	}
	if (m_route.size() >= 2) {
		m_startNodeId = route.nodes.front();
		m_endNodeId = route.nodes.back();
	}
}

void MarbleMap::MyRouteLayer::clear() {
	MultiReaderSingleWriterLocker lck(lock(), MultiReaderSingleWriterLocker::WRITE_LOCK);
	m_route.clear();
	m_startNodeId = 0xFFFFFFFF;
	m_endNodeId = 0xFFFFFFFF;
}

bool MarbleMap::MyRouteLayer::render(Marble::GeoPainter* painter, Marble::ViewportParams* /*viewport*/, const QString& /*renderPos*/, Marble::GeoSceneLayer* /*layer*/) {
	MultiReaderSingleWriterLocker lck(lock(), MultiReaderSingleWriterLocker::READ_LOCK);
	painter->setPen(QPen(QBrush(Qt::blue, Qt::BrushStyle::SolidPattern), 3));
	painter->setBrush(Qt::BrushStyle::SolidPattern);
	painter->drawPolyline(m_route);
	if (m_route.size() >= 2) {
		painter->drawEllipse(m_route.first(), 10, 10);
		painter->drawText(m_route.first(), QString::number(m_startNodeId));
		painter->drawEllipse(m_route.last(), 10, 10);
		painter->drawText(m_route.last(), QString::number(m_endNodeId));
	}
	
	painter->setPen(QPen(QBrush(Qt::yellow, Qt::BrushStyle::SolidPattern), 3));
	painter->drawEllipse(m_startCoordinate, 30, 30);
	painter->drawText(m_startCoordinate, "Start");
	painter->drawEllipse(m_endCoordinate, 30, 30);
	painter->drawText(m_endCoordinate, "End");
	
	return true;
}

void MarbleMap::RouteInfo::clear() {
	srcLat = srcLon = tgtLat = tgtLon = std::numeric_limits<double>::max();
}

bool MarbleMap::RouteInfo::valid() const {
	return (srcLat != std::numeric_limits<double>::max() && tgtLat != std::numeric_limits<double>::max());
}

MarbleMap::MarbleMap(QWidget * parent, const StatePtr& state) :
MarbleWidget(parent),
m_state(state)
{
	m_routeLayer = new MyRouteLayer({"HOVERS_ABOVE_SURFACE"}, 0.0, state);
	m_nodesLayer = new MyNodesLayer({"HOVERS_ABOVE_SURFACE"}, 0.0, state);
	m_edgesLayer = new MyEdgesLayer({"HOVERS_ABOVE_SURFACE"}, 0.0, state);
	
	QAction * routeSrcME = new QAction("Set as origin for routing", this);
	QAction * routeTgtME = new QAction("Set as destination for routing", this);
	
	addLayer(m_routeLayer);
	addLayer(m_nodesLayer);
	addLayer(m_edgesLayer);
	
	popupMenu()->addAction(Qt::MouseButton::RightButton, routeSrcME);
	popupMenu()->addAction(Qt::MouseButton::RightButton, routeTgtME);
	
	//get mouse clicks
	connect(this->inputHandler(), SIGNAL(rmbRequest(int,int)), this, SLOT(rmbRequested(int,int)));
	
	connect(routeSrcME, SIGNAL(triggered(bool)), this, SLOT(routeSourceSelected()));
	connect(routeTgtME, SIGNAL(triggered(bool)), this, SLOT(routeTargetSelected()));
}

MarbleMap::~MarbleMap() {
	removeLayer(m_routeLayer);
	removeLayer(m_nodesLayer);
	removeLayer(m_edgesLayer);
	delete m_routeLayer;
	delete m_nodesLayer;
	delete m_edgesLayer;
}

void MarbleMap::shownNodesChanged() {
	this->update();
}

void MarbleMap::shownEdgesChanged() {
	this->update();
}

void MarbleMap::clearRoute() {
	m_routeLayer->clear();
	m_ri.clear();
}

void MarbleMap::zoomToNode(uint32_t nodeId) {
	const Graph::NodeInfo & ni = m_state->graph.nodeInfo(nodeId);
	Marble::GeoDataCoordinates geo(ni.lon, ni.lat, 100.0, Marble::GeoDataCoordinates::Degree);
	centerOn(geo);
	setZoom(100000);
}

void MarbleMap::rmbRequested(int x, int y) {
	this->geoCoordinates(x, y, m_lastMouseClickLon, m_lastMouseClickLat, Marble::GeoDataCoordinates::Degree);
}

void MarbleMap::routeSourceSelected() {
	m_ri.srcLat = m_lastMouseClickLat;
	m_ri.srcLon = m_lastMouseClickLon;
	m_routeLayer->setStartCoordinate(Marble::GeoDataCoordinates(m_ri.srcLon, m_ri.srcLat, 0.0, Marble::GeoDataCoordinates::Degree));
	this->update();
	if (m_ri.valid()) {
		emit calculateRoute(m_ri.srcLat, m_ri.srcLon, m_ri.tgtLat, m_ri.tgtLon);
	}
}

void MarbleMap::routeTargetSelected() {
	m_ri.tgtLat = m_lastMouseClickLat;
	m_ri.tgtLon = m_lastMouseClickLon;
	m_routeLayer->setEndCoordinate(Marble::GeoDataCoordinates(m_ri.tgtLon, m_ri.tgtLat, 0.0, Marble::GeoDataCoordinates::Degree));
	this->update();
	if (m_ri.valid()) {
		emit calculateRoute(m_ri.srcLat, m_ri.srcLon, m_ri.tgtLat, m_ri.tgtLon);
	}
}

void MarbleMap::displayRoute(const Graph::Route & route) {
	m_routeLayer->setRoute(route);
	this->update();
}


}//end namespace simpleroute