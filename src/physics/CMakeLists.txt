set(MISO_PHYSICS_HEADERS
   common_outputs.hpp
   diag_mass_integ.hpp
   finite_element_dual.hpp
   finite_element_state.hpp
   finite_element_vector.hpp
   miso_input.hpp
   miso_integrator.hpp
   miso_load.hpp
   miso_nonlinearform.hpp
   mfem_common_integ.hpp
   pde_solver.hpp
   physics.hpp
)

target_sources(miso
   PRIVATE
      common_outputs.cpp
      diag_mass_integ.cpp
      finite_element_state.cpp
      finite_element_vector.cpp
      miso_input.cpp
      miso_integrator.cpp
      miso_nonlinearform.cpp
      mfem_common_integ.cpp
      pde_solver.cpp
      ${MISO_PHYSICS_HEADERS}
)

# get_target_property(MISO_PUBLIC_HEADERS miso PUBLIC_HEADER)
# set(MISO_PUBLIC_HEADERS ${MISO_PUBLIC_HEADERS} ${MISO_PHYSICS_HEADERS})
# set_target_properties(miso PROPERTIES
#    PUBLIC_HEADER "${MISO_PUBLIC_HEADERS}"
# )

add_public_headers_to_target(miso "${MISO_PHYSICS_HEADERS}")

target_include_directories(miso
   PUBLIC
     "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
     "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDE_DIR}>"
)

add_subdirectory(electromagnetics)
add_subdirectory(fluidflow)
add_subdirectory(thermal)
