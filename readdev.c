#include <unistd.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

int main()
{
	int df = open("/dev/keyboard_stats",O_RDONLY);
	if(df==-1)
		return 0;

	char buf[512];
	
	while(1)
	{
		printf("reading data \n");
		int err = read(df,buf,512);
		
		printf("key press ---->  %s \n",buf);
		memset(buf,0,512);
	}	
	close(df);
	
	
	return 0;
}
