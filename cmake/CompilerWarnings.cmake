# ==============================================================================
# CompilerWarnings.cmake - Strict warning flags for MSVC, Clang, and GCC
# ==============================================================================

function(strata_set_compiler_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE
            /W4             # Baseline reasonable warnings
            /w14242         # Conversion: possible loss of data
            /w14254         # Conversion from larger to smaller type
            /w14263         # Member function does not override base
            /w14265         # Class has virtual functions but destructor is not virtual
            /w14287         # Unsigned/negative constant mismatch
            /we4289         # Loop control variable declared in loop-init is used outside
            /w14296         # Expression is always 'true' or always 'false'
            /w14311         # Pointer truncation
            /w14545         # Expression before comma evaluates to a function which is missing an argument list
            /w14546         # Function call before comma missing argument list
            /w14547         # Operator before comma has no effect
            /w14549         # Operator before comma has no effect
            /w14555         # Expression has no effect
            /w14619         # Pragma warning: there is no warning number
            /w14640         # Enable warning on thread unsafe static member initialization
            /w14826         # Conversion is sign-extended
            /w14905         # Wide string literal cast to LPSTR
            /w14906         # String literal cast to LPWSTR
            /w14928         # Illegal copy-initialization
            /permissive-    # Standards conformance mode
            /Zc:__cplusplus # Correct __cplusplus macro
        )
    else()
        target_compile_options(${target_name} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wnon-virtual-dtor
            -Wold-style-cast
            -Wcast-align
            -Wunused
            -Woverloaded-virtual
            -Wnull-dereference
            -Wdouble-promotion
            -Wformat=2
        )
    endif()
endfunction()
