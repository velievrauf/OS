#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <string.h>

#define SHARED_MEMORY_SIZE 1024

struct SharedMemory {
    float numbers[100];
    int count;
    volatile int flag;  
    char result[256];    
};

int main() {
    int shm_fd = shm_open("/my_shm", O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    
    if (ftruncate(shm_fd, sizeof(struct SharedMemory)) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    struct SharedMemory *shm_ptr = mmap(NULL, sizeof(struct SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
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

        return 0;
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

        printf("%s\n", shm_ptr->result);

        munmap(shm_ptr, sizeof(struct SharedMemory));
        shm_unlink("/my_shm");

        wait(NULL);
    }

    return 0;
}
