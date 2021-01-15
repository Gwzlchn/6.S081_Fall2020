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
