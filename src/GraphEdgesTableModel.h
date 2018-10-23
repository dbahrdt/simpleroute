#ifndef SIMPLE_ROUTE_GRAPH_EDGES_TABLE_MODEL_H
#define SIMPLE_ROUTE_GRAPH_EDGES_TABLE_MODEL_H
#include <QAbstractTableModel>
#include <QTableView>
#include "State.h"

namespace simpleroute {

class GraphEdgesTable: public QTableView {
	Q_OBJECT
public:
	explicit GraphEdgesTable(QWidget* parent = 0);
	virtual ~GraphEdgesTable() override {}
	virtual int sizeHintForColumn(int column) const;
	virtual int sizeHintForRow(int row) const override;
	virtual void setModel(QAbstractItemModel* model) override;
private:
	int m_sizeHintForColumn;
	int m_sizeHintForRow;
};

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
	virtual int rowCount(const QModelIndex&) const override;
	virtual int columnCount(const QModelIndex&) const override;
	virtual QVariant data(const QModelIndex & index, int role) const override;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	virtual Qt::ItemFlags flags(const QModelIndex& /*index*/) const override {
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