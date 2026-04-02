cmake_minimum_required(VERSION 3.20)

project(ssrag
    VERSION 0.1.0
    DESCRIPTION "Self-Sharpening RAG — retrieval that improves with scale"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(SSRAG_BUILD_TESTS "Build tests" ON)

# CatWhisper — IOT distance, sparse vectors, ANN index
add_subdirectory(catwhisper)

# SSRAG library
file(GLOB_RECURSE SSRAG_SOURCES "src/*.cpp")
list(FILTER SSRAG_SOURCES EXCLUDE REGEX "src/main\\.cpp$")
list(FILTER SSRAG_SOURCES EXCLUDE REGEX "src/ingest_string\\.cpp$")
list(FILTER SSRAG_SOURCES EXCLUDE REGEX "src/query_string\\.cpp$")

add_library(ssrag STATIC ${SSRAG_SOURCES})
add_library(ssrag::ssrag ALIAS ssrag)

target_include_directories(ssrag
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

find_package(Threads REQUIRED)

target_link_libraries(ssrag
    PUBLIC
        catwhisper::catwhisper
        Threads::Threads
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ssrag PRIVATE
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
        -Werror=return-type
        $<$<CONFIG:Release>:-march=znver4>
        $<$<CONFIG:Release>:-mtune=znver4>
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Release>:-ffast-math>
    )
endif()

# MCP server executable
add_executable(ssrag-mcp src/main.cpp)
target_link_libraries(ssrag-mcp PRIVATE ssrag)

# STRING ingestion tool
add_executable(ssrag-ingest-string src/ingest_string.cpp)
target_link_libraries(ssrag-ingest-string PRIVATE ssrag)

# Query tool
add_executable(ssrag-query src/query_string.cpp)
target_link_libraries(ssrag-query PRIVATE ssrag)

if(SSRAG_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
