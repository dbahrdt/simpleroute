#include "GraphNodesTableModel.h"
#include "assert.h"

namespace simpleroute {

GraphNodesTableModel::GraphNodesTableModel(QObject* parent, const StatePtr& state) :
QAbstractTableModel(parent),
m_state(state)
{}

GraphNodesTableModel::~GraphNodesTableModel() {}

int GraphNodesTableModel::columnCount(const QModelIndex&) const {
	return CN_COL_COUNT;
}

int GraphNodesTableModel::rowCount(const QModelIndex&) const {
	return std::min<int>(m_state->graph.nodeCount(), 1000*1000);
}

QVariant GraphNodesTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (role == Qt::DisplayRole) {
		if (orientation == Qt::Horizontal) {
			switch (section) {
			case (CN_OSMID):
				return QVariant("Osm id");
			case (CN_LAT):
				return QVariant("Latitude");
			case (CN_LON):
				return QVariant("Longitude");
			case (CN_EDGE_COUNT):
				return QVariant("Edge count");
			case (CN_START_EDGE):
				return QVariant("Start-Edge");
			case (CN_SHOW):
				return QVariant("Show");
			default:
				return QVariant();
			}
		}
		else {
			return QVariant();
		}
	}
	return QVariant();
}

QVariant GraphNodesTableModel::data(const QModelIndex& index, int role) const {
	if (role == Qt::DisplayRole) {

		int col = index.column();
		int row = index.row();
		
		assert(row < (int) m_state->graph.nodeCount());
		
		const Graph::Node & n = m_state->graph.node(row);
		const Graph::NodeInfo & ni = m_state->graph.nodeInfo(row);
		
		switch (col) {
		case CN_OSMID:
			return QVariant(QString::number(ni.osmId));
		case CN_LAT:
			return QVariant(ni.lat);
		case CN_LON:
			return QVariant(ni.lon);
		case CN_EDGE_COUNT:
			return QVariant(n.edgeCount());
		case CN_START_EDGE:
			return QVariant(n.end);
		case CN_SHOW:
		{
			MultiReaderSingleWriterLocker lck(m_state->enabledNodesLock, MultiReaderSingleWriterLocker::READ_LOCK);
			if (m_state->enabledNodes.count(row)) {
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

void GraphNodesTableModel::resetData() {
	emit beginResetModel();
	emit endResetModel();
}

void GraphNodesTableModel::doubleClicked(const QModelIndex & index) {
	int column = index.column();
	int row = index.row();
		
	assert(row >= 0 && (unsigned int) row < m_state->graph.nodeCount());

	switch (column) {
	case CN_LAT:
	case CN_LON:
	{
		emit nodeCoordinateClicked(row);
		break;
	}
	case CN_START_EDGE:
	{
		emit startEdgeClicked(row);
		break;
	}
	case CN_SHOW:
	{
		emit toggleNodeClicked(row);
		break;
	}
	default:
		break;
	}
}

void GraphNodesTableModel::headerClicked(int logicalindex) {
	switch(logicalindex) {
	case CN_SHOW:
		emit clearShownNodesClicked();
		break;
	default:
		break;
	}
}

}//end namespace