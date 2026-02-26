# Recommended Realistic HPC Workloads for Testing

This document outlines realistic HPC workloads that can be used to validate the OTF-Profiler visualization with real-world I/O patterns. These applications run 30-60+ minutes and generate diverse I/O patterns.

## Recommended Workloads

### 1. IOR (Parallel I/O Benchmark)
**Repository**: https://github.com/hpc/ior
**Runtime**: 30-60 minutes (configurable)
**I/O Patterns**: POSIX, MPI-IO, HDF5, NetCDF

IOR is the standard HPC I/O benchmark supporting multiple backends:
- POSIX (direct file I/O)
- MPI-IO
- HDF5
- NetCDF
- DFS (DAOS)

**Run commands:**
```bash
# Clone and build
git clone https://github.com/hpc/ior.git
cd ior
./bootstrap
./configure --with-s3fs=/path/to/s3fs
make -j

# Run with multiple backends
mpirun -np 64 ./src/ior -a POSIX -t 4m -b 16g -s 10 /tmp/ior_test
mpirun -np 64 ./src/ior -a MPIIO -t 4m -b 16g -s 10 /tmp/ior_test
mpirun -np 64 ./src/ior -a HDF5 -t 4m -b 16g -s 10 /tmp/ior_test.h5
```

### 2. MDTest (Metadata Benchmark)
**Repository**: https://github.com/hpc/ior (same repo)
**Runtime**: 15-30 minutes
**I/O Patterns**: File creation, stat, open, read

**Run commands:**
```bash
mpirun -np 64 ./src/mdtest -d /tmp/mdtest -n 10000 -i 3
```

### 3. LAMMPS (Molecular Dynamics)
**Repository**: https://github.com/lammps/lammps
**Runtime**: 60+ minutes
**I/O Patterns**: Checkpointing, trajectory dumps, log files

LAMMPS is a widely-used molecular dynamics simulator with complex I/O patterns:
- Periodic checkpoint files (binary)
- Trajectory outputs (XYZ, DCD, LAMMPS dump formats)
- Log files

**Run commands:**
```bash
# Build with MPI
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED=ON -DMLIAP_ENABLE=OFF ../cmake
make -j

# Download benchmark inputs
wget https://lammps.org/bench/bench.tar.gz
tar -xzf bench.tar.gz

# Run benchmark
mpirun -np 16 lmp -in in.lj
mpirun -np 16 lmp -in in.chain
mpirun -np 16 lmp -in in.rhodo
```

### 4. HPCG (High Performance Conjugate Gradient)
**Repository**: https://github.com/hpcg-bench/hpcg
**Runtime**: 30-60 minutes
**I/O Patterns**: Sparse matrix I/O, checkpointing

**Run commands:**
```bash
./configure
make
mpirun -np 64 ./bin/xhpcg --iterations=1000
```

### 5. Graph500 (Graph Analytics)
**Repository**: https://github.com/graph500/graph500
**Runtime**: 30-60 minutes
**I/O Patterns**: Large-scale data exchange, irregular access

**Run commands:**
```bash
./configure
make
mpirun -np 64 ./bin/graph500_mpi_simple --scale=36 --edge-factor=16
```

### 6. nekRS (Nek5000 CFD Solver)
**Repository**: https://github.com/nek5000/nekrs
**Runtime**: 60+ minutes
**I/O Patterns**: Checkpoint/restart, field outputs

nekRS is a GPU-accelerated computational fluid dynamics solver.

## Recommended Test Suite

For comprehensive validation, run this sequence:

```bash
#!/bin/bash
# test_workloads.sh

export SCOREP_EXPERIMENT_DIRECTORY=scorep_trace_ior
scorep mpirun -np 32 ior -a POSIX -t 4m -b 8g -s 5
../build/otf-profiler --json -i scorep_trace_ior/traces.otf2 -o results_ior

export SCOREP_EXPERIMENT_DIRECTORY=scorep_trace_mdtest  
scorep mpirun -np 32 mdtest -d /tmp/mdtest -n 5000
../build/otf-profiler --json -i scorep_trace_mdtest/traces.otf2 -o results_mdtest

# Combine results and visualize
# (scripts would merge the JSON outputs)
quarto render scripts/output.qmd
```

## Expected I/O Patterns

| Workload | Primary Paradigm | Access Pattern | Duration |
|----------|----------------|----------------|----------|
| IOR POSIX | POSIX | Contiguous | 30 min |
| IOR MPI-IO | MPI-IO | Contiguous + Strided | 30 min |
| IOR HDF5 | HDF5 | Chunked | 30 min |
| MDTest | POSIX | Metadata | 15 min |
| LAMMPS | POSIX | Contiguous (checkpoints) | 60 min |
| HPCG | MPI-IO | Collective | 30 min |
| Graph500 | MPI-IO | Irregular | 60 min |

## Validation Checklist

- [ ] All I/O paradigms detected (POSIX, MPI-IO, HDF5, NetCDF)
- [ ] Access patterns correctly identified (CONTIGUOUS, STRIDED, RANDOM)
- [ ] Per-rank I/O statistics accurate
- [ ] File counts match expected values
- [ ] Total bytes reasonable for workload size
- [ ] Visualization renders without errors
- [ ] Charts display correct data
