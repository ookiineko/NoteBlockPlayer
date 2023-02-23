find_package(SndFile REQUIRED)
find_package(Portaudio REQUIRED)

add_executable(mcnbp
               main.c)
target_include_directories(mcnbp PRIVATE
                           "${SndFile_INCLUDE_DIR}"
                           "${PORTAUDIO_INCLUDE_DIRS}")
target_compile_definitions(mcnbp PRIVATE
                           "${PORTAUDIO_DEFINITIONS}")
target_compile_options(mcnbp PRIVATE
                       -Wall -Wextra -fPIE)
target_link_libraries(mcnbp "${SndFile_LIBRARIES}"
                            "${PORTAUDIO_LIBRARIES}")
