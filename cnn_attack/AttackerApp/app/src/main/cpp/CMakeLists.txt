
# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.22.1)

# Declares and names the project.

project("p_a_opencl")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
        p_a_opencl

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
        p_a_opencl.cpp
        )

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                        p_a_opencl

                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib}
                        )

#add_link_options("-rpath=/system/vendor/lib64/libOpenCL.so")
#string(REPLACE "--no-undefined" "--unresolved-symbols=ignore-all" CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS})
#string(REPLACE "--no-undefined" "--unresolved-symbols=ignore-all" CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS})
#set(CMAKE_EXE_LINKER_FLAGS    "-Wl,--unresolved-symbols=ignore-all -Wl,--no-as-needed C:\\Users\\Frederik\\Desktop\\libOpenCL.so ${CMAKE_EXE_LINKER_FLAGS}")
#set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--unresolved-symbols=ignore-all -Wl,--no-as-needed C:\\Users\\Frederik\\Desktop\\libOpenCL.so ${CMAKE_SHARED_LINKER_FLAGS}")

include_directories(
        include
)

find_library(EGL-lib EGL)
target_link_libraries(p_a_opencl ${EGL-lib})

find_library(GLESv3-lib GLESv3)
target_link_libraries(p_a_opencl ${GLESv3-lib})

if(FALSE)
# Add Qualcomm libOpenCL.so
add_library( OpenCL
        SHARED
        IMPORTED )
set_target_properties( # Specifies the target library.
        OpenCL

        # Specifies the parameter you want to define.
        PROPERTIES IMPORTED_LOCATION

        # Provides the path to the library you want to import.
        C:\\Users\\Frederik\\Desktop\\libOpenCL.so )

target_link_libraries(
        p_a_opencl
        OpenCL
)

# Add libcutils.so
add_library( cutils
        SHARED
        IMPORTED )
set_target_properties( # Specifies the target library.
        cutils

        # Specifies the parameter you want to define.
        PROPERTIES IMPORTED_LOCATION

        # Provides the path to the library you want to import.
        C:\\Users\\Frederik\\Desktop\\libcutils.so )

target_link_libraries(
        p_a_opencl
        cutils
)

# Add libvndksupport.so
add_library( vndksupport
        SHARED
        IMPORTED )
set_target_properties( # Specifies the target library.
        vndksupport

        # Specifies the parameter you want to define.
        PROPERTIES IMPORTED_LOCATION

        # Provides the path to the library you want to import.
        C:\\Users\\Frederik\\Desktop\\libvndksupport.so )

target_link_libraries(
        p_a_opencl
        vndksupport
)

# Add libc++.so
add_library( cpp
        SHARED
        IMPORTED )
set_target_properties( # Specifies the target library.
        cpp

        # Specifies the parameter you want to define.
        PROPERTIES IMPORTED_LOCATION

        # Provides the path to the library you want to import.
        C:\\Users\\Frederik\\Desktop\\libc++.so )

target_link_libraries(
        p_a_opencl
        cpp
)

# Add libbase.so
add_library( base
        SHARED
        IMPORTED )
set_target_properties( # Specifies the target library.
        base

        # Specifies the parameter you want to define.
        PROPERTIES IMPORTED_LOCATION

        # Provides the path to the library you want to import.
        C:\\Users\\Frederik\\Desktop\\libbase.so )

target_link_libraries(
        p_a_opencl
        base
)

# Add libdl_android.so
add_library( dl_android
        SHARED
        IMPORTED )
set_target_properties( # Specifies the target library.
        dl_android

        # Specifies the parameter you want to define.
        PROPERTIES IMPORTED_LOCATION

        # Provides the path to the library you want to import.
        C:\\Users\\Frederik\\Desktop\\libdl_android.so )

target_link_libraries(
        p_a_opencl
        dl_android
)
endif()