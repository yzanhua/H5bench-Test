HDF5_HOME=/homes/zhd1108/HDF5/1.12.0
MPI_HOME=/homes/zhd1108/MPICH/3.4.2
APP_HOME=/homes/zhd1108/H5BENCH/native

all:
	rm -rf full-test
	rm -f configuration-*.log

	cp ${APP_HOME}/h5* .

	./h5bench --debug configuration.json