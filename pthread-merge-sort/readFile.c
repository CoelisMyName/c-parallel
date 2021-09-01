#include <stdio.h>
#include <stdlib.h>
#include "readFile.h"

int *readFile(char *path, long long *lsize) {
    FILE *file = fopen(path, "rb");
    int result;
    int *data;

    if (file) {
        fseek(file, 0, SEEK_END);
        long long size = ftell(file);
        fseek(file, 0, SEEK_SET);
        (*lsize) = size / sizeof(int);

        data = (int *) malloc(sizeof(int) * (*lsize));
        if (data == NULL) {
            fputs("Memory error", stderr);
            exit(2);
        }
        result = fread(data, sizeof(int), (*lsize), file);
        if (result != (*lsize)) {
            fputs("Reading error", stderr);
            exit(3);
        }

        fclose(file);
        return data;
    } else {
        fprintf(stderr, "文件打不开\n");
    }
    return NULL;
}

void writeFile(char *path, int *data, long long length) {
    FILE *file;
    file = fopen64(path, "wb");
    if (file) {
        fwrite(data, sizeof(int), length, file);
    }
    fclose(file);
}