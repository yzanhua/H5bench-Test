/*
 * This example shows how to create a compound data type,
 * write an array which has the compound data type to the file,
 * and read back fields' subsets.
 */

#include "hdf5.h"
#include "stdlib.h"

#define FILE "test.h5"
#define DATASETNAME "particle"
#define LENGTH 10
#define RANK 1

hid_t PARTICLE_COMPOUND_TYPE;
hid_t PARTICLE_COMPOUND_TYPE_SEPARATES[3];

typedef struct Particle {
    int a;
    float b;
    double c;
} particle;

typedef struct data_md {
    int *a;
    float *b;
    double *c;
} data_contig_md;

hid_t make_compound_type();
hid_t* make_compound_type_separates();

int main(void) {
    // prepare in-mem data
    data_contig_md data_md;
    data_md.a = (int*)malloc(LENGTH * sizeof(int));
    data_md.b = (float*)malloc(LENGTH * sizeof(float));
    data_md.c = (double*)malloc(LENGTH * sizeof(double));
    for(int i = 0; i < LENGTH; i++){
        data_md.a[i] = i;
        data_md.b[i] = i / 10.0;
        data_md.c[i] = i / 100.0;
    }
    
    hid_t file_id = H5Fcreate(FILE, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    hsize_t    dim[] = {LENGTH};
    hid_t space_id = H5Screate_simple(RANK, dim, NULL);
    
    make_compound_type();
    make_compound_type_separates();

    hid_t dset_id = H5Dcreate(file_id, "particle", PARTICLE_COMPOUND_TYPE, space_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Dwrite(dset_id, PARTICLE_COMPOUND_TYPE_SEPARATES[0], H5S_ALL, H5S_ALL, H5P_DEFAULT, data_md.a);
    H5Dwrite(dset_id, PARTICLE_COMPOUND_TYPE_SEPARATES[1], H5S_ALL, H5S_ALL, H5P_DEFAULT, data_md.b);
    H5Dwrite(dset_id, PARTICLE_COMPOUND_TYPE_SEPARATES[2], H5S_ALL, H5S_ALL, H5P_DEFAULT, data_md.c);
    
    H5Sclose(space_id);
    H5Dclose(dset_id);
    H5Fclose(file_id);

    // readin data again
    particle* particles_ptr = (particle*)malloc(LENGTH * sizeof(particle));
    file_id = H5Fopen(FILE, H5F_ACC_RDONLY, H5P_DEFAULT);
    dset_id = H5Dopen(file_id, DATASETNAME, H5P_DEFAULT);
    if (dset_id < 0) {
        printf("Err: dset read err");
    }
    H5Dread(dset_id, PARTICLE_COMPOUND_TYPE, H5S_ALL, H5S_ALL, H5P_DEFAULT, particles_ptr);
    
    for (int i = 0; i < LENGTH; i++){
        printf("Particle %d: a = %d, b=%.2f, c=%.2f\n", i, particles_ptr[i].a, particles_ptr[i].b, particles_ptr[i].c);
    }

    H5Dclose(dset_id);
    H5Fclose(file_id);
}

hid_t make_compound_type() {
    PARTICLE_COMPOUND_TYPE = H5Tcreate(H5T_COMPOUND, sizeof(particle));
    H5Tinsert(PARTICLE_COMPOUND_TYPE, "a", HOFFSET(particle, a), H5T_NATIVE_INT);
    H5Tinsert(PARTICLE_COMPOUND_TYPE, "b", HOFFSET(particle, b), H5T_NATIVE_FLOAT);
    H5Tinsert(PARTICLE_COMPOUND_TYPE, "c", HOFFSET(particle, c), H5T_NATIVE_DOUBLE);
    return PARTICLE_COMPOUND_TYPE;
}

hid_t* make_compound_type_separates() {
    PARTICLE_COMPOUND_TYPE_SEPARATES[0] = H5Tcreate(H5T_COMPOUND, sizeof(int));
    H5Tinsert(PARTICLE_COMPOUND_TYPE_SEPARATES[0], "a", 0, H5T_NATIVE_INT);

    PARTICLE_COMPOUND_TYPE_SEPARATES[1] = H5Tcreate(H5T_COMPOUND, sizeof(float));
    H5Tinsert(PARTICLE_COMPOUND_TYPE_SEPARATES[1], "b", 0, H5T_NATIVE_FLOAT);

    PARTICLE_COMPOUND_TYPE_SEPARATES[2] = H5Tcreate(H5T_COMPOUND, sizeof(double));
    H5Tinsert(PARTICLE_COMPOUND_TYPE_SEPARATES[2], "c", 0, H5T_NATIVE_DOUBLE);

    return PARTICLE_COMPOUND_TYPE_SEPARATES;
}