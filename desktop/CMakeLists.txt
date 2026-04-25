cmake_minimum_required(VERSION 3.20)
project(ymp3 VERSION 0.4.3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

if(MSVC)
    add_compile_options(/O1 /Os /GL)
    add_link_options(/LTCG /OPT:REF /OPT:ICF)
endif()

add_executable(ymp3 WIN32 main.cpp)

target_include_directories(ymp3 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/webview2/build/native/include
)

target_link_directories(ymp3 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/webview2/build/native/x64
)

target_link_libraries(ymp3 PRIVATE
    WebView2LoaderStatic.lib
    shell32
    ole32
    oleaut32
    uuid
)
