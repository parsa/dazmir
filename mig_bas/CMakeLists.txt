
################################################################################
# Basic Migration
################################################################################
add_hpx_executable(mig_bas
  SOURCES
    prog.cpp
  COMPONENT_DEPENDENCIES iostreams
)

set_target_properties(mig_bas
  PROPERTIES FOLDER "Migration"
)

################################################################################
# Copy required HPX DLLs to bin directory
################################################################################
if(MSVC)
  # Copy HPX dlls
  string(REPLACE "/lib/cmake/HPX" "" HPX_ROOTPATH ${HPX_DIR})
  string(REPLACE "/" "\\" HPX_ROOTPATH ${HPX_ROOTPATH})
  add_custom_command(TARGET mig_bas PRE_BUILD
    COMMAND xcopy /D /Y ${HPX_ROOTPATH}\\$(Configuration)\\bin\\*.dll $(TargetDir)
    COMMENT "Copying files from HPX for $(Configuration) configuration")
endif()
