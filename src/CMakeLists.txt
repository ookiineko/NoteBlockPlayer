find_package(Sndfile REQUIRED)
find_package(Portaudio REQUIRED)

add_executable(mcnbp
               main.c)
target_include_directories(mcnbp PRIVATE
                           "${SNDFILE_INCLUDE_DIR}"
                           "${PORTAUDIO_INCLUDE_DIRS}")
target_compile_definitions(mcnbp PRIVATE
                           "${PORTAUDIO_DEFINITIONS}")
target_compile_options(mcnbp PRIVATE
                       -Wall -Wextra -fPIE)
target_link_libraries(mcnbp "${SNDFILE_LIBRARIES}"
                            "${PORTAUDIO_LIBRARIES}")
