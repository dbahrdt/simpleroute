#include "MainWindow.h"
#include <iostream>
#include <QApplication>
#include <QFile>
#include <QMetaType>

Q_DECLARE_METATYPE(simpleroute::Graph::Route);

void help() {
	std::cout << "simpleroute [options] file.osm.pbf\n";
	std::cout << "\t-s\tspatial sort nodes\n";
	std::cout << "\t-x\tgrid bins in lat\n";
	std::cout << "\t-y\tgrid bins in lon\n";
	std::cout << "\t-c\tdo a self-check\n";
	std::cout << std::endl;
}

int main(int argc, char ** argv) {
	qRegisterMetaType<simpleroute::Graph::Route>();
	
	QApplication app(argc, argv);
	QStringList cmdline_args = QCoreApplication::arguments();
	
	simpleroute::Config cfg;
	bool doSelfCheck = false;

	for(uint32_t i(1), s(cmdline_args.size()); i < s; ++i) {
		if (cmdline_args.at(i) == "-c") {
			doSelfCheck = true;
		}
		else if(cmdline_args.at(i) == "-s") {
			cfg.doSpatialSort = true;
		}
		else if (cmdline_args.at(i) == "-x" && i+1 < s) {
			cfg.latCount = cmdline_args.at(i+1).toUInt();
			++i;
		}
		else if (cmdline_args.at(i) == "-y" && i+1 < s) {
			cfg.lonCount = cmdline_args.at(i+1).toUInt();
			++i;
		}
		else if(cmdline_args.at(i) == "--help" || cmdline_args.at(i) == "-h") {
			help();
			return 0;
		}
		else if (QFile::exists(cmdline_args.at(i))) {
			cfg.graphFileName = cmdline_args.at(i).toStdString();
			
		}
	}
	if (!cfg.graphFileName.size()) {
		help();
		return -1;
	}
	
	simpleroute::StatePtr state(new simpleroute::State(cfg));

	if (doSelfCheck) {
		std::cout << "Graph self-check: " << std::flush;
		if (state->graph.selfCheck()) {
			std::cout << "OK" << std::endl;
		}
		else {
			std::cout << "FAILED" << std::endl;
			return -1;
		}

		std::cout << "Grid self-check: " << std::flush;
		if (state->grid.selfCheck()) {
			std::cout << "OK" << std::endl;
		}
		else {
			std::cout << "FAILED" << std::endl;
			return -1;
		}
	}

	simpleroute::MainWindow mainWindow(state);
	mainWindow.show();
	return app.exec();
}
