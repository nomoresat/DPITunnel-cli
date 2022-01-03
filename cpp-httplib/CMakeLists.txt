# Set the project name
project(cpp-httplib)

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME}
	INTERFACE
	${PROJECT_SOURCE_DIR}/include
)
