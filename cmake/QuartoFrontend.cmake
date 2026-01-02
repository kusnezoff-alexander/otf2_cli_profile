find_program(QUARTO_EXECUTABLE quarto)
if(NOT QUARTO_EXECUTABLE)
    message(FATAL_ERROR "Quarto not found! Please install quarto.")
endif()

add_custom_target(visual
	COMMAND ${QUARTO_EXECUTABLE} preview output.qmd
    COMMENT "Running Quarto Visualization"
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/scripts
    VERBATIM
)
