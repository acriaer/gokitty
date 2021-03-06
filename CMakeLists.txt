cmake_minimum_required(VERSION 3.0)
project(gokitty)

include(CTest)
include(ExternalProject)
include(CMakeRC.cmake)

enable_testing()

# ==============================================================================
# names
# ==============================================================================
set(APP_NAME ${PROJECT_NAME})

# ==============================================================================
# compiler settings
# ==============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  add_compile_options(-fno-limit-debug-info)
endif()

include_directories(inc)
include_directories(${CMAKE_BINARY_DIR}/gen)
include_directories(${CMAKE_BINARY_DIR}/dependencies/include)

# ==============================================================================
# dependencies
# ==============================================================================

find_package(Threads)
find_package(Boost COMPONENTS system filesystem unit_test_framework REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

ExternalProject_Add(pugixml-dependency
  URL https://github.com/zeux/pugixml/releases/download/v1.9/pugixml-1.9.tar.gz
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/dependencies -DSPDLOG_BUILD_EXAMPLES=0 -DSPDLOG_BUILD_BENCH=0 -DSPDLOG_BUILD_TESTING=0 -DSPDLOG_BUILD_TESTING=0
)

ExternalProject_Add(spdlog-dependency
  URL https://github.com/gabime/spdlog/archive/v1.1.0.tar.gz
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/dependencies
)

ExternalProject_Add(sdl2-dependency
  URL https://github.com/libSDL2pp/libSDL2pp/archive/0.16.0.tar.gz
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/dependencies -DSDL2PP_WITH_MIXER=OFF -DSDL2PP_WITH_IMAGE=OFF -DSDL2PP_STATIC=ON
)

ExternalProject_Add(adept-dependency
  URL https://github.com/rjhogan/Adept-2/archive/master.tar.gz
  CONFIGURE_COMMAND cd ../adept-dependency/ && autoreconf -i && ./configure CXXFLAGS=-O2 --prefix=${CMAKE_BINARY_DIR}/dependencies
  BUILD_COMMAND make -C ../adept-dependency
  INSTALL_COMMAND make -C ../adept-dependency install
)

FIND_PACKAGE(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

link_directories(${CMAKE_BINARY_DIR}/dependencies/lib)

# ==============================================================================
# resources
# ==============================================================================
cmrc_add_resource_library(resources res/default_configuration.xml)


# ==============================================================================
# cmake targets
# ==============================================================================

set(SRCS_NOMAIN 
  src/log.cpp
  src/config.cpp
  src/model_element.cpp
  src/visualisation.cpp
  src/hinge_model.cpp
  src/util.cpp
  src/data_reader.cpp
  src/executor_racing.cpp
  src/executor_recording.cpp
  src/torcs_integration.cpp
  src/pid_controller.cpp

  inc/visualisation.h
  inc/model_element.h
  inc/config.h
  inc/exceptions.h
  inc/log.h
  inc/hinge_model.h
  inc/util.h
  inc/data_reader.h
  inc/executor.h
  inc/integration.h
  inc/pid_controller.h
  )

add_library (${APP_NAME} STATIC ${SRCS_NOMAIN})

target_link_libraries(${APP_NAME}
  ${Boost_LIBRARIES} pugixml SDL2pp ${SDL2_LIBRARIES} adept resources ${CMAKE_THREAD_LIBS_INIT}
)

add_executable(demo_app src/main.cpp)
target_link_libraries(demo_app ${APP_NAME})

add_dependencies(${APP_NAME} spdlog-dependency)
add_dependencies(${APP_NAME} pugixml-dependency)
add_dependencies(${APP_NAME} sdl2-dependency)
add_dependencies(${APP_NAME} adept-dependency)

# ==============================================================================
# tests
# ==============================================================================
file(GLOB TEST_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} tests/*.cpp)

foreach(testSrc ${TEST_SRCS})
  get_filename_component(testName ${testSrc} NAME_WE)
  set(testName "test_${testName}")
  add_executable(${testName} "${testSrc}")
  target_link_libraries(${testName} ${PROJECT_NAME})

  set_target_properties(${testName} PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_CURRENT_BINARY_DIR}/tests)

  add_test(NAME ${testName} 
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMAND ${CMAKE_CURRENT_BINARY_DIR}/tests/${testName} )
endforeach(testSrc)