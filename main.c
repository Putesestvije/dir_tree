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

/*ansi red*/
#define ALIZARIN_CRIMSON  	"\x1b[31m"
/*ansi light red*/
#define BRIGHT_RED	  	"\x1b[1;31m"
/*ansi green*/
#define PHTALO_GREEN   		"\x1b[32m"
/*ansi light green*/
#define SAP_GREEN   		"\x1b[1;32m"
/*ansi yellow*/
#define YELLOW_OCHRE  		"\x1b[33m"
/*ansi light yellow*/
#define CADMIUM_YELLOW 		"\x1b[1;33m"
/*ansi blue*/
#define PRUSSIAN_BLUE		"\x1b[34m"
/*ansi light blue*/
#define PHTALO_BLUE		"\x1b[1;34m"
/*anis magenta mufugga*/
#define MAUVE			"\x1b[35m"
/*ansi light magenta*/
#define MAGENTA			"\x1b[1;35m"
/*ansi cyan*/
#define VIRIDIAN_GREEN		"\x1b[36m"
/*ansi light cyan*/
#define TURQUOISE		"\x1b[1;36m"
/*ansi color reset*/
#define BEAT_THE_DEVIL_OUTTA_TTY   "\x1b[0m"

#define P_STEM "├── "
#define P_ENDSTEM "└── "
#define P_BRANCH "│   "

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
         int last_entry, char *full_path);

void print_entry(char *name, struct stat file_info, char *full_path);

struct dirent *next_valid_entry(dir *dir_stack, unsigned int stack_height);

void exit_directory();

void enter_directory(dir **dir_stack, unsigned int stack_height, struct dirent *curr_ent);

char *generate_path(dir *dir_stack, unsigned int stack_height);

struct stat open_file(dir *dir_stack, struct dirent *curr_ent, unsigned int stack_height);

void init_stack(dir **dir_stack, char *start);

char *path_to_entry(dir *dir_stack, struct dirent *curr_ent, unsigned int stack_height);

int main(int argc,char **argv)
{
    int er;
    char *start;
    struct stat info;
    if (argc < 2){
        start = ".";
        build_tree(start);
    }
    else {
        for(int i = 1; i < argc; i++) {
            er = stat(argv[i], &info);
            if (er == -1){
                perror("Coudln't open file");
                printf("File is :%s\n", argv[i]);
            }
            if(S_ISDIR(info.st_mode)){
                start = argv[i];
                if(access(start, F_OK)){
                    perror("Couldn't open the given path");
                    exit(1);
                }
                build_tree(start);
            } else {
                printf("%s is not a directory or a link to a directory!\n", argv[i]);
            }
        }
    }
    return 0;
}

void build_tree (char *start)
{
    struct dirent *curr_ent = NULL, *next_ent = NULL;
    struct stat file_info, tmp;
    unsigned int n_files = 0, n_folders = 0;
    int last_entry = 0;
    unsigned int stack_height = 0;
    dir *dir_stack;
    init_stack(&dir_stack, start);
    char r, *full_path, *test, *link;
    off_t link_l;


    stack_height++;

    printf(PHTALO_BLUE "%s\n" BEAT_THE_DEVIL_OUTTA_TTY, start);

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
            full_path = path_to_entry(dir_stack, curr_ent, stack_height);
            print_layer(stack_height, dir_stack, curr_ent, file_info, last_entry, full_path);
            free(full_path);
            if(S_ISLNK(file_info.st_mode)){
                test = path_to_entry(dir_stack, curr_ent, stack_height);
                lstat(test, &tmp);
                link_l= tmp.st_size;

                link = malloc(link_l * sizeof(char));
                readlink(test, link, link_l);
                link[link_l] = '\0';
                stat(link, &tmp);
                ;
                //tmp = open_file(dir_stack, curr_ent, stack_height);
                if(S_ISDIR(tmp.st_mode)){
                    //printf(" directory\n");
                    n_folders++;
                } else {
                    //printf(" file\n");
                    n_files++;
                }
                //printf("A link found\n");
                curr_ent = next_ent;
                next_ent = next_valid_entry(dir_stack, stack_height);
                free(test);
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
         int last_entry, char *full_path)
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
    print_entry(curr_ent->d_name, file_info, full_path);
}

void print_entry(char *name, struct stat file_info, char *full_path)
{
    mode_t mode = S_IFMT & file_info.st_mode;
    int err;
    ssize_t r;
    off_t link_len = 0;
    char *link_path = NULL, *tmp = NULL;
    struct stat link_info, sym_info;

    switch (mode){
        case S_IFDIR :
            printf(PHTALO_BLUE "%s -D\n" BEAT_THE_DEVIL_OUTTA_TTY, name);
            break;
        case S_IFLNK :
            printf(TURQUOISE "%s" BEAT_THE_DEVIL_OUTTA_TTY, name);
            printf(" -> ");

            /*now printing the file pointed to*/
                //printf("full entry from within S_IFLNK %s\n", full_path);
                err = lstat(full_path, &link_info);
                if (err == -1){ /*check lstat for errors*/
                    perror("Can't open the fullpath in print_entry");
                    printf("full: %s, link: %s\n", full_path, link_path);
                }

                link_len = link_info.st_size; /*buffer size for the name of the dereferenced link*/

                tmp = malloc(link_len * sizeof(char));
                if (tmp == NULL){
                    printf("Error allocating space for symlink path");
                    exit(1);
                } else
                    link_path = tmp;

                r = readlink(full_path, link_path, link_len);
                link_path[link_len] = '\0';
                //printf("%s \n", link_path);

                err = stat(link_path, &sym_info);
                if (err == -1){
                    //perror("Can't open the linkPath in print_entry");
                    printf("%s\n", link_path);

                } else {
                    print_entry(link_path, sym_info, NULL);
                }
                free(link_path);

            break;
        case S_IFCHR :
            printf(CADMIUM_YELLOW "%s -C\n" BEAT_THE_DEVIL_OUTTA_TTY, name);
            break;
        case S_IFREG :
            if (file_info.st_mode & 0111)
                printf(SAP_GREEN "%s -R\n" BEAT_THE_DEVIL_OUTTA_TTY, name);
            else
                printf("%s -R\n", name);
            break;
        case S_IFBLK :
            printf(CADMIUM_YELLOW "%s -D\n" BEAT_THE_DEVIL_OUTTA_TTY, name);

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
    } else {
        (*dir_stack) = tmp;
    }
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
    unsigned int len = 0;


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
    char *path = NULL, *full_path = NULL, *tmp = NULL;
    unsigned int full_len = 0;

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
