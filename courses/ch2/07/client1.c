#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main() {


    // 1. 判断文件是否存在
    int ret = access("fifo1", F_OK);
    if(ret == -1) {
        printf("管道不存在，创建管道\n");
        
        ret = mkfifo("fifo1", 0664);

        if(ret == -1) {
            perror("mkfifo");
            exit(0);
        }

    }

    ret = access("fifo2", F_OK);
    if(ret == -1) {
        printf("管道不存在，创建管道\n");
        
        ret = mkfifo("fifo2", 0664);

        if(ret == -1) {
            perror("mkfifo");
            exit(0);
        }

    }

    // 2 .以只写的方式打开管道fifo1
    int fdw = open("fifo1", O_WRONLY);
    if(fdw == -1) {
        perror("open");
        exit(0);
    }

    printf("打开 fifo1 成功，准备写入...\n");

    // 3 .以只读的方式打开管道fifo2，非阻塞
    int fdr = open("fifo2", O_RDONLY);
    if(fdr == -1) {
        perror("open");
        exit(0);
    }

    printf("打开 fifo2 成功，等待写入...\n");

    // 4. 创建子进程，父进程写，子进程读，循环写读数据

    pid_t pid = fork();
    
    char buf[128];
    if(pid > 0) {
        // parent
        while (1){
            memset(buf, 0, 128);
            // 获取标准输入的数据
            fgets(buf, 128, stdin);
            // 写数据
            ret = write(fdw, buf, strlen(buf));
            if(ret == -1) {
                perror("write");
                close(fdw);
                close(fdr);
                exit(0);
            }
        }
    } else if (pid == 0){
        // child
        while (1){
            memset(buf, 0, 128);

            // 读数据
            ret = read(fdr, buf, 128);
            if(ret <= 0) {
                perror("read");
                close(fdw);
                close(fdr);
                exit(0);
            }

            printf("recv: %s", buf);
        }
        
    } else {
        perror("fork");
        close(fdw);
        close(fdr);
        exit(-1);
    }

    close(fdw);
    close(fdr);

    return 0;
}