
foreach(TARGET IN LISTS EXECUTABLES)
    target_include_directories(${TARGET}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_sources(${TARGET}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/io_buffer.c
            ${CMAKE_CURRENT_SOURCE_DIR}/src/circular_buffer.c
    )
endforeach()
