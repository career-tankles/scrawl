#include <febird/lcast.cpp>
#include <stdio.h>

using namespace febird;

#define PRINT(x) printf("%s:%x\n", x, (short)hexlcast(x))

int main() {
	PRINT("1234");
	PRINT("7f");
	PRINT("ffff");
	PRINT("ffef");
	PRINT("efef");
	return 0;
}

