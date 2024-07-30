cmake_minimum_required(VERSION 3.6)
project(Testovoe)

set(CMAKE_CXX_STANDARD 14)
set(SOURCE_FILES main.cpp BankSystem.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/rapidjson-master/include)

add_executable(testovoe ${SOURCE_FILES})

target_link_libraries(testovoe)


