Content
====
1. test1: default tests. 4 proc + 16M/proc
2. test2: conti write + 8 proc + 16M/proc
3. test3: conti write + 8 proc + 16M/proc  -- ERROR -- wrong-config
4. test4: conti write + 8 proc + 8M/proc + log-vol + collective data off
5. test5: conti write + 8 proc + 8M/proc + collective data off
6. test6: conti write + 8 proc + 8M/proc + collective data on
7. test7: INTERLEAVED write + 8 proc + 8M/proc + log-vol + collective data on  --- ERROR
8. test8: INTERLEAVED write + 8 proc + 8M/proc + collective data on  --- ERROR



```
usage: h5bench [-h] [--abort-on-failure] [--debug] [--validate-mode] setup

H5bench: a Parallel I/O Benchmark Suite for HDF5:

positional arguments:
  setup               JSON file with the benchmarks to run

optional arguments:
  -h, --help          show this help message and exit
  --abort-on-failure  Stop h5bench if a benchmark failed
  --debug             Enable debug mode
  --validate-mode     Validated if the requested mode (async/sync) was run
```

More details at [link](https://h5bench.readthedocs.io/en/latest/running.html).