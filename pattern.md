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

    **filespace**: `H5Screate_simple` (rank 1, dims = num_total_particles), then use `H5Sselect_hyperslab` to select the continuous region (of size num_particles) this mpi-process is responsible for.
    ```c
    H5Sselect_hyperslab(*filespace_out, H5S_SELECT_SET, (hsize_t *)&FILE_OFFSET, NULL, (hsize_t *)&NUM_PARTICLES, NULL);
    ``` 
    **memspace**: `H5Screate_simple` (rank 1, dims = num_particles).

    **summary**: Both are continuous.
2. 2D: The entire dataset to write to the file is a 2D dataset and has dimension `file_dims`. Each mpi-process is responsible for a portion of the dataset with dimension: `mem_dims`. The region is devided along the first dimension. i.e. `mem_dims[0] = file_dims[0] / num_processes` and `mem_dims[1] = file_dims[1]`.
    
    **filespace**: `H5Screate_simple` (rank 2, dims = file_dims), then use `H5Sselect_hyperslab` to select the region this mpi-process is responsible for.
    ```c
    file_starts[0] = dim_1 * (MY_RANK); // file offset for each rank
    file_starts[1] = 0;
    count[0]       = dim_1;
    count[1]       = dim_2;
    H5Sselect_hyperslab(*filespace_out, H5S_SELECT_SET, file_starts, NULL, count, NULL);
    ``` 
    **memspace**: `H5Screate_simple` (rank 2, dims = mem_dims).
    
    **summary**: 
    memspace is continous. filespace is not.
3. 3D: same as 2D except that there is one more dimension. The region is still devided along the first dimension only.

### Write Patterns
1. contigious mem -> contigious file:

    There are eight `H5Dcreate()` followed by eight `H5Dwrite()`, one for each property. 

    Each dataset is created with the `datatype` being either `NATIVE_INT` or `NATIVE_FLOAT`. Each `H5Dwrite` also writes `NATIVE_INT` or `NATIVE_FLOAT` to the dataset.
    
    The data transfer property is either `MPI_INDEPENDENT` or `MPI_COLLECTIVE`, per user's choice.
    
2. contigious mem -> interleaved file:

    There is only one `H5Dcreate` to create the dataset in file. The dataset has a COMPOUND datatype. So the file/dataset saves an array of structs.

    However, there are eight `H5Dwrite`, one for each property. Each write only writes one property to the dataset.

    The memory pattern is contigious, meaning that we don't have an array of structs in memory, so we cannot use one single `H5Dwrite` to write the entire dataset.

    A COMPOUND datatype has 8 fields, one for each property (e,g, filed `x`, `y` and `z`). Each `H5Dwrite` does not write `NATIVE_INT`s or `NATIVE_FLOAT`s to the dataset. Instead,for example to write all `x`s to the dataset, we create one new type using `H5Tcreate` and this new type has only one field `x`.
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

    There are eight `H5Dcreate` to create eight datasets in file, one for each property. However, each dataset has `datatype` neither `NATIVE_INT` nor `NATIVE_FLOAT`. Instead, it creates one new data type for each of the dataset using `H5Tcreate`. For example, to create a dataset for the property `x`, One new type is created with only one filed `x`:
    ```c
    hid_t new_type = H5Tcreate(H5T_COMPOUND, sizeof(float));
    H5Tinsert(new_type, "x", 0, H5T_NATIVE_FLOAT);
    H5Dcreate_async(loc, "x", new_type, filespace, H5P_DEFAULT, dcpl,
                                  H5P_DEFAULT, ts->es_meta_create);
    ```
    
    Since we only have one array of structs in memory (interleaved mem), each of the `H5Dwrite` tries to write the whole array to the dataset. `H5Dwrite` will compare the COMPOUND type with this new type, and identify the postions to write from using the field name `x`.

    ```c
    H5Dwrite_async(entire_dset_in_mem, PARTICLE_COMPOUND_TYPE, memspace, filespace, plist_id, data_in, ts->es_data);
    ```

    The data transfer property is either `MPI_INDEPENDENT` or `MPI_COLLECTIVE`, per user's choice.
    

# Metadata Stress Benchmark
This benchmark accesses the performance of an HDF5 "common pattern". The pattern consists of one write phase and then one read phase (first write then read).

## Overall Info
We assume the number of mpi-processes is `num_mpi_processes = proc_rows * proc_columns`, where `proc_rows` and `proc_columns` are controlled by users. Later, the entire dataset will be divided along two axies among all mpi-processes, one using `proc_rows` and one using proc_columns`.

The dataset to read/write is a 4D dataset, with dimensions 
```c
dims[0] = num_steps
dims[1] = num_arrays
dims[2] = total_rows
dims[3] = total_cols
```
where all four parameters `num_steps`,...,`total_cols` are controlled by users. The entire dataset is divided along the last two dimensions, meaning each mpi process is reponsible for  `total_rows / proc_rows` * `total_cols / proc_columns` many data (in terms of the last two dimension)

## Write Pattern:


# Exerciser Benchmark