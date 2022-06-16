project(WebboxManager)
cmake_minimum_required(VERSION 3.0)

message("Building Webbox Manager")

# not reasonable to expect VLC start on every host
if (CMAKE_CROSSCOMPILING)
    add_definitions(-DVLC_EXEC=1)
endif()

# lazy way to grap all sources
file(GLOB SOURCES *.c)

add_executable(WebboxManager ${SOURCES})

target_link_libraries(WebboxManager libvlc.so)

target_include_directories(WebboxManager PRIVATE /usr/include/libdrm/)
target_link_libraries(WebboxManager libgbm.so libEGL.so libGLESv2.so libdrm.so)
target_link_libraries(WebboxManager libgbm.so libwayland-client.so libwayland-egl.so)

#target_link_libraries(WebboxManager pthread.so)

# use local file on development host
if (CMAKE_CROSSCOMPILING)
    add_definitions(-DWEBBOX_LIB_PATH="/usr/lib/webbox-manager")
else()
    add_definitions(-DWEBBOX_LIB_PATH="..")
endif()

# debug print defines
get_cmake_property(_variableNames VARIABLES)
list (SORT _variableNames)
foreach (_variableName ${_variableNames})
    message(STATUS "${_variableName}=${${_variableName}}")
endforeach()
