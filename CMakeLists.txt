# CMakeList.txt : CMake project for AudioGawd, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()


set(portaudio_DIR "C:/Users/Antonis/source/AudioGawd/AudioGawd/vcpkg_installed/x64-windows/share/portaudio")
set(opus_DIR "C:/Users/Antonis/source/AudioGawd/AudioGawd/vcpkg_installed/x64-windows/share/opus")

project ("AudioGawd")
find_package(portaudio CONFIG REQUIRED)
find_package(opus CONFIG REQUIRED)
# Add source to this project's executable.
add_executable (AudioGawd "AudioGawd.cpp" "constants.h")
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET AudioGawd PROPERTY CXX_STANDARD 20)
endif()

target_link_libraries(AudioGawd PRIVATE portaudio)
target_link_libraries(AudioGawd PRIVATE ${CMAKE_SOURCE_DIR}/build/vcpkg_installed/x64-windows/lib/opus.lib)


# TODO: Add tests and install targets if needed.
