# Linux 系统编程入门

## gcc 编译命令

```shell
gcc [FileName] [-o ProgramName]

# 常用编译选项
[-o] 目标文件名
[-E] 预处理，生成预处理后的代码，不编译
[-S] 编译，生成汇编代码，不汇编
[-c] 汇编，生成目标代码，但是不进行链接
[-I directory] 指定 include 包含文件的搜索目录
[-g] 在编译的时候，生成调试信息，该程序可以被调试器调试
[-D] 编译的时候定义一个宏
[-l] 链接的库
```

g++ 在编译的阶段调用 gcc，但是 gcc 不能自动和 c++ 程序使用的库链接，除非添加编译选项 `gcc -lstdc++`。

## 静态库和动态库

### 静态库的制作

1. 命名规则：
   
   1. Linux：libxxx.a
   2. Windows：libxxx.lib



2. 静态库的制作
    
    1. gcc 编译生成 -o 汇编文件
        
        `gcc -c a.c b.c`

    2. 对汇编文件打包，创建静态库

        `ar rcs libxxx.a a.o b.o`


### 静态库的使用

```shell
gcc FileName -l xxx
```
`xxx` 为 库名称，而非库文件名称

### 动态库的制作

1. 命名规则：
   
   1. Linux：libxxx.so
   2. Windows：libxxx.dll

2. 动态库的制作
    
    1. gcc 编译生成 -o 汇编文件
        
        `gcc -c -fpic/-fPIC a.c b.c`

    2. gcc 制作动态库

        `gcc -shared *.o -o libxxx.so`

### 动态库的使用

1. 程序运行时动态库加载失败的原因：

    系统加载可执行代码的时候，除了需要知道所依赖的动态库的名称，还需要知到其绝对路径，此时需要系统的动态载入器来获取其绝对路径。

2. 解决方法：

    1. 添加环境变量，只在当前终端生效（临时）
        
        `export LD_LIBRARY_PATH = $LD_LIBRARY_PATH:libso_path`

    2. 添加环境变量，在`~/.bashrc`中添加（用户级别）
        
        `export LD_LIBRARY_PATH = $LD_LIBRARY_PATH:libso_path`
        
        使环境变量生效

        `source ~/.bashrc` 或者 `，~/.bashrc`
    
    3. 添加环境变量，在`/etc/profile`中添加（系统级别）
        
        `export LD_LIBRARY_PATH = $LD_LIBRARY_PATH:libso_path`
        
        使环境变量生效

        `source /etc/profile` 或者 `，/etc/profile`
    
    4. 修改 `/etc/ld.so.cache`，二进制文件，不能直接修改，通过 `/etc/ld.so.conf` 修改，直接添加`libso_path`，`ldconfig` 使修改生效。


    5. 将动态库文件放在 `/lib/` 或者 `/usr/lib/` 目录下（不建议），可能会替换同名的系统库文件。


### 静态库 vs. 动态库

1. 静态库
   
     - 优点：
        
        a. 加载速度快
        
        b. 发布程序无需提供静态库，移植方便
    
     - 缺点：
        
        a. 消耗系统资源，浪费内存
        
        b. 更新、部署、发布麻烦

2. 动态库
   
     - 优点：
        
        a. 可以实现进程间的资源共享
        
        b. 更新、部署、发布简单

        c. 可以控制核实加载动态库
    
     - 缺点：
        
        a. 加载速度比静态库慢
        
        b. 发布程序是需要提供依赖的动态库


## Makefile

### 什么是 Makefile

- 工程的源文件根据类型、功能、模块分别放在不同的目录中，Makefile 文件定义了以系列规则来指定哪些文件先编译，哪些文件后编译，哪些文件需要重新编译，甚至更复杂的功能。
- Makefile 带来的好处：“自动化编译”，make 是解释 Makefile 的命令工具。

### Makefile 文件命名和规则

1. 文件命名：

    Makefile 或者 makefile

2. 规则：

```makefile
目标 ... : 依赖...
    命令 (shell 命令)
    ...

目标: 最终要生成的文件 (伪目标除外)
依赖: 生成目标所需要的文件或是目标
命令: 通过执行命令对依赖操作生成目标 (前有缩进)

Makefile 文件里的其他规则一般是为第一条规则服务的. 
```

### 例子

```makefile
# 1
app:sub.c add.c mult.c div.c main.c
	gcc sub.c add.c mult.c div.c main.c -o app
```

```makefile
# 2
app:sub.o add.o mult.o div.o main.o
	gcc sub.o add.o mult.o div.o main.o -o app

sub.o:sub.c
	gcc -c sub.c -o sub.o

add.o:add.c
	gcc -c add.c -o add.o

mult.o:mult.c
	gcc -c mult.c -o mult.o

div.o:div.c
	gcc -c div.c -o div.o

main.o:main.c
	gcc -c main.c -o main.o
```

```makefile
# 3
#定义变量
src=sub.o add.o mult.o div.o main.o
target=app
$(target):$(src)
	$(CC) $(src) -o $(target)

sub.o:sub.c
	gcc -c sub.c -o sub.o

add.o:add.c
	gcc -c add.c -o add.o

mult.o:mult.c
	gcc -c mult.c -o mult.o

div.o:div.c
	gcc -c div.c -o div.o

main.o:main.c
	gcc -c main.c -o main.o
```

```makefile
# 4
#定义变量
src=sub.o add.o mult.o div.o main.o
target=app
$(target):$(src)
	$(CC) $(src) -o $(target)

%.o:%.c
	$(CC) -c $< -o $@
```

```makefile
# 5
#定义变量
# add.c sub.c main.c mult.c div.c
src=$(wildcard ./*.c)
objs=$(patsubst %.c, %.o, $(src))
target=app
$(target):$(objs)
	$(CC) $(objs) -o $(target)

%.o:%.c
	$(CC) -c $< -o $@

.PHONY:clean
clean:
	rm $(objs) -f
```

## GDB 调试

命令:

向可执行程序中加入调试信息. 

```shell
gcc -g test.c -o test
```

启动/退出 gdb:
```shell
gdb [可执行文件] # 启动

(gdb) q/quit # 退出 gdb
```


gdb 常用命令: 
```makefile
# 给程序设置参数/获取设置参数
(gdb) set args 10 20    # 设置函数参数
(gdb) show args         # 获取函数参数

(gdb) help [command] # 查看帮助文档

# 查看当前文件代码
(gdb) list/l        # 从默认位置显, 默认10行
(gdb) list/l line   # 从指定的行显示, 行号上下文
(gdb) list/l func   # 从指定的函数显示, func上下文

# 查看非当前文件代码
(gdb) list/l filename : line
(gdb) list/l filename : func

# 设置显示的行数
(gdb) show list/listsize
(gdb) set list/listsize line

# 设置断点
(gdb) b/break line
(gdb) b/break func
(gdb) b/break filename : line
(gdb) b/break filename : func

# 查看断点
(gdb) i/info b/break

# 删除断点
(gdb) d/del/delete brk_num

# 设置断点无效
(gdb) dis/disable brk_num

# 设置断点生效
(gdb) en/enable brk_num

# 设置条件断点 (一般用在循环位置)
(gdb) b/break 10 if i==5

# 运行GDB程序
(gdb) start     # 停在main第一行
(gdb) run       # 停在第一个断点

# 继续运行
(gdb) c/continue    # 下一个断点
(gdb) n/next        # 下一行, 不会进入函数体
(gdb) s/step        # 单步调试, 会进入函数体
(gdb) finish        # 跳出函数体

# 变量操作
(gdb) p/print variable      # 打印变量值
(gdb) ptype   variable      # 打印变量类型

# 自动变量操作
(gdb) display variable # 自动打印指定变量
(gdb) i/info display
(gdb) undisplay no.

# 其他操作
(gdb) set var variable = value  # 设置变量的值
(gdb) until                     # 跳出循环
```
gdb 不输入命名按回车默认执行上一条命令
