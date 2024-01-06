# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)

# Declares the project name. The project name can be accessed via ${ PROJECT_NAME},
# Since this is the top level CMakeLists.txt, the project name is also accessible
# with ${CMAKE_PROJECT_NAME} (both CMake variables are in-sync within the top level
# build script scope).
project("bitmapscaler")

add_library(libyuv STATIC IMPORTED)
add_library(libhwy STATIC IMPORTED)

add_library(${CMAKE_PROJECT_NAME} SHARED
        HalfFloats.cpp JniExceptions.cpp
        Rgb1010102toF16.cpp
        CopyUnaligned.cpp
        XScaler.cpp
        RGBAlpha.cpp
        BitmapScaler.cpp)

target_include_directories("bitmapscaler" PRIVATE ${CMAKE_SOURCE_DIR}/bitmapscaler ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/libyuv)

set_target_properties(bitmapscaler libyuv PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/lib/${ANDROID_ABI}/libyuv.a)
set_target_properties(bitmapscaler libhwy PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/lib/${ANDROID_ABI}/libhwy.a)

add_library(cpufeatures STATIC ${ANDROID_NDK}/sources/android/cpufeatures/cpu-features.c)
target_include_directories(cpufeatures PUBLIC ${ANDROID_NDK}/sources/android/cpufeatures)
target_link_libraries(cpufeatures dl)
list(APPEND EXTRA_LIBS cpufeatures)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CheckCXXCompilerFlag)

if (ANDROID_ABI STREQUAL x86)
    target_compile_options(bitmapscaler PRIVATE "-msse2")

    if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")
    endif()
endif ()

if (ANDROID_ABI STREQUAL x86-64)
    set_target_properties(bitmapscaler PROPERTIES COMPILE_FLAGS "-march=i686")

    if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")
    endif()
endif ()


add_definitions(-DSTB_IMAGE_RESIZE_IMPLEMENTATION -DCMS_NO_REGISTER_KEYWORD -DHWY_COMPILE_ONLY_STATIC)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -ffast-math")
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DHWY_COMPILE_ONLY_STATIC)
endif ()

# Specifies libraries CMake should link to your target library. You
# can link libraries from various origins, such as libraries defined in this
# build script, prebuilt third-party libraries, or Android system libraries.
target_link_libraries(${CMAKE_PROJECT_NAME}
        # List libraries link to the target library
        android
        cpufeatures
        libhwy
        libyuv
        log
        -ljnigraphics)