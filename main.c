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

void enter_directory(dir **dir_stack, unsigned int stack_height, struct dirent *curr_ent);

char *generate_path(dir *dir_stack, unsigned int stack_height);

struct stat open_file(dir *dir_stack, struct dirent *curr_ent, unsigned int stack_height);

void init_stack(dir **dir_stack, char *start);

char *path_to_entry(dir *dir_stack, struct dirent *curr_ent, unsigned int stack_height);

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
	unsigned int n_files = 0, n_folders = 0;
	int last_entry = 0;
	unsigned int stack_height = 0;
	dir *dir_stack;
	init_stack(&dir_stack, start);
	char r, *full_path;
	
	stack_height++;
	
	printf(ANSI_COLOR_LIGHT_BLUE "%s\n" ANSI_COLOR_RESET, start);	
	
	curr_ent = next_valid_entry(dir_stack, stack_height);
	next_ent = next_valid_entry(dir_stack, stack_height);
	//printf("stack height: %d\n", stack_height);
	/*in case the command line argument was an empty directory*/
	if(curr_ent == NULL){
		return;
	}
	while (stack_height > 0){
		//scanf("%c", &r);/*for step by step execution*/ 
		if (curr_ent == NULL){/*when we decrease the stack*/
			stack_height--;
			if (stack_height == 0)
				break;
			exit_directory(&dir_stack, stack_height);
			curr_ent = dir_stack[stack_height-1].n_entry;
			next_ent = next_valid_entry(dir_stack, stack_height);			
			//printf("stack height: %d\n", stack_height);
		} else {
			if (next_ent == NULL)
				last_entry = 1;
			else 
				last_entry = 0;
			
			file_info = open_file(dir_stack, curr_ent, stack_height);
			//printf("inspecting: %s\n", curr_ent->d_name);
			print_layer(stack_height, dir_stack, curr_ent, file_info, last_entry);
			if(S_ISLNK(file_info.st_mode)){
				curr_ent = next_ent;
				next_ent = next_valid_entry(dir_stack, stack_height);
				n_files++;
			}
			else if(S_ISDIR(file_info.st_mode)){
				//printf("stack height: %d\n", stack_height);
				dir_stack[stack_height-1].n_entry = next_ent;
				if(last_entry)
					dir_stack[stack_height-1].tree_part = SPACE;
				else
					dir_stack[stack_height-1].tree_part = BRANCH;
				stack_height++;
				enter_directory(&dir_stack, stack_height, curr_ent);
				curr_ent = next_valid_entry(dir_stack, stack_height);
				next_ent = next_valid_entry(dir_stack, stack_height);
				n_folders++;
			} else {
				curr_ent = next_ent;
				next_ent = next_valid_entry(dir_stack, stack_height);
				n_files++;
			}
		}
	}	
	printf("\n%u directories, %u files\n", n_folders, n_files);
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
			printf(ANSI_COLOR_LIGHT_BLUE "%s -D\n" ANSI_COLOR_RESET, curr_ent->d_name);
			break;
		case S_IFLNK :
			printf("LINK :");
			printf(ANSI_COLOR_CYAN "%s\n" ANSI_COLOR_RESET, curr_ent->d_name);
			break;
		case S_IFCHR :
			printf(ANSI_COLOR_YELLOW "%s -B\n" ANSI_COLOR_RESET, curr_ent->d_name);
			break;
		default :
			printf("%s -R\n", curr_ent->d_name);
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

void exit_directory(dir **dir_stack, unsigned int stack_height)
{	
	dir *tmp;
	
	free((*dir_stack)[stack_height].name);
	
	closedir((*dir_stack)[stack_height].stream);
	
	tmp = realloc((*dir_stack), stack_height*sizeof(dir));
	if(tmp == NULL){
		printf("Error while reallocating stac: exit_directory\n");
		exit(1);
	}
	(*dir_stack) = tmp;
	
	(*dir_stack)[stack_height-1].tree_part = STEM;
}

void enter_directory(dir **dir_stack, unsigned int stack_height, struct dirent *curr_ent)
{
	dir *tmp;
	char *path;
	
	tmp = realloc((*dir_stack), stack_height*sizeof(dir));
	if (tmp == NULL){
		printf("Error while reallocating the directory stack\n");
		exit(1);
	} else
		(*dir_stack) = tmp;
	
	(*dir_stack)[stack_height-1].name = strdup(curr_ent->d_name);
	
	path = generate_path((*dir_stack), stack_height);
	
	//printf("Now entering: %s\n", path);
	
	(*dir_stack)[stack_height-1].stream = opendir(path);
	
	free(path);
	
	(*dir_stack)[stack_height-1].tree_part = STEM;
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
	char *full_path;
	int err;
		
	full_path = path_to_entry(dir_stack, curr_ent, stack_height);
	
	err = lstat(full_path, &info);
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

char *path_to_entry(dir *dir_stack, struct dirent *curr_ent, unsigned int stack_height){
	char *path, *full_path, *tmp;
	unsigned int full_len;
	
	path = generate_path(dir_stack, stack_height);
	full_len += strlen(path);
	full_len += strlen(curr_ent->d_name);
	full_len++;
	
	tmp = malloc(full_len * sizeof(char));
	if (tmp == NULL){
		printf("Error allocating space for full path: path_to_entry\n");
		exit(1);
	} else 
		full_path = tmp;
	
	strcpy(full_path, path);
	free(path);
	strcat(full_path, curr_ent->d_name);
	
	return full_path;
}