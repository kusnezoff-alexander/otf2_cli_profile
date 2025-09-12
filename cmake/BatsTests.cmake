# Supplies command for running bats integration tests
find_program(BATS_EXECUTABLE bats)
if(NOT BATS_EXECUTABLE)
    message(FATAL_ERROR "Bats not found! Please install bats-core.")
endif()

find_program(SCOREP_EXECUTABLE scorep)
if(NOT SCOREP_EXECUTABLE)
	message(FATAL_ERROR "scorep not found which is needed to generate test traces.")
endif()

find_program(MPI_EXECUTABLE mpirun)
if(NOT MPI_EXECUTABLE)
	message(FATAL_ERROR "mpirun not found which is needed to compile test files.")
endif()

add_custom_target(run_tests
    COMMAND ${BATS_EXECUTABLE} --verbose-run --show-output-of-passing-tests --print-output-on-failure ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration
    COMMENT "Running Bats integration tests"
    VERBATIM
)
