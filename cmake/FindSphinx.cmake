include(FindPackageHandleStandardArgs)

find_package(Doxygen REQUIRED) # to pull in docstrings from src-files

# We are likely to find Sphinx near the Python interpreter
find_package(Python3 COMPONENTS Interpreter REQUIRED)
if(PYTHONINTERP_FOUND)
    get_filename_component(_PYTHON_DIR "${PYTHON_EXECUTABLE}" DIRECTORY)
    set(
        _PYTHON_PATHS
        "${_PYTHON_DIR}"
        "${_PYTHON_DIR}/bin"
        "${_PYTHON_DIR}/Scripts")
endif()

find_program(
	SPHINX_BUILD
    NAMES sphinx-build sphinx-build.exe
    HINTS ${_PYTHON_PATHS})
mark_as_advanced(SPHINX_BUILD)

find_package_handle_standard_args(Sphinx DEFAULT_MSG SPHINX_BUILD)

# Check if sphinx python module is available
set(VENV_DIR "${CMAKE_BINARY_DIR}/.venv")
message(STATUS "Sphinx not found, creating virtual environment and installing...")
execute_process(COMMAND ${Python3_EXECUTABLE} -m venv ${VENV_DIR})
execute_process(COMMAND ${VENV_DIR}/bin/pip install --quiet --upgrade pip)
execute_process(COMMAND ${VENV_DIR}/bin/pip install --quiet sphinx breathe myst-parser)
