#include <stdio.h>

int main(void)
{
	//Test sys_helloworld system call
	syscall(318);

	//Test sys_simple_add system call
	int number1, number2, result;

	//Get user input
	printf("Input two integers: ");
	scanf("%d %d", &number1, &number2);

	//Pass system call ID and parameters
	syscall(319, number1, number2, &result);
	printf("%d + %d = %d\n", number1, number2, result);
	
	return 0;
}