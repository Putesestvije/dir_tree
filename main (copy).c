#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <string.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_LIGHT_BLUE    "\x1b[1;34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

typedef enum _tree_parts {BRANCH, SPACE, STEM} tree_parts;

typedef struct _dirs{
	DIR *stream;
	struct dirent *l_entry;
} dirs;

void error_upgrade(char *s){
	perror(s);
	exit(1);
}

void build_tree(char *start);

void print_layer(unsigned int stack_height, tree_parts* tp_stack, 
		 struct dirent *curr_ent, struct stat file_info,
		 int last_entry);

void print_entry(struct dirent *curr_ent, struct stat file_info, int last_entry);

struct dirent *next_valid_entry(dirs *dir_stack, unsigned int stack_height);

void exit_directory(dirs *dir_stack, char **name_stack,tree_parts *tp_stack, unsigned int stack_height);

void enter_directory(struct dirent *curr_ent, dirs **dir_stack, char ***name_stack, 
		     tree_parts **tp_stack, unsigned int stack_height);

char *generate_path(char **name_stack, unsigned int stack_height);

struct stat open_file(char **name_stack, unsigned int stack_height, struct dirent *curr_ent);

int main(int argc,char **argv)
{
	char *start;
	
	if (argc == 1)
		start = ".";
	else
		start = argv[1];
	
	if(access(start, F_OK)){
		perror("Couldn't open the given path");	
		exit(1);
	}
	build_tree(start);
	
	return 0;
}

void build_tree (char *start)
{
	struct dirent *curr_ent = NULL, *next_ent = NULL;
	struct stat file_info;
	int n_files = 0, n_folders = 0;
	int last_entry = 0;
	unsigned int stack_height = 0;
	tree_parts *tp_stack;	
	char **name_stack, *path;
	DIR **dir_stack;
	
	dirs *dir_stack2;
	
	name_stack = (char**)malloc(sizeof(char*));
	name_stack[stack_height] = start;
	
	
	dir_stack = (DIR**)malloc(sizeof(DIR*));
	dir_stack[stack_height] = opendir(start);
	
	dir_stack2 = (dirs*)malloc(sizeof(dirs));
	dir_stack2[stack_height].stream = opendir(start);
	
	tp_stack = (tree_parts*)malloc(sizeof(tree_parts));
	tp_stack[stack_height] = STEM;
	
	stack_height++;
	
	printf(ANSI_COLOR_LIGHT_BLUE "%s\n" ANSI_COLOR_RESET, start);
	
	curr_ent = next_valid_entry(dir_stack2, stack_height);
	next_ent = next_valid_entry(dir_stack2, stack_height);
	
	
	/*in case the command line argument was an empty directory*/
	if(curr_ent == NULL){
		return;
	}
	
	while(stack_height > 0){
		printf("new loop\n");
		printf("%d\n", stack_height);
		if (curr_ent == NULL){
			printf("SHAZAM\n");
			exit_directory(dir_stack2, name_stack, tp_stack, stack_height);
			stack_height--;
			if(stack_height){
				curr_ent = dir_stack2[stack_height-1].l_entry;
				next_ent = next_valid_entry(dir_stack2, stack_height);
			}
		}
		else{
			printf("%d\n", stack_height);
			file_info = open_file(name_stack, stack_height, curr_ent);
			printf("%d\n", stack_height);
			//next_ent = next_valid_entry(dir_stack, stack_height);
			if(next_ent == NULL)
				last_entry = 1;
			//print_layer(stack_height, tp_stack, curr_ent, file_info, last_entry);
			//file_info = open_file(name_stack, stack_height, curr_ent);
			if(S_ISDIR(file_info.st_mode)){
				if(last_entry)
					tp_stack[stack_height-1] = SPACE;
				else 
					tp_stack[stack_height-1] = BRANCH;
				
				dir_stack2[stack_height-1].l_entry = next_ent;
				stack_height++;
				enter_directory(curr_ent, &dir_stack2, &name_stack, &tp_stack, stack_height);
				
				
				
				curr_ent = next_valid_entry(dir_stack2, stack_height);
				
				next_ent = next_valid_entry(dir_stack2, stack_height);
				
			} else {
			curr_ent = next_ent;
			next_ent = next_valid_entry(dir_stack2, stack_height);
			}
		}
	}	
	return;
}

void print_layer(unsigned int stack_height, tree_parts* tp_stack, 
		 struct dirent *curr_ent, struct stat file_info,
		 int last_entry)
{
	for(int i = 0; i < stack_height; i++){
		switch (tp_stack[i]) {
			case BRANCH :
				printf("|   ");
				break;
			case SPACE : 
				printf("    ");
				break;
			case STEM :
				if(last_entry)
					printf("+---");
				else
					printf("+---");
				break;
		}
	}
	print_entry(curr_ent, file_info, last_entry);
}

void print_entry(struct dirent *curr_ent, struct stat file_info, int last_entry)
{
	mode_t mode = S_IFMT & file_info.st_mode;
	
	switch (mode){
		case S_IFDIR :
			printf(ANSI_COLOR_LIGHT_BLUE "%s\n" ANSI_COLOR_RESET, curr_ent->d_name);
			break;
		case S_IFLNK :
			printf(ANSI_COLOR_CYAN "%s\n" ANSI_COLOR_RESET, curr_ent->d_name);
			break;
		case S_IFCHR :
			printf(ANSI_COLOR_YELLOW "%s\n" ANSI_COLOR_RESET, curr_ent->d_name);
			break;
		default :
			printf("%s\n", curr_ent->d_name);
	}
}

struct dirent *next_valid_entry(dirs *dir_stack, unsigned int stack_height)
{
	struct dirent *result;
	
	/*printf("here 6 %ld\n", (long)dir_stack[stack_height-1].stream);*/
	result = readdir(dir_stack[stack_height - 1].stream);
	if (result == NULL)
		return result;
	
	while(result != NULL && (!strcmp(result->d_name, ".") || !strcmp(result->d_name, ".."))){
		result = readdir(dir_stack[stack_height-1].stream);
	}
	//printf("%d\n", stack_height);
	return result;
}

void exit_directory(dirs *dir_stack, char **name_stack,tree_parts *tp_stack, unsigned int stack_height)
{	
	if(stack_height > 1)
		free(name_stack[stack_height-1]);
	name_stack = (char**)realloc(name_stack, (stack_height-1)*sizeof(char*));
	closedir(dir_stack[stack_height-1].stream);
	tp_stack = (tree_parts*)realloc(tp_stack, (stack_height-1)*sizeof(tree_parts));
	if (stack_height > 1){
		tp_stack[stack_height-2] = STEM;
	}
}

void enter_directory(struct dirent *curr_ent, dirs **dir_stack, char ***name_stack, 
		     tree_parts **tp_stack, unsigned int stack_height)
{
	
	char *path;
	struct dirent *test;
	(*name_stack) = (char**)realloc((*name_stack), (stack_height)*sizeof(char*));
	(*name_stack)[stack_height-1] = strdup(curr_ent->d_name);
	path = generate_path((*name_stack), stack_height);	
	
	(*dir_stack) = (dirs*)realloc((*dir_stack), (stack_height)*sizeof(dirs));
	
	(*dir_stack)[stack_height - 1].stream = opendir(path);
	
	free(path);
	
	(*tp_stack) = (tree_parts*)realloc((*tp_stack), (stack_height)*sizeof(tree_parts));
	(*tp_stack)[stack_height -1] = STEM;	
	
	
	return;
}

char *generate_path(char **name_stack, unsigned int stack_height)
{
	
	unsigned int path_len;
	char *result;
	
	printf("%d before the calc\n", stack_height);
	printf("%s\n", name_stack[0]);
	for (int i = 0; i < stack_height; i++){
		printf("%d %d %s \n", i, stack_height, name_stack[i]);
		path_len += strlen(name_stack[i]) + 1; //+1 for /'s and the final '\0' 
	}
	printf("%d", stack_height);
	printf("bum\n");
	result = (char*)malloc(path_len*sizeof(char));
	
	strcpy(result, name_stack[0]);
	strcat(result, "/");
	
	for (int i = 1; i < stack_height; i++){
		strcat(result, name_stack[i]);
		strcat(result, "/");
	}
	
	return result;
}


struct stat open_file(char **name_stack, unsigned int stack_height, struct dirent *curr_ent)
{
	printf("%d\n", stack_height);
	char *path;
	struct stat file;
	
	path = generate_path(name_stack, stack_height);
	
	path = realloc(path, strlen(path) + strlen(curr_ent->d_name) + 1);
	strcat(path, curr_ent->d_name);
	/*printf("%s\n", path);*/
	
	if(stat(path, &file)){
		perror("Couldn't get file info");
		printf("in %s\n", path);
	}
	
	return file;
}