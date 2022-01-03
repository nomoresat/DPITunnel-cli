# Set the project name
project(RawSocket)

add_library(${PROJECT_NAME} CheckSum.cpp)
target_include_directories(${PROJECT_NAME}
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)
