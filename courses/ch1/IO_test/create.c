/*
open 系统调用:
int open(const char *pathname, int flags);
int open(const char *pathname, int flags, mode_t mode);

所需头文件: 
# include <sys/types.h>      // flags : 权限标记, Linux 里定义为宏, 在前两个头文件里
# include <sys/stat.h>       //
# include <fcntl.h>          // open 的函数声明

C 没有函数重载, 两个 open 通过可变参数实现: 
int open(const char *pathname, int flags, ...);

// 打开一个已存在文件
int open(const char *pathname, int flags);
    parameters:
        pathname : 要打开的文件路径
        flags    : 对文件的操作权限设置和其他设置
            必选 : O_RDONLY, O_WRONLY, or O_RDWR. 这三个互斥
            可选 : O_APPEND,  O_CLOEXEC,  O_CREAT,  O_DIRECTORY,  O_EXCL,  O_NOCTTY,  O_NOFOLLOW,  O_TMPFILE,  and O_TRUNC

    return value :
        成功返回新的文件描述符, 失败返回 -1 
    errno : 
        属于 Linux 系统函数库, 库里面的全局变量, 记录的是最近的错误号

perror函数
# include <stdio.h>
// 打印对应 errno 对应的错误描述
void perror(const char *s);
输出: s: system error message

// 创建一个新的文件
int open(const char *pathname, int flags, mode_t mode);
    parameters:
        pathname : 要打开的文件路径
        flags    : 对文件的操作权限设置和其他设置
            必选 : O_RDONLY, O_WRONLY, or O_RDWR. 这三个互斥
            可选 : O_CLOEXEC,  O_CREAT,  O_DIRECTORY,  O_EXCL,  O_NOCTTY,  O_NOFOLLOW,  O_TMPFILE,  and O_TRUNC
        mode: 八进制数, 表示创建出的文件操作权限, 比如 0775, 0 表示八进制
            最终的权限是 mode & ~umask
                
    return value :
        成功返回新的文件描述符, 失败返回 -1 
*/


# include <sys/types.h>      // flag : 权限标记, Linux 里定义为宏, 在前两个头文件里
# include <sys/stat.h>       //
# include <fcntl.h>          // open 的函数声明
# include <stdio.h>
# include <unistd.h>

int main() {

    int fd = open("create.txt", O_RDWR | O_CREAT, 0777); // 创建 create.txt

    if(fd == -1) {
        perror("open");
    }

    // 关闭文件描述符
    close(fd);

    return 0;
}