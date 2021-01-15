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
