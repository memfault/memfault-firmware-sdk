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

function(mflt_esp32_add_component_dependencies depender_tgt dependency_dir dep_type)
    mflt_esp32_component_get_target(dependency_tgt ${dependency_dir})
    add_component_dependencies(${depender_tgt} ${dependency_tgt} ${dep_type})
endfunction()
