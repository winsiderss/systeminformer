include("${CMAKE_CURRENT_LIST_DIR}/Default.cmake")

set_config_specific_property("OUTPUT_DIRECTORY" "${CMAKE_SOURCE_DIR}$<$<NOT:$<STREQUAL:${CMAKE_VS_PLATFORM_NAME},Win32>>:/${CMAKE_VS_PLATFORM_NAME}>/${PROPS_CONFIG}")

if(MSVC)
    create_property_reader("DEFAULT_CXX_EXCEPTION_HANDLING")
    create_property_reader("DEFAULT_CXX_DEBUG_INFORMATION_FORMAT")

    set_target_properties("${PROPS_TARGET}" PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    set_config_specific_property("DEFAULT_CXX_EXCEPTION_HANDLING" "/EHsc")
    set_config_specific_property("DEFAULT_CXX_DEBUG_INFORMATION_FORMAT" "/Zi")
endif()
