cmake_minimum_required(VERSION 3.5)
project(transcribe)
find_package(AWSSDK REQUIRED COMPONENTS s3 transcribe )
set(CMAKE_CXX_STANDARD 11)
include_directories( ${PROJECT_SOURCE_DIR}/include)
#add_executable(transcribe "transcribe.cpp")
add_library(transcribe SHARED "transcribe.cpp") # 
# list all deps for static linking
target_link_libraries(transcribe ${AWSSDK_LINK_LIBRARIES})
target_compile_options(transcribe PRIVATE "-Wall" "-Werror")

set_target_properties(transcribe PROPERTIES 
                      PUBLIC_HEADER ${CMAKE_SOURCE_DIR}/include/transcribe.h
                      PREFIX lib
                      SUFFIX .so
                      VERSION ${CMAKE_PROJECT_VERSION}
                      SOVERSION ${CMAKE_PROJECT_VERSION_MAJOR})

install(TARGETS transcribe 
        EXPORT MyMathTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        PUBLIC_HEADER DESTINATION include
        )
