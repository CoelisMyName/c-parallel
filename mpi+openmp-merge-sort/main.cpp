#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <omp.h>
#include "readFile.h"
#include "sort.h"

int* arr;
int* buf;
int64 length;

int processSize;
int threadSize;
int64* starts1, * ends1;

int* flag;
int rank;

MPI_Comm allcomm;
MPI_Win arrWin;
MPI_Win bufWin;

int64 getMyStart(int id, int total, int64 aLength) {
    int64 base = aLength / total;
    int64 mod = aLength - (base * total);
    int64 start = base * id;
    if (id <= mod) {
        return start + id;
    }
    else {
        return start + mod;
    }
}

int64 getMyEnd(int id, int total, int64 aLength) {
    int64 base = aLength / total;
    int64 mod = aLength - (base * total);
    int64 end = base * (id + 1);
    if (id < mod) {
        return end + id + 1;
    }
    else {
        return end + mod;
    }
}

void printResult(int v) {
    printf("%d\t", v);
}

void initialSingleProcess() {
    buf = (int*)malloc(length * sizeof(int));
    starts1 = (int64*)malloc(sizeof(int64) * threadSize);
    ends1 = (int64*)malloc(sizeof(int64) * threadSize);
    flag = (int*)malloc(sizeof(int) * threadSize);
}

void destroySingleProcess() {
    free(arr);
    free(buf);
    free(starts1);
    free(ends1);
    free(flag);
}

void singleProcessWork() {
#pragma omp parallel
    {
        int id = omp_get_thread_num();
        int* a = arr, * b = buf;
        int64 start = starts1[id] = getMyStart(id, threadSize, length);
        int64 end = ends1[id] = getMyEnd(id, threadSize, length);
        flag[id] = mergeSort(&a[start], &b[start], end - start);
        if (flag[id] > 0) {
            int* temp = a;
            a = b;
            b = temp;
        }
#pragma omp barrier
        int64 index = 0, remain = 0;
        int64* starts2, * ends2;
        int64 length0 = ends1[0] - starts1[0];
        int64 minIdx = getMyStart(id, threadSize, length0), maxIdx = getMyEnd(id, threadSize, length0);
        int minValue, maxValue;
        if (minIdx < ends1[0]) {
            minValue = a[minIdx];
        }
        else {
            minValue = a[ends1[0] - 1];
        }
        if (maxIdx < ends1[0]) {
            maxValue = a[maxIdx];
        }
        else {
            maxValue = a[ends1[0] - 1];
        }
        starts2 = (int64*)malloc(sizeof(int64) * threadSize);
        ends2 = (int64*)malloc(sizeof(int64) * threadSize);

        if (id > 0) {
            for (int i = 0; i < threadSize; ++i) {
                int64 ss = starts1[i];
                int64 ll = ends1[i] - starts1[i];
                int64 idx = binarySearch(&a[ss], ll, minValue, lessThan);
                starts2[i] = ss + idx;
                index += idx;
            }
        }
        else {
            for (int i = 0; i < threadSize; ++i) {
                starts2[i] = starts1[i];
            }
        }
        /**
         * generate ends2 and remain(number of element to merge)
         */
        if (id < threadSize - 1) {
            for (int i = 0; i < threadSize; ++i) {
                int64 ss = starts1[i];
                int64 ll = ends1[i] - starts1[i];
                int64 idx = binarySearch(&a[ss], ll, maxValue, lessThan);
                ends2[i] = ss + idx;
                remain += ends2[i] - starts2[i];
            }
        }
        else {
            for (int i = 0; i < threadSize; ++i) {
                ends2[i] = ends1[i];
                remain += ends2[i] - starts2[i];
            }
        }
        /**
         * merge
         */
        do {
            int idx = -1;
            int m = 0;
            for (int i = 0; i < threadSize; ++i) {
                if (starts2[i] == ends2[i]) {
                    continue;
                }
                if (idx < 0 || a[starts2[i]] < m) {
                    idx = i;
                    m = a[starts2[i]];
                }
            }
            remain -= 1;
            starts2[idx] += 1;
            b[index] = m;
            index += 1;
        } while (remain > 0);
        ;
        /**
         * barrier
         */
#pragma omp barrier
        if (id == 0) {
            printResult(b[length / 2]);
        }
        free(starts2);
        free(ends2);
    }
}

void initial() {
    if (rank == 0) {
        /*申请存放数据集内存*/
        int* temp;
        MPI_Win_allocate_shared(length * sizeof(int), sizeof(int), MPI_INFO_NULL, allcomm, &temp, &arrWin);
        memcpy(temp, arr, length * sizeof(int));
        free(arr);
        arr = temp;
    }
    else {
        int* temp;
        MPI_Win_allocate_shared(0 * sizeof(int), sizeof(int), MPI_INFO_NULL, allcomm, &temp, &arrWin);
    }

    if (rank != 0) {
        /*获取存放数据集内存*/
        MPI_Aint shareLength;
        int unit;
        MPI_Win_shared_query(arrWin, 0, &shareLength, &unit, &arr);
        length = shareLength / unit;
    }

    if (rank == 1) {
        /*申请存放缓冲内存*/
        MPI_Win_allocate_shared(length * sizeof(int), sizeof(int), MPI_INFO_NULL, allcomm, &buf, &bufWin);
    }
    else {
        MPI_Win_allocate_shared(0 * sizeof(int), sizeof(int), MPI_INFO_NULL, allcomm, &buf, &bufWin);
    }
    if (rank != 1) {
        /*获取存放缓冲内存*/
        MPI_Aint shareLength;
        int unit;
        MPI_Win_shared_query(bufWin, 1, &shareLength, &unit, &buf);
    }
    starts1 = (int64*)malloc(sizeof(int64) * threadSize * processSize);
    ends1 = (int64*)malloc(sizeof(int64) * threadSize * processSize);
    flag = (int*)malloc(sizeof(int) * threadSize * processSize);
    MPI_Barrier(allcomm);
}

void destroy() {
    free(starts1);
    free(ends1);
    free(flag);
    MPI_Win_free(&arrWin);
    MPI_Win_free(&bufWin);
}

void work() {
#pragma omp parallel
    {
        int id = omp_get_thread_num() + rank * threadSize;
        int* a = arr, * b = buf;
        for (int i = 0; i < threadSize * processSize; ++i) {
            starts1[i] = getMyStart(i, threadSize * processSize, length);
            ends1[i] = getMyEnd(i, threadSize * processSize, length);
        }
        int64 start = starts1[id];
        int64 end = ends1[id];

        flag[id] = mergeSort(&a[start], &b[start], end - start);
        if (flag[id] > 0) {
            int* temp = a;
            a = b;
            b = temp;
        }
#pragma omp barrier
#pragma omp single
        {
            MPI_Barrier(allcomm);
        }
#pragma omp barrier
        
        int64 index = 0, remain = 0;
        int64* starts2, * ends2;
        int64 length0 = ends1[0] - starts1[0];
        int64 minIdx = getMyStart(id, threadSize * processSize, length0), maxIdx = getMyEnd(id, threadSize * processSize, length0);
        int minValue, maxValue;
        if (minIdx < ends1[0]) {
            minValue = a[minIdx];
        }
        else {
            minValue = a[ends1[0] - 1];
        }
        if (maxIdx < ends1[0]) {
            maxValue = a[maxIdx];
        }
        else {
            maxValue = a[ends1[0] - 1];
        }
        starts2 = (int64*)malloc(sizeof(int64) * threadSize * processSize);
        ends2 = (int64*)malloc(sizeof(int64) * threadSize * processSize);

        if (id > 0) {
            for (int i = 0; i < threadSize * processSize; ++i) {
                int64 ss = starts1[i];
                int64 ll = ends1[i] - starts1[i];
                int64 idx = binarySearch(&a[ss], ll, minValue, lessThan);
                starts2[i] = ss + idx;
                index += idx;
            }
        }
        else {
            for (int i = 0; i < threadSize * processSize; ++i) {
                starts2[i] = starts1[i];
            }
        }

        if (id < threadSize * processSize - 1) {
            for (int i = 0; i < threadSize * processSize; ++i) {
                int64 ss = starts1[i];
                int64 ll = ends1[i] - starts1[i];
                int64 idx = binarySearch(&a[ss], ll, maxValue, lessThan);
                ends2[i] = ss + idx;
                remain += ends2[i] - starts2[i];
            }
        }
        else {
            for (int i = 0; i < threadSize * processSize; ++i) {
                ends2[i] = ends1[i];
                remain += ends2[i] - starts2[i];
            }
        }

        do {
            int idx = -1;
            int m = 0;
            for (int i = 0; i < threadSize * processSize; ++i) {
                if (starts2[i] == ends2[i]) {
                    continue;
                }
                if (idx < 0 || a[starts2[i]] < m) {
                    idx = i;
                    m = a[starts2[i]];
                }
            }
            remain -= 1;
            starts2[idx] += 1;
            b[index] = m;
            index += 1;
        } while (remain > 0);
#pragma omp barrier
#pragma omp single
        {
            MPI_Barrier(allcomm);
        }
#pragma omp barrier
        if (id == 0) {
            printResult(b[length / 2]);
        }
        free(starts2);
        free(ends2);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        return 1;
    }
    MPI_Init(NULL, NULL);
    allcomm = MPI_COMM_WORLD;
    MPI_Comm_size(allcomm, &processSize);
    MPI_Comm_rank(allcomm, &rank);
    if (rank == 0) {
        arr = readFile(argv[1], &length);
    }
    threadSize = strtol(argv[2], NULL, 10);
    if (threadSize * processSize == 1) {
        buf = (int*)malloc(length * sizeof(int));
        if (mergeSort(arr, buf, length) > 0) {
            printResult(buf[length / 2]);
        }
        else {
            printResult(arr[length / 2]);
        }
        free(arr);
        free(buf);
        MPI_Finalize();
        return 0;
    }
    omp_set_num_threads(threadSize);
    if (processSize == 1) {
        buf = (int*)malloc(length * sizeof(int));
        initialSingleProcess();
        singleProcessWork();
        destroySingleProcess();
        MPI_Finalize();
        return 0;
    }
    initial();
    work();
    destroy();
    MPI_Finalize();
    return 0;
}