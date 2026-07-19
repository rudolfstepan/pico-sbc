add_library(libbf STATIC
    libbf.c
    cutils.c
)

target_include_directories(libbf PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_compile_definitions(libbf PRIVATE
    LIBBF_DISABLE_FFT
)

# Keep upstream code isolated from the project's warning policy.
if(MSVC)
    target_compile_options(libbf PRIVATE /W0)
else()
    target_compile_options(libbf PRIVATE -w)
endif()
