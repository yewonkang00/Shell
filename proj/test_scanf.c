#include <stdio.h>
#include <unistd.h>

int main() {
	char buf[100];
	scanf("%s", buf);
	printf("%s\n", buf);
	sleep(20);
	return 0;
}
