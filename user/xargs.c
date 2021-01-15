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
