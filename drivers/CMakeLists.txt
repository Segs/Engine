

foreach(tgt ${global_targets})
    OPTION(OPTION_${tgt}_DISABLE_RENDERING "Useful for some tests or a server build" OFF)
    OPTION(OPTION_${tgt}_ENABLE_MIDI "Support for midi access" OFF)

    add_library(${tgt}_drivers STATIC )

    set(drivers_sources)

    if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        include(alsa/sources.cmake)
        include(pulseaudio/sources.cmake)
    elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
        add_subdirectory(wasapi)
    elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
        add_subdirectory(coreaudio)
    endif()
    if(MSVC)
        target_compile_definitions(${tgt}_drivers PRIVATE NOMINMAX)
    endif()
    # OS drivers
    include(unix/sources.cmake)
    include(windows/sources.cmake)
    # Sounds drivers

    target_compile_definitions(${tgt}_drivers PRIVATE GLAD_ENABLED)


    list(APPEND drivers_sources "register_driver_types.cpp")

    target_link_libraries(${tgt}_drivers PRIVATE glad)

    if(OPTION_${tgt}_ENABLE_MIDI)
        if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
            add_subdirectory(alsamidi)
        elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
            add_subdirectory(coremidi)
        elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
            add_subdirectory(winmidi)
        endif()
    endif()


    if(NOT OPTION_${tgt}_DISABLE_RENDERING)
        target_include_directories(${tgt}_drivers PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/..)
        target_include_directories(${tgt}_drivers PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
        include(gles3/sources.cmake)
    else()
        add_subdirectory(dummy)
    endif()

    target_link_libraries(${tgt}_drivers PRIVATE ${tgt}_core)
    target_link_libraries(${tgt}_drivers PRIVATE gles3_shaders)

    target_sources(${tgt}_drivers PRIVATE ${drivers_sources})
if(USE_UNITY_BUILDS)
    set_target_properties(${tgt}_drivers PROPERTIES UNITY_BUILD ON)
    set_target_properties(${tgt}_drivers PROPERTIES UNITY_BUILD_BATCH_SIZE ${DEFAULT_UNITY_BATCH_SIZE})
endif()

    set_common_target_properties(${tgt}_drivers)
    INSTALL (
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DESTINATION include/SegsEngine
        FILES_MATCHING PATTERN "*.h"
    )

endforeach()

