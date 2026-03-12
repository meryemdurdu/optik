cmake_minimum_required(VERSION 3.16)
project(optik LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenCV REQUIRED)

add_executable(optik
    src/main.cpp
    src/AnswerKey.cpp
    src/ROIDetector.cpp
    src/core/CornerFinder.cpp
    src/core/PerspectiveCorrector.cpp
    src/core/Logger.cpp
)

target_include_directories(optik PRIVATE src src/core)

target_link_libraries(optik PRIVATE ${OpenCV_LIBS})
