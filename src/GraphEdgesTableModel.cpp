#include "GraphEdgesTableModel.h"
#include "assert.h"

namespace simpleroute {

GraphEdgesTableModel::GraphEdgesTableModel(QObject* parent, const StatePtr& state) :
QAbstractTableModel(parent),
m_state(state)
{}

GraphEdgesTableModel::~GraphEdgesTableModel() {}

int GraphEdgesTableModel::columnCount(const QModelIndex&) const {
	return CN_COL_COUNT;
}

int GraphEdgesTableModel::rowCount(const QModelIndex&) const {
	return m_state->graph.edgeCount();
}

QVariant GraphEdgesTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (role == Qt::DisplayRole) {
		if (orientation == Qt::Horizontal) {
			switch (section) {
			case (CN_SOURCE):
				return QVariant("Source");
			case (CN_TARGET):
				return QVariant("Target");
			case (CN_TYPE):
				return QVariant("Type");
			case (CN_ACCESS):
				return QVariant("Access");
			case (CN_LENGTH):
				return QVariant("Length");
			case (CN_MAXSPEED):
				return QVariant("Maxspeed");
			case (CN_SHOW):
				return QVariant("Show");
			default:
				return QVariant();
			}
		}
		else {
			return QVariant(section);
		}
	}
	return QVariant();
}

QVariant GraphEdgesTableModel::data(const QModelIndex& index, int role) const {
	if (role == Qt::DisplayRole) {

		int col = index.column();
		int row = index.row();
		
		assert(row < (int) m_state->graph.edgeCount());
		
		const Graph::Edge & e = m_state->graph.edge(row);
		
		switch (col) {
		case CN_SOURCE:
			return QVariant(e.source);
		case CN_TARGET:
			return QVariant(e.target);
		case CN_TYPE:
			return QVariant(QString::fromUtf8(Graph::Edge::edge_type_2_osm_highway_value[e.type]));
		case CN_ACCESS:
		{
			QString ats;
			if (e.access & Graph::Edge::AT_FOOT) {
				ats += "F";
			}
			if (e.access & Graph::Edge::AT_BIKE) {
				ats += "B";
			}
			if (e.access & Graph::Edge::AT_CAR) {
				ats += "C";
			}
			return QVariant(ats);
		}
		case CN_LENGTH: //Name-Column
			return QVariant(e.distance);
		case CN_MAXSPEED:
			return QVariant(e.speed);
		case CN_SHOW:
		{
			MultiReaderSingleWriterLocker lck(m_state->enabledEdgesLock, MultiReaderSingleWriterLocker::READ_LOCK);
			if (m_state->enabledEdges.count(row)) {
				return QVariant("Yes");
			}
			else {
				return QVariant("No");
			}
		}
		default:
			return QVariant();
		}
	}
	return QVariant();
}

void GraphEdgesTableModel::doubleClicked(const QModelIndex & index) {
	int column = index.column();
	int row = index.row();
		
	assert(row >= 0 && (unsigned int) row < m_state->graph.edgeCount());

	switch(column) {
	case CN_SHOW:
	{
		emit toggleEdgeClicked(row);
		break;
	}
	case CN_SOURCE:
	{
		uint32_t nodeId = m_state->graph.edge(row).source;
		emit sourceNodeClicked(nodeId);
		break;
	}
	case CN_TARGET:
	{
		uint32_t nodeId = m_state->graph.edge(row).target;
		emit targetNodeClicked(nodeId);
		break;
	}
	default:
		break;
	}
}

void GraphEdgesTableModel::headerClicked(int logicalindex) {
	switch(logicalindex) {
	case CN_SHOW:
		emit clearShownEdgesClicked();
		break;
	default:
		break;
	}
}

void GraphEdgesTableModel::resetData() {
	emit beginResetModel();
	emit endResetModel();
}

}//end namespace