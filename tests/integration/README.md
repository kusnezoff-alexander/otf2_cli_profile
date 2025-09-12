How to run tests:
```sh
# 0. Make sure `bats` is installed (also see [BATS Docs: Installation](https://bats-core.readthedocs.io/en/stable/installation.html):
spack install bats
spack load bats

# Run tests (but pls use the provided CMake command eg `make run_tests` instead):
bats test_file_stats.bats
```
