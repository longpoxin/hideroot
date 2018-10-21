#include <stdio.h>
#include <unistd.h>

#include "log.h"

int main(int angc, char** argv) {
	while(1) {
		if (access("/sbin/su", F_OK) == 0) {
			DEBUG_PRINT("Can access /sbin/su!\n");
		}
		else {
			DEBUG_PRINT("Can not access /sbin/su!\nS");
		}
		sleep(3);
	}
}