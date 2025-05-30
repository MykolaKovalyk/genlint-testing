#include <zephyr/kernel.h>
#include <stdio.h>

int main(void)
{
	k_sleep(K_SECONDS(5));

	printf("Hello, Zephyr!\n");

	return 0;
}