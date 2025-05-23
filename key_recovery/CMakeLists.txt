add_compile_options(-Wall -Wextra -Wpedantic)
add_compile_options(-Wredundant-decls -Wshadow -Wpointer-arith)
add_compile_options(-fomit-frame-pointer)
add_compile_options(-march=native -mtune=native -O3 -fwrapv)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fopenmp")

add_library(utils utils.c)

add_executable(key_recovery key_recovery.c)
target_link_libraries(key_recovery utils)

