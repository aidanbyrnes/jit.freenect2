cmake_minimum_required(VERSION 4.0)

string(REGEX REPLACE "(.*)/" "" THIS_FOLDER_NAME "${CMAKE_CURRENT_SOURCE_DIR}")
project(${THIS_FOLDER_NAME})

# Architectures
set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures for macOS" FORCE)

# Add external
set(C74_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/externals")
add_subdirectory(source/jit.freenect2)

# Set RPATH
set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
set_target_properties(jit.freenect2 PROPERTIES
	INSTALL_RPATH "@loader_path/../../../../support"
)

# Add libfreenect2
OPTION(BUILD_SHARED_LIBS "Build shared (ON) or static (OFF) libraries" OFF)
OPTION(BUILD_EXAMPLES "Build examples" OFF)
OPTION(ENABLE_CXX11 "Enable C++11 support" ON)
add_subdirectory(source/libfreenect2)

# Set output path
set_target_properties(freenect2 PROPERTIES
	LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/support"
	LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/support"
)

# Link libraries manually
target_link_libraries(jit.freenect2 PRIVATE freenect2)
