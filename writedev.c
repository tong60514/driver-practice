#include <unistd.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main()
{
	int df = open("/dev/keyboard_stats",O_RDWR);
	if(df==-1)
		return 0;

	char buf[10];
	
	char c = 'e';
		write(df,&c,1);
			
	close(df);
	
	
	return 0;
}
