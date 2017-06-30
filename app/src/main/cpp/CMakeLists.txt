
# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( elfhook

             SHARED
             main.cpp
             elf_common.cpp
             elf_module.cpp
             elf_hooker.cpp )

target_link_libraries( elfhook

                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib} )