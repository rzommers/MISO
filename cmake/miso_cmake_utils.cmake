function(add_public_headers_to_target target headers)
   set(HEADERS_FULL "")
   foreach(header ${headers})
      set(HEADERS_FULL
         ${HEADERS_FULL}
         "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${header}>"
         "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDE_DIR}/${header}>")
   endforeach()

   get_target_property(MISO_PUBLIC_HEADERS ${target} PUBLIC_HEADER)
   set(MISO_PUBLIC_HEADERS ${MISO_PUBLIC_HEADERS} ${HEADERS_FULL})
   set_target_properties(${target} PROPERTIES
      PUBLIC_HEADER "${MISO_PUBLIC_HEADERS}")
endfunction(add_public_headers_to_target)