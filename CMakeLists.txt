cmake_minimum_required(VERSION 2.8)

project( video-pass-through )

find_package( OpenCV REQUIRED )
find_package(Boost 1.63 COMPONENTS system filesystem regex REQUIRED)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# include_directories( ${OpenCV_INCLUDE_DIRS} )

INCLUDE_DIRECTORIES(
    ${OPENCV_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable( video-pass-through video-pass-through.cpp )
target_link_libraries( video-pass-through ${OpenCV_LIBS} )
target_link_libraries( video-pass-through ${Boost_LIBRARIES} )
