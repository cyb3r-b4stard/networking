cmake_minimum_required(VERSION 3.10)
project(networking C CXX)

set(CMAKE_CXX_FLAGS -pthread)
set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost) 

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS}) 
    add_executable(networking src/main.cpp)
    target_link_libraries(networking ${Boost_LIBRARIES})
endif()
