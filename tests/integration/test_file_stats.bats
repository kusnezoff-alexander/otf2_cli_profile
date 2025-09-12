LOG_FILE=test_file_stats.log
export SCOREP_EXPERIMENT_DIRECTORY=${TEST_OUTPUT_DIR}/scorep_trace_file_access_pattern2

function create_file_access_pattern_trace(){
	export SCOREP_ENABLE_PROFILING=false        # enabled by default
	export SCOREP_ENABLE_TRACING=true           # disabled by default
	export SCOREP_IO_POSIX=true
	export BATS_TEST_DIRNAME="$BATS_TEST_DIRNAME"

	# SETUP
	touch $TEST_OUTPUT_DIR/contiguous.txt $TEST_OUTPUT_DIR/strided.txt $TEST_OUTPUT_DIR/random.txt
	: > $TEST_OUTPUT_DIR/contiguous.txt # empty file
	: > $TEST_OUTPUT_DIR/strided.txt
	: > $TEST_OUTPUT_DIR/random.txt
	head -c 1000 /dev/urandom > $TEST_OUTPUT_DIR/strided.txt # put some random bytes in here

	echo "......" >> $LOG_DIR/$LOG_FILE
	# Compile & Run
	scorep --io=posix mpicc -o $TEST_OUTPUT_DIR/file_access_pattern.out $PROGRAMS_DIR/file_access_pattern.c
	mpirun -n 2 $TEST_OUTPUT_DIR/file_access_pattern.out
	echo "END......" >> $LOG_DIR/$LOG_FILE
}


@test "can run our script" {
	run create_file_access_pattern_trace
	# run Profiler
	../build/otf-profiler --json -i ${SCOREP_EXPERIMENT_DIRECTORY}/traces.otf2 -o ${TEST_OUTPUT_DIR}/results

	# get determined access patterns
	access_pattern_contiguous=$( jq -r '.Files[] | select(.FileName | contains("contiguous.txt")) | .["Access Pattern"]' ${TEST_OUTPUT_DIR}/results.json)
	access_pattern_strided=$( jq -r '.Files[] | select(.FileName | contains("strided.txt")) | .["Access Pattern"]' ${TEST_OUTPUT_DIR}/results.json)
	access_pattern_random=$( jq -r '.Files[] | select(.FileName | contains("random.txt")) | .["Access Pattern"]' ${TEST_OUTPUT_DIR}/results.json)
	echo "Contiguous?: ${access_pattern_contiguous}"
	echo "Strided?: ${access_pattern_strided}"
	echo "Random?: ${access_pattern_random}"
	# [ "$access_pattern_contiguous" = "contiguous" ]
	# [ "$access_pattern_strided" = "contiguous" ]
	# [ "$access_pattern_random" = "random" ]
	echo "OK"
}

