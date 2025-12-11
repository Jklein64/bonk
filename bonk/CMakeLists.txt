cmake_minimum_required(VERSION 3.12...3.29)

project( Bonk )

find_package( CGAL REQUIRED COMPONENTS )
find_package( Boost REQUIRED )
find_package( Eigen3 REQUIRED )

set(SPECTRA_DIR "/usr/lib/spectra/include/Spectra")
set(NODE_ADDON_API_DIR "./node_modules/node-addon-api")

add_library(${PROJECT_NAME} SHARED
  src/addon.cpp
  src/tet.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_JS_INC}
  ${NODE_ADDON_API_DIR}
  ${EIGEN3_INCLUDE_DIR}
  ${SPECTRA_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  CGAL::CGAL
  ${CMAKE_JS_LIB}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
  NAPI_VERSION=8
  NAPI_DISABLE_CPP_EXCEPTIONS
)

set_target_properties(${PROJECT_NAME} PROPERTIES
  PREFIX ""
  SUFFIX ".node"
  CXX_STANDARD 20
  CXX_STANDARD_REQIURED ON
)