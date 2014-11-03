cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

project(GLIDAR)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules/")

option(ENABLE_PUBSUB "Enable publish-subscribe with ZeroMQ" ON)

if (APPLE)
  include (UseDebugSymbols)

  set(GUI_TYPE MACOSX_BUNDLE)

  find_library(OpenGL_LIBRARY OpenGL)
  find_library(Cocoa_LIBRARY Cocoa)
  find_library(IOKit_LIBRARY IOKit)
  find_library(CoreVideo_LIBRARY CoreVideo)

  mark_as_advanced(Cocoa_LIBRARY IOKit_LIBRARY CoreVideo_LIBRARY)

  set(EXTRA_LIBS ${Cocoa_LIBRARY} ${OpenGL_LIBRARY} ${IOKit_LIBRARY} ${CoreVideo_LIBRARY})
else (APPLE)
  find_package(OpenGL REQUIRED)
  include_directories(${OpenGL_INCLUDE_DIRS})
  link_directories(${OpenGL_LIBRARY_DIRS})
  add_definitions(${OpenGL_DEFINITIONS})
endif (APPLE)

find_package(ASSIMP REQUIRED)
include_directories(${ASSIMP_INCLUDE_DIRS})
link_directories(${ASSIMP_LIBRARY_DIRS})
add_definitions(${ASSIMP_DEFINITIONS})

find_package(FLANN)
include_directories(${FLANN_INCLUDE_DIRS})

find_package(GLFW 3 REQUIRED)
include_directories(${GLFW_INCLUDE_DIRS})
link_directories(${GLFW_LIBRARY_DIRS})

if (ENABLE_PUBSUB)
  message(STATUS "Publish-subscribe is enabled")
  find_package(ZeroMQ)
  include_directories(${ZeroMQ_INCLUDE_DIR})

  if (ZEROMQ_FOUND)
    add_definitions(-DHAS_ZEROMQ)
  endif (ZEROMQ_FOUND)
else (ENABLE_PUBSUB)
  message(STATUS "Publish-subscribe is disabled")
endif (ENABLE_PUBSUB)

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
  pkg_search_module(GLEW REQUIRED glew)
  include_directories(${GLEW_INCLUDE_DIRS})
  link_directories(${GLEW_LIBRARY_DIRS})
else (PKG_CONFIG_FOUND)
  message(STATUS "Could not find pkg-config, falling back on FindGLEW.cmake (untested)")
  find_package(GLEW REQUIRED)
  include_directories(${GLEW_INCLUDE_DIRS})
  link_directories(${GLEW_LIBRARY_DIRS})
endif (PKG_CONFIG_FOUND)

# Eigen (required)
include_directories(SYSTEM ${EIGEN_INCLUDE_DIRS})
add_definitions(-DEIGEN_USE_NEW_STDVECTOR
                -DEIGEN_YES_I_KNOW_SPARSE_MODULE_IS_NOT_STABLE_YET)

find_package(PCL 1.7 REQUIRED)
include_directories(${PCL_INCLUDE_DIRS})
link_directories(${PCL_LIBRARY_DIRS})
add_definitions(${PCL_DEFINITIONS})


find_package(ImageMagick REQUIRED COMPONENTS Magick++)
include_directories(${ImageMagick_INCLUDE_DIRS})
link_directories(${ImageMagick_LIBRARY_DIRS})
add_definitions(${ImageMagick_DEFINITIONS})

add_definitions(-DMAGICKCORE_QUANTUM_DEPTH=16
                -DMAGICKCORE_HDRI_ENABLE=0)

if (ENABLE_PUBSUB)
  add_executable(
    glidar
    ${GUI_TYPE}
    src/main.cpp
    src/mesh.cpp
    src/subscribe.cpp
    src/publish.cpp
    src/gl_error.cpp
  )
else (ENABLE_PUBSUB)
  add_executable(
    glidar
    ${GUI_TYPE}
    src/main.cpp
    src/mesh.cpp
    src/gl_error.cpp
  )
endif(ENABLE_PUBSUB)

target_link_libraries(
    glidar
    ${OpenGL_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${GLFW_LIBRARIES}
    ${ASSIMP_LIBRARIES}
    ${PCL_LIBRARIES}
    ${EXTRA_LIBS}
    ${ZeroMQ_LIBRARIES}
    ${ImageMagick_LIBRARIES}
    ${FLANN_LIBRARIES}
)
