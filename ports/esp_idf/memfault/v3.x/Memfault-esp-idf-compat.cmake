if(DEFINED IDF_VER)

# Strip the leading v so we can do a CMAKE version compare
STRING(REGEX REPLACE "^v" "" MEMFAULT_ESP_IDF_VER ${IDF_VER})

# In v3.3, espcoredump was moved to its own component
if(${MEMFAULT_ESP_IDF_VER} VERSION_GREATER_EQUAL "3.3")
set(MEMFAULT_ESP_IDF_VERSION_SPECIFIC_REQUIRES espcoredump)
endif()

endif()

function(mflt_esp32_component_get_target var component_dir)
    if(COMMAND component_get_target)
        # esp-idf v3.3 adds a prefix to the target and introduced component_get_target():
        component_get_target(local_var ${component_dir})
        set(${var} ${local_var} PARENT_SCOPE)
    else()
        # Pre-v3.3, the target is named after the directory:
        set(${var} ${component_dir} PARENT_SCOPE)
    endif()
endfunction()
