LOG_FILE=test_file_stats.log
# export SCOREP_EXPERIMENT_DIRECTORY=${TEST_OUTPUT_DIR}/scorep_trace_file_access_pattern
export SCOREP_ENABLE_PROFILING=false        # enabled by default
export SCOREP_ENABLE_TRACING=true           # disabled by default
export SCOREP_IO_POSIX=true
export BATS_TEST_DIRNAME="$BATS_TEST_DIRNAME"

function create_file_access_pattern_trace(){
	# SETUP
	touch $TEST_OUTPUT_DIR/contiguous.txt $TEST_OUTPUT_DIR/strided.txt $TEST_OUTPUT_DIR/random.txt
	: > $TEST_OUTPUT_DIR/contiguous.txt # empty file
	: > $TEST_OUTPUT_DIR/strided.txt
	: > $TEST_OUTPUT_DIR/random.txt
	head -c 1000 /dev/urandom > $TEST_OUTPUT_DIR/strided.txt # put some random bytes in here

	echo "......" >> $LOG_DIR/$LOG_FILE
	# Compile & Run
	scorep --io=posix mpicc -o $TEST_OUTPUT_DIR/file_access_pattern.out $PROGRAMS_DIR/file_access_pattern.c
	$TEST_OUTPUT_DIR/file_access_pattern.out
	echo "END......" >> $LOG_DIR/$LOG_FILE
}

function posix_hello_world(){
	# SETUP
	touch $TEST_OUTPUT_DIR/posix_hello_world.txt

	echo "......" >> $LOG_DIR/$LOG_FILE
	# Compile & Run
	sleep 2
	scorep --io=posix mpic++ -o $TEST_OUTPUT_DIR/posix_hello_world.out $PROGRAMS_DIR/posix_hello_world.cpp
	$TEST_OUTPUT_DIR/posix_hello_world.out
	echo "END......" >> $LOG_DIR/$LOG_FILE
}

function mpi_hello_world(){
	# SETUP
	touch $TEST_OUTPUT_DIR/mpi_hello_world.txt

	echo "......" >> $LOG_DIR/$LOG_FILE
	# Compile & Run
	sleep 2
	scorep --io=posix mpic++ -o $TEST_OUTPUT_DIR/mpi_hello_world.out $PROGRAMS_DIR/mpi_hello_world.c
	mpiexec -np 2 $TEST_OUTPUT_DIR/mpi_hello_world.out
	echo "END......" >> $LOG_DIR/$LOG_FILE
}

@test "can run our script" {
	# run create_file_access_pattern_trace
	# # run Profiler
	# ../build/otf-profiler --json -i ${SCOREP_EXPERIMENT_DIRECTORY}/traces.otf2 -o ${TEST_OUTPUT_DIR}/results
	# rm -rf $SCOREP_EXPERIMENT_DIRECTORY
	#
	# # get determined access patterns
	# # For each access-pattern there is one location that performs it (see `file_access_pattern.c`)
	# access_pattern_contiguous=$( jq -r '.Files[] | select(.FileName | contains("contiguous.txt")) | .["Ticks spent per Access Pattern"].["AccessPattern::CONTIGUOUS"]' ${TEST_OUTPUT_DIR}/results.json)
	# [ "$access_pattern_contiguous" > "0" ]
	# access_pattern_contiguous=$( jq -r '.Files[] | select(.FileName | contains("contiguous.txt")) | .["Ticks spent per Access Pattern"].["AccessPattern::STRIDED"]' ${TEST_OUTPUT_DIR}/results.json)
	# [ "$access_pattern_contiguous" = "0" ]
	# access_pattern_contiguous=$( jq -r '.Files[] | select(.FileName | contains("contiguous.txt")) | .["Ticks spent per Access Pattern"].["AccessPattern::RANDOM"]' ${TEST_OUTPUT_DIR}/results.json)
	# [ "$access_pattern_contiguous" = "0" ]
	#
	#
	# access_pattern_strided=$( jq -r '.Files[] | select(.FileName | contains("strided.txt")) | .["Ticks spent per Access Pattern"].["AccessPattern::STRIDED"]' ${TEST_OUTPUT_DIR}/results.json)
	# [ "$access_pattern_strided" > "0" ]
	# access_pattern_strided=$( jq -r '.Files[] | select(.FileName | contains("strided.txt")) | .["Ticks spent per Access Pattern"].["AccessPattern::CONTIGUOUS"]' ${TEST_OUTPUT_DIR}/results.json)
	# [ "$access_pattern_strided" = "0" ]
	# access_pattern_strided=$( jq -r '.Files[] | select(.FileName | contains("strided.txt")) | .["Ticks spent per Access Pattern"].["AccessPattern::RANDOM"]' ${TEST_OUTPUT_DIR}/results.json)
	# [ "$access_pattern_strided" = "0" ]
	#
	# access_pattern_random=$( jq -r '.Files[] | select(.FileName | contains("random.txt")) | .["Ticks spent per Access Pattern"].["AccessPattern::RANDOM"]' ${TEST_OUTPUT_DIR}/results.json)
	# [ "$access_pattern_random" > "0" ]
	# access_pattern_random=$( jq -r '.Files[] | select(.FileName | contains("random.txt")) | .["Ticks spent per Access Pattern"].["AccessPattern::STRIDED"]' ${TEST_OUTPUT_DIR}/results.json)
	# [ "$access_pattern_random" = "0" ]
	# access_pattern_random=$( jq -r '.Files[] | select(.FileName | contains("random.txt")) | .["Ticks spent per Access Pattern"].["AccessPattern::CONTIGUOUS"]' ${TEST_OUTPUT_DIR}/results.json)
	# [ "$access_pattern_random" = "0" ]
	#
	# run posix_hello_world
	# ../build/otf-profiler --json -i ${SCOREP_EXPERIMENT_DIRECTORY}/traces.otf2 -o ${TEST_OUTPUT_DIR}/results_posix
	# rm -rf $SCOREP_EXPERIMENT_DIRECTORY
	# access_pattern_none=$( jq -r '.Files[] | select(.FileName | contains("posix_hello_world.txt")) | .["Ticks spent per Access Pattern"].["AccessPattern::NONE"]' ${TEST_OUTPUT_DIR}/results_posix.json)
	# [ "$access_pattern_none" > "0" ]

	run mpi_hello_world
	# ../build/otf-profiler --json -i ${SCOREP_EXPERIMENT_DIRECTORY}/traces.otf2 -o ${TEST_OUTPUT_DIR}/results_mpi
	../build/otf-profiler --json -i scorep*/traces.otf2 -o ${TEST_OUTPUT_DIR}/results_mpi
	# rm -rf $SCOREP_EXPERIMENT_DIRECTORY
}

