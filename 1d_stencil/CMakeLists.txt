add_hpx_executable(1d_stencil
  SOURCES prog.cpp
  HEADERS print_time_results.hpp
  COMPONENT_DEPENDENCIES iostreams
)

set_target_properties(1d_stencil
  PROPERTIES FOLDER "Distributed Heat Solver"
)

if(MSVC)
  # Copy HPX dlls
  string(REPLACE "/lib/cmake/HPX" "" HPX_ROOTPATH ${HPX_DIR})
  string(REPLACE "/" "\\" HPX_ROOTPATH ${HPX_ROOTPATH})
  add_custom_command(TARGET 1d_stencil PRE_BUILD
    COMMAND xcopy /D /Y ${HPX_ROOTPATH}\\$(Configuration)\\bin\\*.dll $(TargetDir)
    COMMENT "Copying files from HPX for $(Configuration) configuration")
endif()
