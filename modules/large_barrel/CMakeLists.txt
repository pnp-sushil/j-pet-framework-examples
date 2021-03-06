set(projectName JPetRecoImageTools)

######################################################################
### this section should not need to be modified for a new project
######################################################################
project(${projectName} CXX) # using only C++

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/${projectName}/JPetRecoImageTools.cpp)
set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/${projectName}/JPetFilterCosine.h
            ${CMAKE_CURRENT_SOURCE_DIR}/${projectName}/JPetFilterHamming.h
            ${CMAKE_CURRENT_SOURCE_DIR}/${projectName}/JPetFilterInterface.h
            ${CMAKE_CURRENT_SOURCE_DIR}/${projectName}/JPetFilterNone.h
            ${CMAKE_CURRENT_SOURCE_DIR}/${projectName}/JPetFilterRamLak.h
            ${CMAKE_CURRENT_SOURCE_DIR}/${projectName}/JPetFilterRidgelet.h
            ${CMAKE_CURRENT_SOURCE_DIR}/${projectName}/JPetFilterSheppLogan.h
            ${CMAKE_CURRENT_SOURCE_DIR}/${projectName}/JPetRecoImageTools.h)
######################################################################
### Configure FFTW(based on: https://github.com/egpbos/findFFTW)
######################################################################
configure_file(downloadFindFFTW.cmake.in findFFTW-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/findFFTW-download )
if(result)
    message(FATAL_ERROR "CMake step for findFFTW failed: ${result}")
    else()
    message("CMake step for findFFTW completed (${result}).")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/findFFTW-download )
if(result)
    message(FATAL_ERROR "Build step for findFFTW failed: ${result}")
endif()

set(findFFTW_DIR ${CMAKE_CURRENT_BINARY_DIR}/findFFTW-src)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${findFFTW_DIR}")

find_package(FFTW REQUIRED COMPONENTS DOUBLE_LIB)

add_library(${projectName} SHARED ${SOURCES} ${HEADERS})
target_include_directories(${projectName} PUBLIC
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${projectName}>
                           $<BUILD_INTERFACE:${FFTW_INCLUDE_DIRS}>)
target_link_libraries(${projectName} PRIVATE JPetFramework::JPetFramework ${FFTW_LIBRARIES})
