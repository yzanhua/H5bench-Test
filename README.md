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
9. test9: INTERLEAVED write + 2 proc + 1M/proc + collective data off  --- ERROR
10. test10: Simplified version of INTERLEAVED write that works correctly. Study why 5 H5Dwrite for the case of "contigious mem -> interleaved file"
11. test11: Metadata Stress.



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

H5Bench key points [(paper)](https://sdm.lbl.gov/~sbyna/research/papers/2021/202106-CUG_2021_h5bench.pdf):
1. various in memory data models and file access patterns
2. asynchronous I/O feature
3. performance evaluation of compression in I/O libraries
4. evaluation and optimization of user-level metadata access costs



# h5bench: Read / Write Benchmark 
1. I/O operations (read, write, and HDF5 metadata), 
2. data locality (arrays of basic data types and arrays of structure representations both in memory and in file)
3. array dimensionality (1D arrays, 2D meshes, 3D cubes)
4. different I/O modes (synchronous and asynchronous).

It assumes: simulation or analysis done in many time steps with multiple subsequent computation and I/O phases. Write: use sleep () to emulate the computation time. Read: use sleep() to emulate data analysis time.



## Data Patterns:
### Some Helpful Structs:
A particle looks like:
```c
typedef struct Particle {
    float x, y, z;
    float px, py, pz;
    int   id_1;
    float id_2;
} particle;
```
and a metada looks like 
```c
typedef struct data_md {
    unsigned long long particle_cnt;
    unsigned long long dim_1, dim_2, dim_3;
    float *x, *y, *z;
    float *px, *py, *pz;
    int *id_1;
    float *id_2;
} data_contig_md;
```

### Choicese of Pattern
User can specify `MEM_PATTERN` and `FILE_PATTERN`:
1. `MEM_PATTERN`: the source (the data layout in the memory):
    1. `CONTIG`: represents arrays of basic data types (i.e., int, float, double, etc.). i.e. particles are saved using `data_contig_md`, where e.g. `data_md->x[0]` is the `x` value of the 1st particle.
    2. `INTERLEAVED`: represents an array of structure (AOS) where each array element is a C struct. particles are saved using an array of `Particle`.
    3. `STRIDED`: represents a few elements in an array of basic data types that are separated by a constant stride
2. `FILE_PATTERN`: the destination (the data layout in the resulting file)
    1. `CONTIG`: represents a HDF5 dataset of basic data types (i.e., int, float, double, etc.)
    2. `INTERLEAVED` represents a dataset of a compound datatype;


## Mem to File Patterns
1. contigious mem -> contigious file:

    `5` H5Dcreate, one for each property.
    
    `5` H5Dwrite, one for each property.
2. contigious mem -> interleaved file:

    `1` H5Dcreate, for the entire particles

    `5` H5Dwrite, one for each property, each write writes to target pos directly.

3. interleaved mem -> interleaved file:

    `1` H5Dcreate, for the entire particles

    `1` H5Dwrite, for the entire particles

4. interleaved mem -> contig file:

    `5` H5Dcreate, one for each property.
    
    `5` H5Dwrite, one for each property.

## MPI-IO:
collevtive/independet per user's choice.


 
## Metadata Stress Benchmark
This benchmark accesses the performance of an HDF5 "common pattern". The pattern consists of one write phase and then one read phase (first write then read).

**Write Phase:**

It uses (`p_rows` * `p_columns`) many mpi-processes to collectively/independently (per user's choice) create one 4D dataset of `doubles`. The dataset has dimensions `dims` = `num_steps` * `num_arrays` * `total_rows` * `total_cols`. The entire dataset is diveded along the last two dimensions, meaning that each mpi process is responsible for `num_steps` * `num_arrays` * (`total_rows`/`p_rows`) * (`total_cols`/`p_cols`) much of data. Each mpi process will call `num_steps` * `num_arrays` many H5Sselect_hyperslab and H5Dwrites, and each write writes (`total_rows`/`p-rows`) * (`total_cols`/`p-cols`) much of data.

**Read Phase:**

Read Phase is identical to Write Phase except that all `writes` are changed to `reads`.



