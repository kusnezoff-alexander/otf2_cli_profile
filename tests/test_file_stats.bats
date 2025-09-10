LOG_FILE=test_file_stats.log

function create_file_access_pattern_trace(){
	export SCOREP_ENABLE_PROFILING=false        # enabled by default
	export SCOREP_ENABLE_TRACING=true           # disabled by default
	export SCOREP_IO_POSIX=true
	# export SCOREP_EXPERIMENT_DIRECTORY=working_dir/scorep_trace_file_access_pattern

	# SETUP
	touch working_dir/contiguous.txt working_dir/strided.txt working_dir/random.txt
	: > working_dir/contiguous.txt # empty file
	: > working_dir/strided.txt
	: > working_dir/random.txt
	head -c 1000 /dev/urandom > working_dir/strided.txt # put some random bytes in here

	echo "......" >> $LOG_FILE
	# Compile & Run
	scorep --io=posix mpicc -o file_access_pattern.out file_access_pattern.c
	mpirun -n 2 ./file_access_pattern.out
	echo "END......" >> $LOG_FILE
}


@test "can run our script" {
	run create_file_access_pattern_trace
    # ./project.sh
	echo "OK"
}

