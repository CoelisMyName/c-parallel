#pragma once
#ifndef SORT_H
#define SORT_H

#include <stdbool.h>

typedef long long int64;

extern int mergeSort(int *arr, int *buff, int64 len);

extern int64 binarySearch(const int *array, int64 length, int value, bool (*compare)(int, int));

extern bool lessThan(int a, int b);

extern bool lessEqualThan(int a, int b);

#endif