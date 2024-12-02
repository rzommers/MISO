# Creates executables for each file in the SANDBOX_SRCS list
# Accompaning options files must be named the same as the .cpp file,
#   but with _options.json as the end.
# For example, steady_vortex.cpp goes with steady_vortex_options.json
function(create_sandbox source_list)
   foreach(X ${source_list})
      add_executable("${X}.bin" "${X}.cpp")
      add_dependencies(sandbox "${X}.bin")
      target_link_libraries("${X}.bin" PRIVATE miso)
      target_include_directories("${X}.bin" PRIVATE "${CMAKE_SOURCE_DIR}")
      configure_file("${X}_options.json" "${X}_options.json" COPYONLY)
      configure_file("miso_options.json" "miso_options.json" COPYONLY)
   endforeach()
endfunction(create_sandbox)

set(SANDBOX_SRCS
   # air-gap
   steady_vortex
   steady_vortex_adjoint
   unsteady_vortex
   viscous_shock
   airfoil_steady
   airfoil_chaotic
   airfoil_flow_control
   forced_box
   navier_stokes3D_mms
   mpi_cci
   freestream_box
   # magnetostatic_box
   # magnetostatic_motor
   # magnetostatic_wire
   navier_stokes_mms
   # thermal_square
   # joule_wire
   # joule_box
   # mesh_move
   surface_distance
   # joule_wire
)

create_sandbox("${SANDBOX_SRCS}")
