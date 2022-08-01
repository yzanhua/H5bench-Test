# H5Bench
This file explains how to run external benchmark [h5bench](https://h5bench.readthedocs.io/en/latest/) w/wo Log Vol plugin.

## Install
1. clone the h5bench GitHub repository with the submodules:
    ```shell
    # a floder named "h5bench" will be created
    git clone --recurse-submodules https://github.com/hpc-io/h5bench
    ```
1. set an environment variable to specify the HDF5 install path:
    ```shell
    export HDF5_HOME=/path/to/your/hdf5/installation
    ```
    It should point to a path that contains the `include/`, `lib/`, and `bin/` subdirectories.
1. create `install` and `build` directories 
    ```shell
    mkdir install
    mkdir build
    ```
1. build benchmark and install.
    ```
    cd build

    cmake .. -DCMAKE_INSTALL_PREFIX=/path/to/h5bench/install -DH5BENCH_METADATA=ON -DH5BENCH_AMREX=ON -DH5BENCH_EXERCISER=ON

    make
    make install
    ```
    The following benchmark are installed: `h5bench write`, `h5bench read`, `Metadata Stress`, `AMReX`, `Exerciser`. Other supported benchmarks such as `OpenPMD` and `E3SM-IO` are not installed.
1. set env path
   ```shell
   export LD_LIBRARY_PATH="$HDF5_HOME/lib:$LD_LIBRARY_PATH"
   export PATH="/path/to/h5bench/install/bin:$PATH"
   ```

## Run without Log Vol Plugin
1. Create configuration file (`configuration.json`)
    ```json
    {
        "mpi": {
            "command": "mpirun",
            "ranks": "8",
            "configuration": "-np 8"
        },
        "vol": {
        },
        "file-system": {
            "lustre": {
                "stripe-size": "1M",
                "stripe-count": "4"
            }
        },
        "directory": "output",
        "benchmarks": [
            {
                "benchmark": "write",
                "file": "test.h5",
                "configuration": {
                    "MEM_PATTERN": "CONTIG",
                    "FILE_PATTERN": "CONTIG",
                    "NUM_PARTICLES": "16 M",
                    "TIMESTEPS": "5",
                    "DELAYED_CLOSE_TIMESTEPS": "2",
                    "COLLECTIVE_DATA": "YES",
                    "COLLECTIVE_METADATA": "NO",
                    "EMULATED_COMPUTE_TIME_PER_TIMESTEP": "1 s", 
                    "NUM_DIMS": "1",
                    "DIM_1": "16777216",
                    "DIM_2": "1",
                    "DIM_3": "1",
                    "MODE": "SYNC",
                    "CSV_FILE": "output.csv"
                }
            },
            {
                "benchmark": "exerciser",
                "configuration": {
                    "numdims": "2",
                    "minels": "8 8",
                    "nsizes": "3",
                    "bufmult": "2 2",
                    "dimranks": "8 4"
                }
            },
            {
                "benchmark": "metadata",
                "file": "hdf5_iotest.h5",
                "configuration": {
                    "version": "0",
                    "steps": "20",
                    "arrays": "500",
                    "rows": "100",
                    "columns": "200",
                    "process-rows": "4",
                    "process-columns": "2",
                    "scaling": "weak",
                    "dataset-rank": "4",
                    "slowest-dimension": "step",
                    "layout": "contiguous",
                    "mpi-io": "independent",       
                    "csv-file": "hdf5_iotest.csv"
                }
            },
            {
                "benchmark": "amrex",
                "file": "amrex.h5",
                "configuration": {
                    "ncells": "64",
                    "max_grid_size": "8",
                    "nlevs": "1",
                    "ncomp": "6",
                    "nppc": "2",
                    "nplotfile": "2",
                    "nparticlefile": "2",
                    "sleeptime": "2",
                    "restart_check": "1",
                    "mode": "SYNC",
                    "hdf5compression": "ZFP_ACCURACY#0.001"
                }
            }
        ]
    }
    ```
   This configuration uses the following benchmarks: `h5bench write`, `Metadata Stress`, `AMReX`, `Exerciser`. Also note that if the `mpi:configuration` option is defined, h5bench will ignore the `mpi:ranks` property.
1. run commands
    ```shell
    h5bench configuration.json
    ```   

## Run with Log Vol Plugin
1. Use/create the same `configuration.json` as above.
1. Set env paths:
   ```shell
   export LD_LIBRARY_PATH="/path/to/log-vol-plugin/lib:$LD_LIBRARY_PATH"
   export HDF5_PLUGIN_PATH="/path/to/log-vol-plugin/lib"
   export HDF5_VOL_CONNECTOR="LOG under_vol=0;under_info={}"
   ```
2. Modify `h5bench` script (located in `/path/to/h5bench/install/bin`). We need to modify `h5bench` script because by default, `h5bench` disables usage of any custom HDF5 plugin other than `async`. We need remove such restriction. This step is simple since `h5bench` is a python script and we only need to modify one line.
   
   search for the following function in `h5bench`:
   ```python
    def disable_vol(self, vol):
        """Disable VOL by setting the connector."""
        if 'HDF5_VOL_CONNECTOR' in self.vol_environment:
            del self.vol_environment['HDF5_VOL_CONNECTOR']
   ```
   change it to:
   ```python
    def disable_vol(self, vol):
        return
        # """Disable VOL by setting the connector."""
        # if 'HDF5_VOL_CONNECTOR' in self.vol_environment:
        #    del self.vol_environment['HDF5_VOL_CONNECTOR']
   ```
1. Run commands
    ```shell
    h5bench configuration.json
    ``` 