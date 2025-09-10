LOG_FILE=test.log

function setup_suite(){
	spack install otf2@3.1.1 openmpi scorep@8.4
	# spack load otf2@3.1.1 openmpi
	spack load otf2@3.1.1 arch=linux-almalinux9-zen4 %gcc@12.3.0 cubelib@4.9 cubew@4.9 openmpi scorep@8.4
	echo "Ran setup_suite" >> $LOG_FILE
}

