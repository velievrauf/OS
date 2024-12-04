#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>  

#define RUN 32 

int compare(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

void insertionSort(int arr[], int left, int right) {
    for (int i = left + 1; i <= right; i++) {
        int key = arr[i];
        int j = i - 1;
        while (j >= left && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

void merge(int arr[], int left, int mid, int right) {
    int len1 = mid - left + 1, len2 = right - mid;
    int *leftArr = (int *)malloc(len1 * sizeof(int));
    int *rightArr = (int *)malloc(len2 * sizeof(int));

    for (int i = 0; i < len1; i++)
        leftArr[i] = arr[left + i];
    for (int i = 0; i < len2; i++)
        rightArr[i] = arr[mid + 1 + i];

    int i = 0, j = 0, k = left;
    while (i < len1 && j < len2) {
        if (leftArr[i] <= rightArr[j]) {
            arr[k] = leftArr[i];
            i++;
        } else {
            arr[k] = rightArr[j];
            j++;
        }
        k++;
    }

    while (i < len1) {
        arr[k] = leftArr[i];
        i++;
        k++;
    }

    while (j < len2) {
        arr[k] = rightArr[j];
        j++;
        k++;
    }

    free(leftArr);
    free(rightArr);
}

void timSort(int arr[], int n) {
    for (int i = 0; i < n; i += RUN)
        insertionSort(arr, i, (i + RUN - 1) < (n - 1) ? (i + RUN - 1) : (n - 1));

    for (int size = RUN; size < n; size = 2 * size) {
        for (int left = 0; left < n; left += 2 * size) {
            int mid = left + size - 1;
            int right = (left + 2 * size - 1) < (n - 1) ? (left + 2 * size - 1) : (n - 1);

            if (mid < right)
                merge(arr, left, mid, right);
        }
    }
}

typedef struct {
    int *arr;
    int start;
    int end;
} ThreadData;

void *sortArray(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    timSort(data->arr + data->start, data->end - data->start); 
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <array size> <number of threads>\n", argv[0]);
        return 1;
    }

    int arr_size = atoi(argv[1]);
    int max_threads = atoi(argv[2]);

    int *arr = (int *)malloc(arr_size * sizeof(int));
    if (!arr) {
        printf("Memory allocation failed\n");
        return 1;
    }
    for (int i = 0; i < arr_size; i++) {
        arr[i] = rand() % 1000;
    }

    pthread_t threads[max_threads];
    ThreadData threadData[max_threads];
    int segment_size = arr_size / max_threads;

    clock_t start_time = clock();

    for (int i = 0; i < max_threads; i++) {
        threadData[i].arr = arr;
        threadData[i].start = i * segment_size;
        threadData[i].end = (i == max_threads - 1) ? arr_size : (i + 1) * segment_size;

        pthread_create(&threads[i], NULL, sortArray, (void *)&threadData[i]);
    }

    for (int i = 0; i < max_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    timSort(arr, arr_size);

    clock_t end_time = clock();
    double time_taken = (double)(end_time - start_time) / CLOCKS_PER_SEC;

    printf("Time taken for sorting with %d threads: %.5f seconds\n", max_threads, time_taken);

    printf("Sorted Array: \n");
    for (int i = 0; i < arr_size; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");

    free(arr);
    return 0;
}
