#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

int main() {
    int pipe1[2], pipe2[2];
    pid_t pid;
    
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    
    pid = fork();
    
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    
    if (pid == 0) { 
        close(pipe1[1]);  
        close(pipe2[0]);
        
        float numbers[100];
        int count = read(pipe1[0], numbers, sizeof(numbers));
        close(pipe1[0]);
        
        for (int i = 1; i < count / sizeof(float); i++) {
            if (numbers[i] == 0) {
                printf("Ошибка: деление на ноль\n");
                exit(EXIT_FAILURE);
            }
        }
        
        float result = numbers[0];
        for (int i = 1; i < count / sizeof(float); i++) {
            result /= numbers[i];
        }
        
        int file = open("result.txt", O_WRONLY | O_CREAT, 0644);
        if (file < 0) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        dprintf(file, "Результат: %.2f\n", result);
        close(file);
        
        exit(EXIT_SUCCESS);
        
    } else { 
        close(pipe1[0]);  
        close(pipe2[1]);  
        
        float numbers[100];
        int count = 0;
        printf("Введите числа (разделенные пробелами), завершите <endline>: ");
        while (scanf("%f", &numbers[count]) != EOF) {
            count++;
        }
        
        write(pipe1[1], numbers, sizeof(numbers[0]) * count);
        close(pipe1[1]);
        
        wait(NULL);  
    }
    
    return 0;
}
