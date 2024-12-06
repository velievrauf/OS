#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/wait.h>

#define SHARED_MEMORY_NAME "/my_shm"
#define SHARED_MEMORY_SIZE sizeof(struct SharedMemory)

struct SharedMemory {
    float numbers[100];
    int count;
    volatile int flag; 
    char result[256];   
};

int main() {
    int shm_fd = shm_open(SHARED_MEMORY_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    
    if (ftruncate(shm_fd, SHARED_MEMORY_SIZE) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    struct SharedMemory *shm_ptr = mmap(NULL, SHARED_MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {  
        close(shm_fd);  

        execl("./kid", "kid", NULL);
        perror("execl");
        exit(EXIT_FAILURE);
    } else { 
        close(shm_fd);  

        printf("Введите числа (разделенные пробелами), завершите <endline>: ");
        float numbers[100];
        int count = 0;
        
        while (scanf("%f", &numbers[count]) != EOF) {
            count++;
        }

        memcpy(shm_ptr->numbers, numbers, sizeof(float) * count);
        shm_ptr->count = count;
        shm_ptr->flag = 1;  

        while (shm_ptr->flag != 2) {
            usleep(100000);  
        }

        printf("Результат: %s\n", shm_ptr->result);

        munmap(shm_ptr, SHARED_MEMORY_SIZE);
        shm_unlink(SHARED_MEMORY_NAME);

        wait(NULL);
    }

    return 0;
}
