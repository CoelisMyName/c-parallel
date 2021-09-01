#include "sort.h"
#include <stdlib.h>

int64 min(int64 x, int64 y) {
    return x < y ? x : y;
}

int mergeSort(int *arr, int *buff, int64 len) {
    int *a = arr;
    int *b = buff;
    int64 seg, start;
    for (seg = 1; seg < len; seg += seg) {
        for (start = 0; start < len; start += seg + seg) {
            int64 low = start, mid = min(start + seg, len), high = min(start + seg + seg, len);
            int64 k = low;
            int64 start1 = low, end1 = mid;
            int64 start2 = mid, end2 = high;
            while (start1 < end1 && start2 < end2)
                b[k++] = a[start1] < a[start2] ? a[start1++] : a[start2++];
            while (start1 < end1)
                b[k++] = a[start1++];
            while (start2 < end2)
                b[k++] = a[start2++];
        }
        int *temp = a;
        a = b;
        b = temp;
    }
    if (a != arr) {
        return 1;
    }
    return 0;
}

int64 binarySearch(const int *array, int64 length, int value, bool (*compare)(int, int)) {
    int64 lm = 0, rm = length, mid;
    while (lm != rm) {
        mid = (rm - lm) / 2 + lm;
        int temp = array[mid];
        if (compare(temp, value)) {
            lm = mid + 1;
        } else {
            rm = mid;
        }
    }
    return lm;
}

bool lessThan(int a, int b) {
    return a < b;
}

bool lessEqualThan(int a, int b) {
    return a <= b;
}