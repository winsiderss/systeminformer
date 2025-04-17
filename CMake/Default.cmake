################################################################################
# Command for variable_watch. This command issues error message, if a variable
# is changed. If variable PROPERTY_READER_GUARD_DISABLED is TRUE nothing happens
#     variable_watch(<variable> property_reader_guard)
################################################################################
function(property_reader_guard VARIABLE ACCESS VALUE CURRENT_LIST_FILE STACK)
    if("${PROPERTY_READER_GUARD_DISABLED}")
        return()
    endif()

    if("${ACCESS}" STREQUAL "MODIFIED_ACCESS")
        message(FATAL_ERROR
            " Variable ${VARIABLE} is not supposed to be changed.\n"
            " It is used only for reading target property ${VARIABLE}.\n"
            " Use\n"
            "     set_target_properties(\"<target>\" PROPERTIES \"${VARIABLE}\" \"<value>\")\n"
            " or\n"
            "     set_target_properties(\"<target>\" PROPERTIES \"${VARIABLE}_<CONFIG>\" \"<value>\")\n"
            " instead.\n")
    endif()
endfunction()

################################################################################
# Create variable <name> with generator expression that expands to value of
# target property <name>_<CONFIG>. If property is empty or not set then property
# <name> is used instead. Variable <name> has watcher property_reader_guard that
# doesn't allow to edit it.
#     create_property_reader(<name>)
# Input:
#     name - Name of watched property and output variable
################################################################################
function(create_property_reader NAME)
    set(PROPERTY_READER_GUARD_DISABLED TRUE)
    set(CONFIG_VALUE "$<TARGET_GENEX_EVAL:${PROPS_TARGET},$<TARGET_PROPERTY:${PROPS_TARGET},${NAME}_$<UPPER_CASE:$<CONFIG>>>>")
    set(IS_CONFIG_VALUE_EMPTY "$<STREQUAL:${CONFIG_VALUE},>")
    set(GENERAL_VALUE "$<TARGET_GENEX_EVAL:${PROPS_TARGET},$<TARGET_PROPERTY:${PROPS_TARGET},${NAME}>>")
    set("${NAME}" "$<IF:${IS_CONFIG_VALUE_EMPTY},${GENERAL_VALUE},${CONFIG_VALUE}>" PARENT_SCOPE)
    variable_watch("${NAME}" property_reader_guard)
endfunction()

################################################################################
# Set property $<name>_${PROPS_CONFIG_U} of ${PROPS_TARGET} to <value>
#     set_config_specific_property(<name> <value>)
# Input:
#     name  - Prefix of property name
#     value - New value
################################################################################
function(set_config_specific_property NAME VALUE)
    set_target_properties("${PROPS_TARGET}" PROPERTIES "${NAME}_${PROPS_CONFIG_U}" "${VALUE}")
endfunction()

################################################################################

create_property_reader("TARGET_NAME")
create_property_reader("OUTPUT_DIRECTORY")

set_config_specific_property("TARGET_NAME" "${PROPS_TARGET}")
set_config_specific_property("OUTPUT_NAME" "${TARGET_NAME}")
set_config_specific_property("ARCHIVE_OUTPUT_NAME" "${TARGET_NAME}")
set_config_specific_property("LIBRARY_OUTPUT_NAME" "${TARGET_NAME}")
set_config_specific_property("RUNTIME_OUTPUT_NAME" "${TARGET_NAME}")

set_config_specific_property("ARCHIVE_OUTPUT_DIRECTORY" "${OUTPUT_DIRECTORY}")
set_config_specific_property("LIBRARY_OUTPUT_DIRECTORY" "${OUTPUT_DIRECTORY}")
set_config_specific_property("RUNTIME_OUTPUT_DIRECTORY" "${OUTPUT_DIRECTORY}")