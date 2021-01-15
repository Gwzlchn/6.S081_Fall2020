# Fall2020/6.S081 实验笔记（一） Lab1: Xv6 and Unix utilities

开始日期：2021/01/03
完成日期: 2021/01/04



## 实验一： `sleep`

第一个实验让我们利用系统调用，来实现用户态调用`sleep`函数。

拿到题目一看，以为会很难，是让自己写这个`sleep`的系统调用，后来一看`user/user.h`，发现这个系统调用已经被实现了，自己的代码里面直接调`user/user.h` 里面的 `sleep`就行。把难度直接降为0。

```cpp
#include "../kernel/types.h"
#include "../user/user.h"

int main(int argc, char *argv[])
{

	if (argc != 2)
	{
		fprintf(2, "Usage: sleep ticks...\n");
		exit(1);
	}
	int ticks = atoi(argv[1]);
	sleep(ticks);
	exit(0);
}
```

## 实验二：`pingpong`

第二个实验让我们利用管道实现父子进程的通信。简单来说，就是调`fork()`创建子进程，再通过`pipe()`传递参数。

下面几点供参考：

1. `fork()` 函数：
   1. 对于父进程返回的是子进程的`PID`，对于子进程返回的是0。
   2. 为什么是这样？  
   参考杨力祥先生的《Linux内核设计的艺术》 第三章 + 赵炯先生的《Linux 内核 0.11 完全注释-V3.0》 第八章第10节。

2. 管道 `pipe()`
   1. 函数定义 [pipe(2) — Linux manual page](https://man7.org/linux/man-pages/man2/pipe.2.html)

   2. `pipe()` 一个小例子：[CSDN](https://blog.csdn.net/MyArrow/article/details/9037135)

3. 读写`pipe` 用到的 `write()`, `read()`
    1. `read`: [read(2) — Linux manual page](https://man7.org/linux/man-pages/man2/read.2.html)
    2. `write`: [write(2) — Linux manual page](https://man7.org/linux/man-pages/man2/write.2.html)

4. 父进程等子进程退出: `wait()`
    1. `wait()` : [wait(2) — Linux manual page](https://man7.org/linux/man-pages/man2/waitid.2.html)
    注意：`wait(0)`表示父进程等所有子孙进程。因为父进程创建子进程的时候，`group ID` 保持不变。

    >  0  :     meaning wait for any child process whose process group ID
    >              is equal to that of the calling process at the time of the
    >             call to waitpid().

以下是代码实现：

```cpp
#include "../kernel/types.h"
#include "../user/user.h"

int main(int argc, char **argv)
{
	char buf;
	// pipefd[0] read end
	// pipefd[1] write end
	int pipefd[2];
	if (pipe(pipefd) == -1)
	{
		fprintf(2, "Cannot create a pipe\n");
		exit(-1);
	}
	int child_pid = fork();
	if (child_pid == -1)
	{
		fprintf(2, "Cannot fork a child process\n");
		exit(-1);
	}

	// parent process
	// 1st: send a byte to child
	// 2nd: recv a byte from child
	if (child_pid > 0)
	{
		char p_send = 'a';
		write(pipefd[1], &p_send, sizeof(p_send));
		close(pipefd[1]);

		// wait child exit
		wait(0);

		read(pipefd[0], &buf, sizeof(buf));
		printf("%d: received pong\n", getpid(), buf);
		close(pipefd[0]);
	}
	// child process
	// 1st: recv a byte from parent
	// 2nd: send a byte to parent
	if (child_pid == 0)
	{
		read(pipefd[0], &buf, sizeof(buf));
		printf("%d: received ping\n", getpid());

		char c_send = 'b';
		write(pipefd[1], &c_send, sizeof(c_send));

		close(pipefd[0]);
		close(pipefd[1]);
		exit(0);
	}
	exit(0);
}

```


## 实验三：`primes`

第三个实验让我们用 Eratosthenes 方法筛素数，这个筛素数的过程这里就不赘述了。重点是，如何用父子进程+管道来完成素数筛？

思路：当前进程从父进程创建的管道读端读入所有的数，读到的第一个数必然是素数，输出。然后将余下的数，刨去那些第一个数做因子的和数，发送给新管道的写端。

参考这个网页的图片部分: [Bell Labs and CSP Threads](https://swtch.com/~rsc/thread/)

以下是代码实现：
```cpp
#include "../kernel/types.h"
#include "../user/user.h"

void primeproc(int *fd)
{

	close(fd[1]);
	int n, next_pipefd[2];

	int int_bytes = sizeof(int);
	pipe(next_pipefd);
	// 从 fd 读入父进程的输出
	if (read(fd[0], &n, int_bytes) == int_bytes)
	{
		printf("prime %d\n", n);
		int child_pid = fork();
		if (child_pid != 0)
		{

			primeproc(next_pipefd);
			exit(0);
		}
		// 把筛过的数传给自己的子进程
		else
		{
			// parent process
			close(next_pipefd[0]);
			int temp;
			while (read(fd[0], &temp, int_bytes) == int_bytes)
			{
				if ((temp % n) != 0)
				{
					write(next_pipefd[1], &temp, int_bytes);
				}
			}
			close(next_pipefd[1]);
			wait(0);
		}
	}
}
// pipefd[0] refers to the read end
// pipefd[1] refers to the write end
int main(int argc, char **argv)
{

	int pipefd[2], err;
	if ((err = pipe(pipefd)) < 0)
	{
		fprintf(2, "cannot create a pipe\n");
		exit(-1);
	}
	int c_pid = fork();
	if (c_pid == 0)
	{
		primeproc(pipefd);
		exit(0);
	}
	close(pipefd[0]);

	// feed all intngers through pipe
	int limit = 35;
	if (argc == 2)
	{
		limit = atoi(argv[1]);
	}
	for (int i = 2; i <= limit; i++)
	{
		int cur_write_bytes = 0;
		if ((cur_write_bytes = write(pipefd[1], &i, 4)) != 4)
		{
			fprintf(2, "cannot write interger %d to pipe, %d bytes", i, cur_write_bytes);
		}
	}
	close(pipefd[1]);
	wait(0);

	exit(0);
}

```

## 实验四： `find`

思路：
1. 获取 + 格式化处理文件名：
   1. 参考`kernel/fs.h` 文件，获取文件名。
   2. 参考`user/ls.c`文件，格式化文件名。 

2. 判断当前文件是文件夹还是纯文件：
   1. 参考`kernel/stat.h` 文件，利用 `stat` 函数获取当前文件的 `stat` 中的 `type` 信息。

下面是代码实现：
```cpp
#include "../kernel/types.h"
#include "../kernel/stat.h"
#include "../user/user.h"
#include "../kernel/fs.h"

void find(char *base_dir, char *to_find_file)
{
	int fd;
	struct stat st;
	struct dirent de;

	if ((fd = open(base_dir, 0)) < 0)
	{
		fprintf(2, "find: cannot access path %s\n", base_dir);
		return;
	}
	if ((fstat(fd, &st) < 0) || (st.type != T_DIR))
	{
		fprintf(2, "find: cannot stat path %s", base_dir);
		return;
	}

	char buf[512], *buf_ptr;
	if (strlen(base_dir) + 1 + DIRSIZ + 1 > sizeof(buf))
	{
		printf("find: path too long\n");
		return;
	}
	strcpy(buf, base_dir);
	buf_ptr = buf + strlen(buf);
	// buf ptr 指向当前 buf 字符串的尾端
	*buf_ptr++ = '/';
	while (read(fd, &de, sizeof(de)) == sizeof(de))
	{
		if (de.inum == 0)
			continue;
		// strcmp return
		// 0 -> the contents of both strings are equal
		if (!strcmp(de.name, ".") || !strcmp(de.name, ".."))
		{
			continue;
		}
		// 拼接当前文件名
		memmove(buf_ptr, de.name, DIRSIZ);
		*(buf_ptr + DIRSIZ) = 0;

		// 当前文件是文件，匹配名字
		// 当前文件是文件夹，递归查找
		if (stat(buf, &st) < 0)
		{
			printf("find: connot stat file %s\n", buf);
			continue;
		}
		switch (st.type)
		{
		case T_FILE:
			if (!strcmp(de.name, to_find_file))
			{
				printf("%s\n", buf);
			}
			break;

		case T_DIR:
			//printf("it a dir\n");
			find(buf, to_find_file);
			break;
		}
	}
	return;
}

int main(int argc, char *argv[])
{

	if (argc < 3)
	{
		fprintf(2, "Usage: find dir filename\n");
		exit(0);
	}
	char *base_dir = argv[1];
	char *to_find_file = argv[2];
	find(base_dir, to_find_file);

	exit(0);
}
```   

## 实验五：`xargs`

说实话，做实验之前我都不知道 `xargs` 是干嘛用的。所以得先了解一下 `xargs` 的作用，参考[阮一峰的博客](https://www.ruanyifeng.com/blog/2019/08/xargs-tutorial.html)

其中，第二段的一句话非常关键；
> xargs命令的作用，是将标准输入转为命令行参数
所以我们实验的整体思路，就是读入标准输入，转成多个命令行参数。

只要把握住这句话，这个实验代码就呼之欲出了。


还有一点需要注意的是：`int exec(char*, char**)` 这个函数的第二个参数接受一个`char` 类型二维指针，其中第一个指针指向可执行文件名。


下面给出参考代码：
```cpp
#include "../kernel/types.h"
#include "../user/user.h"
#include "../kernel/param.h"

#define MAX_STDIN 512

void fmt_args(char *read_in, int len, char **args, int *args_cnt)
{

	// 将一行标准输入转为多个参数
	char cur_buf[MAX_STDIN];
	int cur_buf_len = 0;

	int i;
	for (i = 0; i <= len; i++)
	{
		if ((read_in[i] == ' ' || read_in[i] == '\n') && cur_buf_len)
		{
			// 读到了一个新参数
			args[*args_cnt] = malloc(cur_buf_len + 1);
			memcpy(args[*args_cnt], cur_buf, cur_buf_len);
			args[*args_cnt][cur_buf_len] = 0;
			cur_buf_len = 0;
			(*args_cnt)++;
		}
		else
		{
			cur_buf[cur_buf_len] = read_in[i];
			cur_buf_len++;
		}
	}
}

int main(int argc, char **argv)
{
	char stdin_buf[MAX_STDIN];
	int result;

	char *args[MAXARG + 1];
	int args_cnt;

	// 读 xargs 后面的命令行参数
	for (int i = 1; i < argc; i++)
	{
		args[args_cnt] = malloc(sizeof(argv[i]));
		memcpy(args[args_cnt++], argv[i], sizeof(argv[i]));
	}
	// 读从管道传来的标准输入
	if ((result = read(0, stdin_buf, sizeof(stdin_buf))) > 0)
	{

		fmt_args(stdin_buf, strlen(stdin_buf), args, &args_cnt);
	}

	exec(args[0], args);

	exit(0);
}

```

## 测试分数

好了，这样就完成了第一个实验。

```bash
$ ./grade-lab-util
make: 'kernel/kernel' is up to date.
== Test sleep, no arguments == sleep, no arguments: OK (1.7s)
== Test sleep, returns == sleep, returns: OK (0.5s)
== Test sleep, makes syscall == sleep, makes syscall: OK (0.8s)
== Test pingpong == pingpong: OK (1.0s)
== Test primes == primes: OK (1.0s)
== Test find, in current directory == find, in current directory: OK (1.0s)
== Test find, recursive == find, recursive: OK (1.1s)
== Test xargs == xargs: OK (1.0s)
== Test time ==
time: OK
Score: 100/100
```



