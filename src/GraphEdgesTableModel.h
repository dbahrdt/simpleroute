#ifndef SIMPLE_ROUTE_GRAPH_EDGES_TABLE_MODEL_H
#define SIMPLE_ROUTE_GRAPH_EDGES_TABLE_MODEL_H
#include <QAbstractTableModel>
#include "State.h"

namespace simpleroute {


class GraphEdgesTableModel: public QAbstractTableModel {
	Q_OBJECT
private:
	typedef enum { CN_SOURCE=0, CN_TARGET=1, CN_TYPE=2, CN_ACCESS=3,
					CN_LENGTH=4, CN_MAXSPEED=5, CN_SHOW=6, CN_COL_COUNT=CN_SHOW+1} ColNames;
private:
	StatePtr m_state;
	
public:
	GraphEdgesTableModel(QObject * parent, const StatePtr & state);
	virtual ~GraphEdgesTableModel();
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
	void toggleEdgeClicked(uint32_t edgeId);
	void sourceNodeClicked(uint32_t nodeId);
	void targetNodeClicked(uint32_t nodeId);
	void clearShownEdgesClicked();
};

}//end namespace simpleroute

#endif