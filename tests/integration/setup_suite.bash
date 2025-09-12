LOG_FILE=test.log
export LOG_DIR=${BATS_TEST_DIRNAME}/log
export PROGRAMS_DIR=${BATS_TEST_DIRNAME}/programs
export TEST_OUTPUT_DIR=${BATS_TEST_DIRNAME}/out

function setup_suite(){
	# clean output from previous run
	rm -rf $LOG_DIR/*
	rm -rf $TEST_OUTPUT_DIR/*
	# spack install otf2@3.1.1 openmpi scorep@8.4
	# spack load otf2@3.1.1 openmpi
	# spack load otf2@3.1.1 arch=linux-almalinux9-zen4 %gcc@12.3.0 cubelib@4.9 cubew@4.9 openmpi scorep@8.4
	echo "Ran setup_suite" >> $LOG_DIR/$LOG_FILE
}

