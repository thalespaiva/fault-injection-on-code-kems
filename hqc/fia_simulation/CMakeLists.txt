cmake_minimum_required(VERSION 3.21)

file(GLOB BENCH_SRCS
        "${CMAKE_CURRENT_SOURCE_DIR}/fia_simulation.c"
        # "${CMAKE_CURRENT_SOURCE_DIR}/gen_hw_s_plot_data.c"
)

foreach(variant IN LISTS VARIANTS)
    string(REPLACE "-" "_" safe_var ${variant})

    foreach(src IN LISTS BENCH_SRCS)
        get_filename_component(name ${src} NAME_WE)
        set(exe "${name}_${safe_var}")

        add_executable(${exe} ${src})
        target_link_libraries(${exe} PRIVATE
                ${safe_var}_${HQC_ARCH}
                fips202
        )
        target_compile_definitions(${exe} PRIVATE
                $<$<STREQUAL:${HQC_ARCH},x86_64>:AVX2_MODE>
                $<$<NOT:$<STREQUAL:${HQC_ARCH},x86_64>>:REF_MODE>
        )
        target_include_directories(${exe} PRIVATE
                ${CMAKE_SOURCE_DIR}/tests/bench
                ${CMAKE_SOURCE_DIR}/src/common
                ${CMAKE_SOURCE_DIR}/src/${HQC_ARCH}/${variant}
                ${CMAKE_SOURCE_DIR}/lib/fips202
                ${CMAKE_SOURCE_DIR}/src/
        )

    endforeach()
endforeach()
