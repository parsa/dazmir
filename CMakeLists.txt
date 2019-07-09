cmake_minimum_required(VERSION 3.10)

project(dazmir CXX)

find_package(HPX REQUIRED)

add_subdirectory(1d_stencil)

if(MSVC)
  # Enable solution folders for MSVC
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)

  # Set up the startup project
  set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY
    VS_STARTUP_PROJECT 1d_stencil)
endif()
