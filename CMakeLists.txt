cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
project(sitemap CXX)

find_package(Boost 1.81 REQUIRED COMPONENTS url program_options)

add_executable(${PROJECT_NAME} sitemap.cpp sitemap.h)
target_include_directories(${PROJECT_NAME} PRIVATE .)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_11)
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::url Boost::program_options)

add_subdirectory(deps/http)
add_subdirectory(deps/parser)

target_link_libraries(${PROJECT_NAME} PRIVATE httplib)
target_link_libraries(${PROJECT_NAME} PRIVATE htmlparser)