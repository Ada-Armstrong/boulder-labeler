cmake_minimum_required(VERSION 2.8)
project( BoulderLabeler )

set (CMAKE_CXX_STANDARD 17)

find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

add_executable( BoulderLabeler main.cpp )
target_link_libraries( BoulderLabeler ${OpenCV_LIBS} )
