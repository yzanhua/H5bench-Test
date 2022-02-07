# h5bench: Write Benchmark
## Data Structures
A particle has 8 properties:
```c
typedef struct Particle {
    float x, y, z;
    float px, py, pz;
    int   id_1;
    float id_2;
} particle;
```
and a struct of arraies looks like 
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
## Patterns
### Filespace and Memspace
For all 4 write patterns, the creation of filesapce and memspace are the same. Filesapce and memspace are determined by the num of dimestion of the dataset. 
1. 1D: The entire dataset to write to the file is a 1D dataset and has length `num_total_particles`. Each mpi-process is responsible for a portion of the dataset with length: `num_particles = num_total_particles / num_mpi_processes`.

    **filespace**: `H5Screate_simple` (rank 1, dims = num_total_particles), then use `H5Sselect_hyperslab` to select the contiguous region (of size num_particles) this mpi-process is responsible for.
    ```c
    H5Sselect_hyperslab(*filespace_out, H5S_SELECT_SET, (hsize_t *)&FILE_OFFSET, NULL, (hsize_t *)&NUM_PARTICLES, NULL);
    ``` 
    **memspace**: `H5Screate_simple` (rank 1, dims = num_particles). The region (of size 1 * num_particles) is the exact region each mpi-process owns and modifies. There's no H5Sselect_hyperslab for memspace.

    **summary**: Both memspace and filespace are contiguous.
2. 2D: The entire dataset to write to the file is a 2D dataset and has dimension `file_dims`. Each mpi-process is responsible for a portion of the dataset with dimension: `mem_dims`. The region is devided along the first dimension. i.e. `mem_dims[0] = file_dims[0] / num_processes` and `mem_dims[1] = file_dims[1]`.
    
    **filespace**: `H5Screate_simple` (rank 2, dims = file_dims), then use `H5Sselect_hyperslab` to select the region this mpi-process is responsible for.
    ```c
    // dim_1 = mem_dims[0], dim_2 = mem_dims[1]
    file_starts[0] = dim_1 * (MY_RANK); // file offset for each rank
    file_starts[1] = 0;
    count[0]       = dim_1;
    count[1]       = dim_2;
    H5Sselect_hyperslab(*filespace_out, H5S_SELECT_SET, file_starts, NULL, count, NULL);
    ``` 
    **memspace**: `H5Screate_simple` (rank 2, dims = mem_dims). The region is the exact region each mpi-process owns and modifies. There's no H5Sselect_hyperslab for memspace.
    
    **summary**: 
    memspace is contiguous. ~~filespace is not.~~ update: filespace is also contiguous since the entire filespace is devided only along the first dimension (row dimension)
3. 3D: same as 2D except that there is one more dimension. The region is still devided along the first dimension only. So both memsapce and filespace is contiguous.

### Write Patterns
1. contiguous mem -> contiguous file:

    There are eight `H5Dcreate()` followed by eight `H5Dwrite()`, one for each particle property. 

    Each dataset is created with the `datatype` being either `NATIVE_INT` or `NATIVE_FLOAT`. Each `H5Dwrite` also writes `NATIVE_INT` or `NATIVE_FLOAT` to the dataset.
    
    The data transfer property is either `MPI_INDEPENDENT` or `MPI_COLLECTIVE`, per user's choice.
    
2. contiguous mem -> interleaved file:

    There is only one `H5Dcreate` to create the dataset in file. The dataset has a COMPOUND datatype. So the file/dataset saves an array of structs.

    However, there are eight `H5Dwrite`, one for each particle property. Each write only writes one property to the dataset.

    The memory pattern is contiguous, meaning that we don't have an array of structs in memory, so we cannot use one single `H5Dwrite` to write the entire dataset from mem to file.

    A COMPOUND datatype has 8 fields, one for each paticle property (e,g, filed `x`, `y` and `z`). Each `H5Dwrite` does not write `NATIVE_INT`s or `NATIVE_FLOAT`s to the dataset. Instead, for example to write all `x`s to the dataset, we create one new type using `H5Tcreate` and this new type has only one field `x`.
    ```c
    hid_t new_type = H5Tcreate(H5T_COMPOUND, sizeof(float));
    H5Tinsert(new_type, "x", 0, H5T_NATIVE_FLOAT);
    ```
    `H5Dwrite` will compare this new type with the COMPOUND type, and identify the postions to write using the field name `x`.

    The data transfer property is either `MPI_INDEPENDENT` or `MPI_COLLECTIVE`, per user's choice.

3. interleaved mem -> interleaved file:

    There is only one `H5Dcreate` to create the dataset in file. The dataset has a COMPOUND datatype. So the file/dataset saves an array of structs.

    There is only one `H5Dwrite`, to write the array of structs (also using the COMPOUND datatype) in mem to file.

    The data transfer property is either `MPI_INDEPENDENT` or `MPI_COLLECTIVE`, per user's choice.

4. interleaved mem -> contig file:

    There are eight `H5Dcreate` to create eight datasets in file, one for each particle property. However, each dataset has `datatype` NEITHER `NATIVE_INT` NOR `NATIVE_FLOAT`. Instead, it creates one new data type for each of the dataset using `H5Tcreate`. For example, to create a dataset for the property `x`, one new type is created (with only one filed `x`) instead of using NATIVE_FLOAT directly:
    ```c
    hid_t new_type = H5Tcreate(H5T_COMPOUND, sizeof(float));
    H5Tinsert(new_type, "x", 0, H5T_NATIVE_FLOAT);
    // create the dataset using new_type
    H5Dcreate_async(loc, "x", new_type, filespace, H5P_DEFAULT, dcpl,
                                  H5P_DEFAULT, ts->es_meta_create);
    ```
    
    Since we only have one array of structs in memory (interleaved mem), each of the `H5Dwrite` feeds the whole array of structs (COMPOUND type) as the input buffer. `H5Dwrite` will compare the COMPOUND type from the input with `new_type`, and identify the postions to write from using the field name `x`.

    ```c
    H5Dwrite_async(entire_dset_in_mem, PARTICLE_COMPOUND_TYPE, memspace, filespace, plist_id, data_in, ts->es_data);
    ```

    The data transfer property is either `MPI_INDEPENDENT` or `MPI_COLLECTIVE`, per user's choice.
    

# Metadata Stress Benchmark
This benchmark accesses the performance of an HDF5 "common pattern". The pattern consists of one write phase followed by one read phase.

## Overall Info
The number of mpi-processes is `num_mpi_processes = proc_rows * proc_columns`, where `proc_rows` and `proc_columns` are controlled by users. Later, the entire dataset will be divided along two axies among all mpi-processes, one using `proc_rows` and one using proc_columns`.

The dataset to read/write is a 4D dataset, with dimensions `dims`: 
```c
dims[0] = num_steps;
dims[1] = num_arrays;
dims[2] = total_rows;
dims[3] = total_cols;
```
where all four parameters `num_steps`,...,`total_cols` are controlled by users. The entire dataset is divided along the last two dimensions, meaning each mpi process is reponsible for  `dims_each_mpi = mem_dims` many data:
```c
mem_dims[0] = dims[0];
mem_dims[1] = dims[1];
mem_dims[2] = dims[2] / proc_rows;
mem_dims[3] = dims[3] / proc_columns;
```


## Write Pattern:
For each mpi-process, there is a nested loops to write to the 4D dataset. It first iterates along `num_steps`, then iterates along `num_arrays`. Inside the nested loops, there is one `H5Dwrite` call. i.e.
```c
for (istep = 0; istep < num_steps; ++istep) {
    for (iarray = 0; iarray < num_arrays; ++iarray) {
        // first select the filespace
        // then one H5Dwrite() call
    }
}
```
So each mpi-process will call `num_steps * num_arrays` many `H5Dwrite`.

`MPI-INDEPENDENT` or `MPI-COLLECTIVE` per user's choice.

## Memspace and Filespace
The memspace has dimensions `mem_dims`. There is no `H5Sselect_hyperslab` for memspace. It is contigious.

The entire filespace is devided along the last two dimensions, so the filespace of each mpi-process is not contigious.



## Read Pattern:
Read Phase is identical to Write Phase except that all `write`s are changed to `read`s.

# Exerciser Benchmark
## Overall Info
This benchmark does the following repeatedly (repeat to measure time performance)

1. Create the same file (ACC_TRUNC)
2. Add group (/Data)
3. (optional) add 64 attributes to the /Data group
4. Create a dataset under the group /Data.
5. Write to the dataset.
6. Close the file, reopen.
7. Read the dataset and check correctness. close the dataset.
8. (optional) Write a dataset of a compound type, 
    (dimensions same as before) at 
    /Data/derivedData/sim
9. close everything

## Dataset Info:
The dimensions are fully controlled by users. It can be 1D, 2D, or maximum 4D dataset of any size. Users also control how to devide each dimensions among mpi-processes.

## Memsapce and Filespace:
Users have full control over `file-block`, `file-stride`, `mem-block`, `mem-stride`.
For the default behavior (dataset devided along the first dimension only, all `file/mem-stride/block` = 1), both memsapce and filespace are contigious.
