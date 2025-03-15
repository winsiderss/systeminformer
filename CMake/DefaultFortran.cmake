include("${CMAKE_CURRENT_LIST_DIR}/Default.cmake")

set_config_specific_property("OUTPUT_DIRECTORY" "${CMAKE_CURRENT_SOURCE_DIR}$<$<NOT:$<STREQUAL:${CMAKE_VS_PLATFORM_NAME},Win32>>:/${CMAKE_VS_PLATFORM_NAME}>/${PROPS_CONFIG}")

get_target_property(${PROPS_TARGET}_BINARY_DIR ${PROPS_TARGET} BINARY_DIR)
set(DEFAULT_FORTRAN_MODULES_DIR "${${PROPS_TARGET}_BINARY_DIR}/${PROPS_TARGET}.Modules.dir")
set_target_properties(${PROPS_TARGET} PROPERTIES Fortran_MODULE_DIRECTORY ${DEFAULT_FORTRAN_MODULES_DIR})

if(${CMAKE_GENERATOR} MATCHES "Visual Studio")
    # Hack for visual studio generator (https://gitlab.kitware.com/cmake/cmake/issues/19552)
    add_custom_command(TARGET ${PROPS_TARGET} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_PROPERTY:${PROPS_TARGET},Fortran_MODULE_DIRECTORY>/${CMAKE_CFG_INTDIR})
endif()