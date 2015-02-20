#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdbool.h>

int main() {
	char userinput;
	const int SIZE = 1024;
	char buff[SIZE];
	int file = open("/dev/simple_char_device", O_RDWR);
	bool cont = true;
while(cont) {

	printf("Press r to read from device\nPress w to write to the device\nPress e to exit from the device\nPress anything else to keep reading or writing from the device\n\nEnter Command: ");
	scanf("%c", &userinput);
	
	switch(userinput){
		case 'R':
		case 'r':
			read(file, buff, sizeof(buff));
			printf("\nDevice: %s\n", buff);
			break;
		case 'W':
		case 'w':
			printf("\nEnter data: ");
			scanf("%[^\n]", buff);
			write(file, buff, sizeof(buff));
			break;
		case'E':
		case'e':
			printf("Exited\n");
			cont = false;
			break;
	}

 	}

	close(file);

   return 0;
}
