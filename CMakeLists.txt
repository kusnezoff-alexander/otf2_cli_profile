cmake_minimum_required (VERSION 3.10)

# Set the version number
project ( OTFProfiler VERSION 2.0.0 )

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)       # or 20, 23...

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fsanitize=address")

include_directories("${PROJECT_BINARY_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}/include/reader")
include_directories("${PROJECT_SOURCE_DIR}/include/output")
include_directories("${PROJECT_SOURCE_DIR}/include/analysis")

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake;${CMAKE_MODULE_PATH}")

add_compile_options(-Wno-error)

# === Custom cmake file
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/") 		# own `.cmake` files: eg `Format.cmake` for consistent formatting
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/module/") 	# add module files for external pkgs under `cmake/modules`
include(DefaultBuildType)
include(BatsTests)

# === Dependencies
include(ExternalProject)

option (USE_OTF "USE OTF library" OFF)
if (USE_OTF)
     find_package (OTF)
     set (HAVE_OPEN_TRACE_FORMAT OTF_FOUND)
endif()

option (USE_OTF2 "Use OTF2 library" ON)
if (USE_OTF2)
	find_package (OTF2)

	# if(NOT OTF2_FOUND)
	# 	message("Downloading OTF23.1 manually...")
	#
	# 	# Set where OTF2 should be built and installed
	# 	set(OTF2_PREFIX ${CMAKE_BINARY_DIR}/otf2-install)
	#
	# 	ExternalProject_Add(OTF2
	# 		URL https://perftools.pages.jsc.fz-juelich.de/cicd/otf2/tags/otf2-3.1/otf2-3.1.tar.gz
	# 		PREFIX ${CMAKE_BINARY_DIR}/otf2
	# 		CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=${OTF2_PREFIX} # CXXFLAGS="-Wno-deprecated-enum-enum-conversion"
	# 		BUILD_COMMAND make -j
	# 		INSTALL_COMMAND make install
	#
	# 		# TODO: prevent rebuilt on every compilation (the below doesnt work..)
	# 		# Set the directory where the file is downloaded and cached
	# 	  	DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/downloads
	# 		# These prevent re-downloading if already built
	# 	  	DOWNLOAD_NO_EXTRACT FALSE
	# 	  	UPDATE_DISCONNECTED TRUE
	# 		BUILD_ALWAYS OFF
	# 	)
	# endif()

    set (HAVE_OTF2 OTF2_FOUND)
    set (VERSION_OTF2_MAJOR ${OTF2_VERSION_MAJOR})
    set (VERSION_OTF2_MINOR ${OTF2_VERSION_MINOR})
endif()

option (USE_Cubelib "USE Cube package (library and GUI)" ON)
if (USE_Cubelib)
    find_package (Cubelib)
    set (HAVE_CUBE Cubelib_FOUND)
endif()

option (USE_MPI "Build MPI parallel version of OTF-Profiler" OFF)
if (USE_MPI)
    find_package(MPI REQUIRED)
    set (HAVE_MPI MPI_FOUND)
endif()

option (USE_JSON "USE JSON outputter (via rapidjson library)" ON)
if(USE_JSON)
    find_package(RapidJson)
    set(HAVE_JSON RapidJson_FOUND)
endif()

option (USE_DATA_OUT "export everything into json format" ON)
if(USE_DATA_OUT)
    find_package(RapidJson)
    set(HAVE_DATA_OUT RapidJson_FOUND)
endif()

# === Compilation-specific

set(SOURCE_FILES
    src/reader/tracereader.cpp
    src/data_tree.cpp
    src/otf-profiler.cpp
    src/definitions.cpp
	src/analysis/access_pattern_detection.cpp
)

if (HAVE_OPEN_TRACE_FORMAT AND USE_OTF)
    include_directories("${OTF_INCLUDE_DIRS}")
    list(APPEND SOURCE_FILES src/reader/OTFReader.cpp)
    list(APPEND EXTRA_LIBS "${OTF_LIBRARIES}")
endif()

if (HAVE_OTF2 AND USE_OTF2)
    include_directories("${OTF2_INCLUDE_DIRS}")
    list(APPEND SOURCE_FILES src/reader/OTF2Reader.cpp)
    list(APPEND EXTRA_LIBS "${OTF2_LIBRARIES}")
endif()

if (HAVE_CUBE AND USE_Cubelib)
    include_directories("${Cubelib_INCLUDE_DIRS}")
    list(APPEND SOURCE_FILES src/output/create_cube.cpp)
    list(APPEND EXTRA_LIBS "${Cubelib_LIBRARIES}")
endif()

if (HAVE_JSON AND USE_JSON)
    include_directories("${RapidJson_INCLUDE_DIRS}")
    list(APPEND SOURCE_FILES src/output/create_json.cpp)
endif ()


list(APPEND SOURCE_FILES src/output/create_dot.cpp)
list(APPEND SOURCE_FILES src/output/dot_writer.cpp)

if (HAVE_DATA_OUT AND USE_DATA_OUT)
    include_directories("${RapidJson_INCLUDE_DIRS}")
    list(APPEND SOURCE_FILES src/output/data_out.cpp)
endif()


if (HAVE_JSON)
    list(APPEND SOURCE_FILES src/reader/jsonreader.cpp)
endif()
# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/include/otf-profiler-config.h.in"
  "${PROJECT_BINARY_DIR}/otf-profiler-config.h"
)

# build sequential version of OTF-Profiler
add_executable (otf-profiler ${SOURCE_FILES})
# Requiring language standard C++ 11
target_compile_options(otf-profiler PRIVATE -Wno-error)
target_compile_features(otf-profiler PUBLIC cxx_std_11)
target_link_directories(otf-profiler PRIVATE ${OTF2_PREFIX}/lib)
target_link_libraries(otf-profiler ${EXTRA_LIBS} otf2)
target_include_directories(otf-profiler PRIVATE
	 ${PROJECT_SOURCE_DIR}/include
	${OTF2_PREFIX}/include
)

# add the install targets
install (TARGETS otf-profiler DESTINATION bin)

# build MPI parallel version of OTF-Profiler
if (HAVE_MPI AND USE_MPI)
    add_executable(otf-profiler-mpi ${SOURCE_FILES} src/reduce_data.cpp)
    target_compile_definitions(otf-profiler-mpi PUBLIC OTFPROFILER_MPI)
    target_compile_features(otf-profiler-mpi PUBLIC cxx_std_11)
    target_link_libraries (otf-profiler-mpi ${EXTRA_LIBS} ${MPI_CXX_LIBRARIES})
endif()

# Docs
option (BUILD_DOCS "Build Docs" ON)
if (BUILD_DOCS)
	add_subdirectory(docs)
endif()
