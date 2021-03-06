message(STATUS "")
message(STATUS "Starting to configure ImageReconstructionTests..")
message(STATUS "")
enable_testing()

set(UNIT_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/SinogramCreatorToolsTest.cpp)
set(TEST_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/../SinogramCreatorTools.cpp)

#Configure Boost
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.50 REQUIRED COMPONENTS unit_test_framework)

if(NOT TARGET Boost::unit_test_framework)
    add_library(Boost::unit_test_framework IMPORTED INTERFACE)
    set_property(TARGET Boost::unit_test_framework PROPERTY
        INTERFACE_INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIR})
    set_property(TARGET Boost::unit_test_framework PROPERTY
        INTERFACE_LINK_LIBRARIES ${Boost_LIBRARIES})
endif()
#End of configuration of Boost

## Add custom target to create symlink from test dir to unitTestData
add_custom_target(link_target_imagereconstruction ALL
                  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/../../unitTestData ${CMAKE_CURRENT_BINARY_DIR}/unitTestData)

get_filename_component(TESTNAME ${UNIT_TEST_SOURCES} NAME_WE)
add_executable(${TESTNAME}.x EXCLUDE_FROM_ALL ${UNIT_TEST_SOURCES} ${TEST_SOURCE})
target_compile_options(${TESTNAME}.x PRIVATE -Wunused-parameter -Wall)
target_link_libraries(${TESTNAME}.x JPetFramework::JPetFramework Boost::unit_test_framework)
add_test(NAME ${TESTNAME}.x COMMAND ${TESTNAME}.x --log_level=error --log_format=XML --log_sink=${TESTNAME}.xml)
set_target_properties(${TESTNAME}.x PROPERTIES FOLDER tests)
add_dependencies(${TESTNAME}.x link_target_imagereconstruction)


add_custom_target(tests_imagereconstruction DEPENDS ${TESTNAME}.x)