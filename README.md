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

H5Bench key points [(paper)](https://sdm.lbl.gov/~sbyna/research/papers/2021/202106-CUG_2021_h5bench.pdf):
1. various in memory data models and file access patterns
2. asynchronous I/O feature
3. performance evaluation of compression in I/O libraries
4. evaluation and optimization of user-level metadata access costs



## h5bench: Read / Write Benchmark 
1. I/O operations (read, write, and HDF5 metadata), 
2. data locality (arrays of basic data types and arrays of structure representations both in memory and in file)
3. array dimensionality (1D arrays, 2D meshes, 3D cubes)
4. different I/O modes (synchronous and asynchronous).

It assumes: simulation or analysis done in many time steps with multiple subsequent computation and I/O phases. Write: use sleep () to emulate the computation time. Read: use sleep() to emulate data analysis time.

WRITE PATTERN:
1. MEM_PATTERN: the source (the data layout in the memory):
    1. CONTIG: represents arrays of basic data types (i.e., int, float, double, etc.)
    2. INTERLEAVED: represents an array of structure (AOS) where each array element is a C struct
    3. STRIDED: represents a few elements in an array of basic data types that are separated by a constant stride
2. FILE_PATTERN: the destination (the data layout in the resulting file)
    1. CONTIG: represents a HDF5 dataset of basic data types (i.e., int, float, double, etc.)
    2. INTERLEAVED represents a dataset of a compound datatype;

Details:
===
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
MEM_PATTERN:
1. CONTIG: allocate data using `data_md`. e.g all particles' `id_1`s are saved at `data_md->id_1`. When saving, the hdf5 looks like:

    /        # root
    
    |--> /x # group "x". saving all x values.

    |--> /y

    |--> /z
    ....

2. INTERLEAVED: allocate data using `Particle`. Particles are saved at an array of `Particle`.
When saving, the hdf5 looks like:
    
    /        # root
    
    |--> /particles # group "particles". is the only group. Each particle is saved in this group using COMPOUND_TYPE.


3. STRIDED: represents a few elements in an array of basic data types that are separated by a constant stride