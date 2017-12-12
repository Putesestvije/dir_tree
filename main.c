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

#define P_STEM "├── "
#define P_ENDSTEM "└── "
#define P_BRANCH "│   "

typedef enum _tree_parts {BRANCH, SPACE, STEM} tree_parts;

typedef struct _dir{
	DIR *stream;
	tree_parts tree_part;
	char *name;
	struct dirent *n_entry;
} dir;

void error_upgrade(char *s){
	perror(s);
	exit(1);
}

void build_tree(char *start);

void print_layer(unsigned int stack_height, dir *dir_stack, 
		 struct dirent *curr_ent, struct stat file_info,
		 int last_entry);

void print_entry(struct dirent *curr_ent, struct stat file_info);

struct dirent *next_valid_entry(dir *dir_stack, unsigned int stack_height);

void exit_directory();

void enter_directory();

char *generate_path(dir *dir_stack, unsigned int stack_height);

struct stat open_file(dir *dir_stack, struct dirent *curr_ent, unsigned int stack_height);

void init_stack(dir **dir_stack, char *start);

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
	dir *dir_stack;
	init_stack(&dir_stack, start);
	
	stack_height++;
	
	printf(ANSI_COLOR_LIGHT_BLUE "%s\n" ANSI_COLOR_RESET, start);	
	
	curr_ent = next_valid_entry(dir_stack, stack_height);
	next_ent = next_valid_entry(dir_stack, stack_height);
	//next_ent = next_valid_entry();
		
	/*in case the command line argument was an empty directory*/
	if(curr_ent == NULL){
		return;
	}
	while (stack_height > 0){
		if (curr_ent == NULL){
			stack_height--;
		} else {
			if (next_ent == NULL)
				last_entry = 1;
			else 
				last_entry = 0;
			
			file_info = open_file(dir_stack, curr_ent, stack_height);
			//OVDE SI STAO
			curr_ent = next_ent;
			next_ent = next_valid_entry(dir_stack, stack_height);
		}
		
	}
	
	/*
	while(stack_height > 0){
		if (curr_ent == NULL){
			exit_directory();
			stack_height--;
			if(stack_height){
				curr_ent = dir_stack2[stack_height-1].l_entry;
				next_ent = next_valid_entry();
			}
		}
		else{
			file_info = open_file();
			if(next_ent == NULL)
				last_entry = 1;
			print_layer();
			if(S_ISDIR(file_info.st_mode)){
				if(last_entry)
					tp_stack[stack_height-1] = SPACE;
				else 
					tp_stack[stack_height-1] = BRANCH;
				dir_stack2[stack_height-1].l_entry = next_ent;
				stack_height++;
				enter_directory();
				
				curr_ent = next_valid_entry();
				next_ent = next_valid_entry();
			} else {
			curr_ent = next_ent;
			next_ent = next_valid_entry();
			}
		}
	}*/	
	return;
}

void print_layer(unsigned int stack_height, dir *dir_stack, 
		 struct dirent *curr_ent, struct stat file_info,
		 int last_entry)
{
	for(int i = 0; i < stack_height; i++){
		switch (dir_stack[i].tree_part) {
			case BRANCH :
				printf(P_BRANCH);
				break;
			case SPACE : 
				printf("    ");
				break;
			case STEM :
				if(last_entry)
					printf(P_ENDSTEM);
				else
					printf(P_STEM);
				break;
		}
	}
	print_entry(curr_ent, file_info);
}

void print_entry(struct dirent *curr_ent, struct stat file_info)
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

struct dirent *next_valid_entry(dir *dir_stack, unsigned int stack_height)
{
	struct dirent *result = readdir(dir_stack[stack_height-1].stream);
	
	if(result == NULL)
		return result;
	while(result != NULL && /*(strcmp(result->d_name, ".") == 0 || strcmp(result->d_name, "..") == 0)*/
		result->d_name[0] == '.'){
		result = readdir(dir_stack[stack_height-1].stream);
	}
	return result;
}

void exit_directory()
{	
	
}

void enter_directory()
{
	
}

char *generate_path(dir *dir_stack, unsigned int stack_height)
{
	char *path;
	unsigned int len;
	
	
	for(int i = 0; i < stack_height; i++){
		len += strlen(dir_stack[i].name) + 1; /*+1 for the /'s e.g dir/, something/, bla/*/
		len++;
	}
	
	path = malloc(len * sizeof(char));
	
	path[0] = '\0';
	
	for(int i = 0; i < stack_height; i++){
		strcat(path, dir_stack[i].name);
		strcat(path, "/");
	}
	
	return path;
}

struct stat open_file(dir *dir_stack, struct dirent *curr_ent, unsigned int stack_height)
{
	struct stat info;
	char *path, *full_path;
	unsigned int full_len;
	int err;
	
	
	
	path = generate_path(dir_stack, stack_height);
	
	full_len += strlen(path);
	full_len += strlen(curr_ent->d_name);
	full_len++; /* and one more for the \0'*/
	
	full_path = malloc(full_len * sizeof(char));
	
	strcpy(full_path, path);
	free(path);
	strcat(full_path, curr_ent->d_name);
	
	printf("%s\n", full_path);
	
	err = stat(full_path, &info);
	if(err != 0){
		perror("Error reading file");
		printf("at %s", full_path);
	}
	free(full_path);
	return info;
}

void init_stack(dir **dir_stack, char *start)
{
	dir* tmp;
	DIR *d;
	tmp = malloc(sizeof(dir));
	if (tmp == NULL){
		fprintf(stderr, "Error allocating a dir in init_stack");
	} else
		(*dir_stack) = tmp;
	
	(*dir_stack)->name = strdup(start);
	d = opendir(start);
	(*dir_stack)->stream = d;
	
	(*dir_stack)->tree_part = STEM;
	
	(*dir_stack)->n_entry = NULL;
}