cmake_minimum_required(VERSION 3.20)
project(starlab_b3_flight LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(SQLite3 REQUIRED)

add_library(b3_environment STATIC
 src/env/Atmosphere.cpp src/env/Geodesy.cpp src/env/PinkNoise.cpp
 src/env/WindModel.cpp src/malzeme/VeriTabanimalzeme.cpp)
add_library(b3_aero_framework STATIC src/aero/extensions/AeroExtensionPipeline.cpp)
file(GLOB_RECURSE b3_datcom_sources CONFIGURE_DEPENDS src/datcom_equivalent/*.cpp)
add_library(b3_datcom STATIC ${b3_datcom_sources})
file(GLOB b3_datcom_integration_sources CONFIGURE_DEPENDS src/datcom_integration/*.cpp)
add_library(b3_datcom_integration STATIC ${b3_datcom_integration_sources})
add_executable(roketsim_cli src/cli/roketsim_cli.cpp)

foreach(target b3_environment b3_aero_framework b3_datcom b3_datcom_integration roketsim_cli)
 target_include_directories(${target} PUBLIC include third_party/eigen)
endforeach()
target_link_libraries(roketsim_cli PRIVATE b3_environment b3_aero_framework
 b3_datcom_integration b3_datcom SQLite3::SQLite3)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
 target_compile_options(b3_datcom PRIVATE -O2 -ffp-contract=off -fno-fast-math)
endif()
