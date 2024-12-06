#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

#define SHARED_MEMORY_NAME "/my_shm"
#define SHARED_MEMORY_SIZE sizeof(struct SharedMemory)

struct SharedMemory {
    float numbers[100];
    int count;
    volatile int flag; 
    char result[256];  
};

int main() {
    int shm_fd = shm_open(SHARED_MEMORY_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    struct SharedMemory *shm_ptr = mmap(NULL, SHARED_MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    while (1) {
        if (shm_ptr->flag == 1) {  
            float result = shm_ptr->numbers[0];
            for (int i = 1; i < shm_ptr->count; i++) {
                if (shm_ptr->numbers[i] == 0) {
                    snprintf(shm_ptr->result, sizeof(shm_ptr->result), "Ошибка: деление на ноль");
                    shm_ptr->flag = 2;
                    return 0;
                }
                result /= shm_ptr->numbers[i];
            }

            snprintf(shm_ptr->result, sizeof(shm_ptr->result), "Результат: %.2f", result);
            shm_ptr->flag = 2;  
        }
        usleep(100000); 
    }

    munmap(shm_ptr, SHARED_MEMORY_SIZE);
    close(shm_fd);
    return 0;
}
