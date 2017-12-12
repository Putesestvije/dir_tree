#include <stdio.h>
#include <locale.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

static char *stem = "├── ";

int main(){
	for (int i = 0; i < 256; i++){
		if (! (i % 4))
			printf("\n");
		printf("%d: %c\t", i, i);
	}
	
	/*setlocale (LC_ALL,"");*/
	
	/*unsigned char i = 'r';*/
	
	printf (ANSI_COLOR_RED "\n|\n|\n|\n+----+\n|    |\n|    +-----" ANSI_COLOR_RESET);
	return 0;
}