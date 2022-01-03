# Set the project name
project(dnslib)

# Add a library with the above sources
add_library(${PROJECT_NAME}
		buffer.cpp
		message.cpp
		qs.cpp
		rr.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)
