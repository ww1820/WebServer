/*  
    标准C库的函数
    #include <stdio.h>
    int fseek(FILE *stream, long offset, int whence);

    Linux系统函数，重置文件指针偏移量
    #include <sys/types.h>
    #include <unistd.h>
    off_t lseek(int fd, off_t offset, int whence);
        参数：
            - fd：文件描述符，通过open得到的，通过这个fd操作某个文件
            - offset：偏移量，偏移量一旦超出了文件的长度，下一次进行文件IO写入操作文件的时候便会延续偏移量的位置继续写入，进而在文件中间产生了空洞的部分，这部分会以”\0”填充，而从原来的文件结尾到新写入数据间的这段空间就被称为“文件空洞”。
            - whence:
                SEEK_SET
                    设置文件指针的偏移量
                SEEK_CUR
                    设置偏移量：当前位置 + 第二个参数offset的值
                SEEK_END
                    设置偏移量：文件大小 + 第二个参数offset的值
        返回值：返回当前位置到文件开始位置以字节为单位的偏移量


    作用：
        1.移动文件指针到文件头
        lseek(fd, 0, SEEK_SET);

        2.获取当前文件指针的位置
        lseek(fd, 0, SEEK_CUR);

        3.获取文件长度
        lseek(fd, 0, SEEK_END);


*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

int main() {

    //原始文件内容: hello,world
    int fd = open("hello.txt", O_RDWR);

    if(fd == -1) {
        perror("open");
        return -1;
    }

    // 扩展文件的长度
    int ret = lseek(fd, 10000000, SEEK_END);
    if(ret == -1) {
        perror("lseek");
        return -1;
    }

    // 在offset = 10000011处写入一个数据
    //  write(fd, ".", SEEK_CUR);

    // 关闭文件
    close(fd);

    return 0;
}
