#include <stdio.h>

int main(int argc, char * argv[], char * envp[])
{
	for(int i = 0; argv[i] != NULL; i++)
	{
		printf("argv[%d]: %s\n",i,  argv[i]);
	}

	for(int i = 0; envp[i] != NULL; i++)
        {
                printf("argv[%d]: %s\n",i,  envp[i]);
        }

	return 0;

}
