cmake_minimum_required(VERSION 3.9)

set(CMAKE_MODULE_PATH
	${CMAKE_SOURCE_DIR}/cmake
)

project (simpleroute)
find_package(Protobuf REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Marble REQUIRED)

add_subdirectory(vendor/osmpbf osmpbf)

SET(MY_LINK_LIBS
	osmpbf
	${MARBLE_LIBRARIES}
	Qt5::Widgets
	Qt5::Gui
	${PROTOBUF_LIBRARIES}
	${ZLIB_LIBRARIES}
	${LIBRT_LIBRARIES}
)

set(MY_INCLUDE_DIRS
	${QT_INCLUDES}
	${MARBLE_INCLUDE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)

set(SOURCES_MOC_H
	src/MainWindow.h
	src/GraphEdgesTableModel.h
	src/GraphNodesTableModel.h
	src/MarbleMap.h
)

set(SOURCES_CPP
	src/MainWindow.cpp
	src/GraphNodesTableModel.cpp
	src/GraphEdgesTableModel.cpp
	src/Graph.cpp
	src/MarbleMap.cpp
	src/Grid.cpp
	src/MarbleMap.cpp
	src/Router.cpp
	src/util.cpp
	src/State.cpp
	src/main.cpp
)

qt5_wrap_cpp(SOURCES_MOC_CPP ${SOURCES_MOC_H})

# The executable itself.
add_executable(${PROJECT_NAME} ${SOURCES_CPP} ${SOURCES_MOC_CPP})
target_include_directories(${PROJECT_NAME} PRIVATE ${MY_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${MY_LINK_LIBS})
