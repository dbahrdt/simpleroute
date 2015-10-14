cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH
	${CMAKE_SOURCE_DIR}/cmake
)

project (simpleroute)
find_package(Qt4 REQUIRED)
find_package(Protobuf REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Marble REQUIRED)
# find_package(LIBRT)

include_directories(
	${QT_INCLUDES}
	${CMAKE_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/vendor/osmpbf
	${CMAKE_CURRENT_SOURCE_DIR}/vendor/osmpbf/osmpbf/include
)

set(MY_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall -Wextra -Wno-unknown-pragmas -Wstrict-overflow=4")
set(MY_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x -Wall -Wextra -Wno-unknown-pragmas -Wstrict-overflow=4")
set(DEBUG_FLAGS "${DEBUG_FLAGS}")#-Wconversion

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MY_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MY_CXX_FLAGS}")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${DEBUG_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${DEBUG_FLAGS}")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} ${DEBUG_FLAGS}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${DEBUG_FLAGS}")

add_subdirectory(vendor/osmpbf osmpbf)

SET(MY_LINK_LIBS
	${osmpbf_BINARY_DIR}/libosmpbf.a
	${MARBLE_LIBRARIES}
	${QT_QTCORE_LIBRARY}
	${QT_QTGUI_LIBRARY}
	${PROTOBUF_LIBRARIES}
	${ZLIB_LIBRARIES}
	${LIBRT_LIBRARIES}
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

qt4_wrap_cpp(SOURCES_MOC_CPP ${SOURCES_MOC_H})

# The executable itself.
add_executable(simpleroute ${SOURCES_CPP} ${SOURCES_MOC_CPP})
target_link_libraries(simpleroute ${MY_LINK_LIBS})
add_dependencies(simpleroute osmpbf)
