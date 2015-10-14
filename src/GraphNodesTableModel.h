#ifndef SIMPLE_ROUTE_GRAPH_NODES_TABLE_MODES_H
#define SIMPLE_ROUTE_GRAPH_NODES_TABLE_MODES_H
#include <QAbstractTableModel>
#include "State.h"

namespace simpleroute {

class GraphNodesTableModel: public QAbstractTableModel {
	Q_OBJECT
private:
	typedef enum { CN_OSMID=0, CN_LAT=1, CN_LON=2, CN_EDGE_COUNT=3, CN_START_EDGE=4, CN_SHOW=5, CN_COL_COUNT=CN_SHOW+1} ColNames;
private:
	StatePtr m_state;
	
public:
	GraphNodesTableModel(QObject * parent, const StatePtr & state);
	virtual ~GraphNodesTableModel();
	virtual int rowCount(const QModelIndex&) const;
	virtual int columnCount(const QModelIndex&) const;
	virtual QVariant data(const QModelIndex & index, int role) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual Qt::ItemFlags flags(const QModelIndex& /*index*/) const {
		return (Qt::ItemIsEnabled);
	}
public Q_SLOTS:
	void doubleClicked(const QModelIndex&);
	void headerClicked(int);
	void resetData();
Q_SIGNALS:
	void nodeCoordinateClicked(uint32_t nodeId);
	void startEdgeClicked(uint32_t nodeId);
	void toggleNodeClicked(uint32_t nodeId);
	void clearShownNodesClicked();
};

}//end namespace simpleroute

#endif