#ifndef SIMPLE_ROUTE_MARBLE_MAP_H
#define SIMPLE_ROUTE_MARBLE_MAP_H
#include <marble/MarbleWidget.h>
#include <marble/LayerInterface.h>
#include <marble/GeoDataLineString.h>
#include <unordered_set>
#include "State.h"
#include "MultiReaderSingleWriterLock.h"

namespace simpleroute {

class MarbleMap : public Marble::MarbleWidget {
	Q_OBJECT
private:

	class MyBaseLayer: public Marble::LayerInterface {
	private:
		qreal m_zValue;
		QStringList m_renderPosition;
		StatePtr m_state;
	protected:
		const StatePtr & state() const { return m_state; }
	public:
		MyBaseLayer(const QStringList & renderPos, qreal zVal, const StatePtr & state);
		virtual ~MyBaseLayer() {}
		virtual QStringList renderPosition() const;
		virtual qreal zValue() const;
	};
	
	class MyLockableBaseLayer: public MyBaseLayer {
	private:
		MultiReaderSingleWriterLock m_lock;
	protected:
		inline MultiReaderSingleWriterLock & lock() { return m_lock;}
	public:
		MyLockableBaseLayer(const QStringList & renderPos, qreal zVal, const StatePtr & state) :
		MyBaseLayer(renderPos, zVal, state) {}
		virtual ~MyLockableBaseLayer() {}
	};
	
	class MyRouteLayer: public MyLockableBaseLayer {
	private:
		Marble::GeoDataLineString m_route;
		uint32_t m_startNodeId;
		uint32_t m_endNodeId;
		Marble::GeoDataCoordinates m_startCoordinate;
		Marble::GeoDataCoordinates m_endCoordinate;
	public:
		MyRouteLayer(const QStringList & renderPos, qreal zVal, const StatePtr & state);
		virtual ~MyRouteLayer() {}
		virtual bool render(Marble::GeoPainter *painter, Marble::ViewportParams * viewport, const QString & renderPos, Marble::GeoSceneLayer * layer);
		void setRoute(const Graph::Route & route);
		void setStartCoordinate(const Marble::GeoDataCoordinates & c) { m_startCoordinate = c; }
		void setEndCoordinate(const Marble::GeoDataCoordinates & c) { m_endCoordinate = c; }
		void clear();
	};
	
	class MyNodesLayer: public MyLockableBaseLayer {
	public:
		MyNodesLayer(const QStringList & renderPos, qreal zVal, const StatePtr & state);
		virtual ~MyNodesLayer() {}
		virtual bool render(Marble::GeoPainter *painter, Marble::ViewportParams * viewport, const QString & renderPos, Marble::GeoSceneLayer * layer);
	};
	
	class MyEdgesLayer: public MyLockableBaseLayer {
	public:
		MyEdgesLayer(const QStringList & renderPos, qreal zVal, const StatePtr & state);
		virtual ~MyEdgesLayer() {}
		virtual bool render(Marble::GeoPainter *painter, Marble::ViewportParams * viewport, const QString & renderPos, Marble::GeoSceneLayer * layer);
	};
	
	struct RouteInfo {
		double srcLat;
		double srcLon;
		double tgtLat;
		double tgtLon;
		RouteInfo() { clear(); }
		void clear();
		bool valid() const;
	};
	
private:
	StatePtr m_state;
	MyRouteLayer * m_routeLayer;
	MyNodesLayer * m_nodesLayer;
	MyEdgesLayer * m_edgesLayer;
	double m_lastMouseClickLat;
	double m_lastMouseClickLon;
	RouteInfo m_ri;
public:
	MarbleMap(QWidget * parent, const StatePtr & state);
	virtual ~MarbleMap();
public slots:
	void displayRoute(const Graph::Route & route);
	void shownEdgesChanged();
	void shownNodesChanged();
	void clearRoute();
	void zoomToNode(uint32_t nodeId);
private slots:
	void rmbRequested(int x, int y);
	void routeSourceSelected();
	void routeTargetSelected();
signals:
	void calculateRoute(double srcLat, double srcLon, double tgtLat, double tgtLon);
};


}//end namespace simpleroute

#endif