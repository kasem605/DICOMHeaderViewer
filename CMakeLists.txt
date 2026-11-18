
cmake_minimum_required(VERSION 3.25)

set(CMAKE_TOOLCHAIN_FILE
    "C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING ""
)

project(DICOMHeaderViewer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_PREFIX_PATH
    "C:/Qt/6.12.0/msvc2022_64"
)

set(DCMTK_DIR
    "C:/dev/vcpkg/installed/x64-windows/share/dcmtk"
)

find_package(Qt6 6.12 REQUIRED COMPONENTS Widgets)
find_package(DCMTK CONFIG REQUIRED)

set(CMAKE_AUTOMOC ON)

add_executable(DICOMHeaderViewer
    src/main.cpp
    src/DicomHeaderReader.h
    src/DicomHeaderReader.cpp
    src/DicomHeaderEntry.h
    src/DicomImageView.h
    src/DicomImageView.cpp
)

target_link_libraries(DICOMHeaderViewer
    PRIVATE
        Qt6::Widgets
        DCMTK::DCMTK
)

# Qt runtime deployment
add_custom_command(TARGET DICOMHeaderViewer POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "$<TARGET_FILE:Qt6::Core>"
        "$<TARGET_FILE_DIR:DICOMHeaderViewer>"
)
