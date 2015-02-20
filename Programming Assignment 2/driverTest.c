#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdbool.h>
#define DEVICE "/dev/simple_char_device"
#define BUFFER_SIZE 1024

int main () {
	char input, buff[BUFFER_SIZE];
	bool repeat = true;
	int file = open(DEVICE, O_RDWR);

	while(repeat)
     	{
 
	printf("\nR) Read from device\nW) Write to dvice\nE) Exit device\nAnything else to continue reading and writing\n\nEnter command: ");
	scanf("%c", &input);

        switch (input)
	{
        	case 'w': case 'W':
                    	printf("Enter data you want to write to the device: ");
                    	scanf("%s", buff);
                    	printf("Wrote %d bytes to device file\n", write(file, buff, BUFFER_SIZE));
 			while(getchar()!='\n');
               		break;
               	case 'r': case 'R':
                    	read(file, buff, BUFFER_SIZE);
                    	printf("Data read from the device: %s\n", buff);
        		while(getchar()!='\n');
            		break;
               	case 'e': case 'E':
                    	printf("Exiting\n");
                    	repeat = false;
                    	break;
		default:
			while(getchar()!='\n');
          }
     }
     close(file);
     return 0;
}
