#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <stdbool.h>
#include <fcntl.h>
#include<string.h>
#include <sys/wait.h>

/* 
de-allocate: char** children_name         name_array
             struct **tree_node children. child_pointer_array
             struct *tree_node node.      node_pointer
             char** line                  line_elements_array
             file_line_element.           char*
             file_line_size_array.        int* 
 */

struct tree_node{
    unsigned             children_num;
    char*                name;
    char**               children_name;
    struct tree_node     **children;
};

size_t fsize;

void create_process_tree(struct tree_node *root, int level);
struct tree_node* create_node(char* Name, int Num, char** children_Name);
void print_tree_helper(struct tree_node* root, int level);
void print_tree(struct tree_node* root);
bool get_file_size(char* filename,size_t *fsize);
void read_entire_file_to_buffer(char* buffer, char* filename, size_t fsize);
int* create_file(char* buffer, char*** file, int num_of_lines);
void findline(int row,char** line, int line_size, char* buffer);
int find_newline(int row, char* buffer);
int find_space_num(int row, char* buffer);
int find_space(int column, char* line, int size_of_line);
int find_num_of_lines(char* buffer);
char* find_line_element(int row, int column, char* buffer);
struct tree_node* create_tree(char* buffer, struct tree_node** tree_node_arr);
struct tree_node* find_child_name_in_arr(char* name, struct tree_node** tree_node_arr, int num_of_lines);
bool str_equal(char* name1, char* name2);
char* findname(int row, char*** file);
int find_childnum(int row, char*** file);
void find_children_name(int row, char** children_name, int* line_size_array, char*** file, int children_num);


void create_process_tree(struct tree_node *root, int level){
    /*used for parent to write info to child and child to read info from parent
    but child is not supposed to have right to write to parent and vice versa
    */
    int fd1[2]; 

    /*used for child to write info to parent and parent to read info from child
    but parent is not supposed to have right to write to child and vice versa
    */
    int fd2[2]; 
    int n = root->children_num;
    int writefd[n];
    int readfd[n];
    pid_t pid[n];
    if (n==0){
        for (int i = 0; i < level; i++){
            printf("\t");
        }
        printf("%s\n", root->name);
        return;
    }
    for (int i =0; i<n; i++){
        pipe(fd1);
        pipe(fd2);
        pid[i] = fork();
        if (pid[i] == 0){
        	/*child is not supposed to write to parent using pipe1*/
            close(fd1[1]);
            /*read message from parent, the message content doesn't matter*/
            read(fd1[0],&pid[i],sizeof(pid_t));
            create_process_tree(root->children[i], level+1);
            /*child is not supposed to read from parent using pipe2*/
            close(fd2[0]);
            /*send message to parent, the message content doesn't matter*/
            write(fd2[1],&pid[i],sizeof(pid_t));
            exit(0);
        } 
        /*parent is not supposed to read from child using pipe1*/
        close(fd1[0]);
        writefd[i] = fd1[1];
        /*parent is not supposed to write to child using pipe2*/
        close(fd2[1]);
        readfd[i] = fd2[0];
    }
    for (int i = 0; i < level; i++){
            printf("\t");
    }
    printf("%s\n", root->name);
    for (int i = 0; i < n; i++){
    	/*send message to child[i], the message content doesn't matter*/
        write(writefd[i], &pid[i], sizeof(pid_t));
        /*read message from child[i], the message content doesn't matter*/
        read(readfd[i],&pid[i],sizeof(pid_t));
    }
    wait(NULL);
}

struct tree_node* create_tree(char* buffer, struct tree_node** tree_node_arr){

    int num_of_lines = find_num_of_lines(buffer);
    char** file[num_of_lines];
    int* line_size_array = create_file(buffer,file,num_of_lines);

    for (int i = 0; i<num_of_lines; i++){
        int children_num = find_childnum(i,file);
        char* name = findname(i,file);
        char** children_name = (char**)malloc((children_num)*sizeof(char*));
        find_children_name(i, children_name, line_size_array, file, children_num);
        tree_node_arr[i]=create_node(name,children_num,children_name);
    }

    for (int i = 0; i < num_of_lines; i++){
        for (int j = 0; j < tree_node_arr[i]->children_num; j++){
            char* name = tree_node_arr[i]->children_name[j];
            tree_node_arr[i]->children[j] = find_child_name_in_arr(name, tree_node_arr, num_of_lines);
        }
    }


    for (int i = 0; i < num_of_lines; i++){ // de-allocate each file_line char**
        free(file[i]);
    }
    free(line_size_array); // de-allocate the array that contains the size of elements of each file line.

    return tree_node_arr[0];
}

struct tree_node* find_child_name_in_arr(char* name, struct tree_node** tree_node_arr, int num_of_lines){
    for (int i = 0; i < num_of_lines; i++){
        if (str_equal(name, tree_node_arr[i]->name)){
            return tree_node_arr[i];
        }
    }
    return NULL;
}

bool str_equal(char* name1, char* name2){
    if (strlen(name1) == strlen(name2)){
        for (int i =0; i<strlen(name1);i++){
            if ( name1[i] != name2[i]){
                return false;
            }
        }
        return true;
    } else {
        return false;
    }
}


char* findname(int row, char*** file){
    char* name = file[row][0];
    return name;
}

int find_childnum(int row, char*** file){
    int children_num = atoi(file[row][1]);
    return children_num;
}

void find_children_name(int row, char** children_name, int* line_size_array, char*** file, int children_num){
    for (int i = 0; i < children_num; i++){
        children_name[i] = file[row][i+2]; 
    }
}





int* create_file(char* buffer, char*** file, int num_of_lines){// used to convert a file to a 2-D array and return a pointer to a line_size_array
    int* line_size_array = (int*)malloc((num_of_lines)*sizeof(int));
    for (int i = 0; i < num_of_lines; i++){
        int space_num = find_space_num(i,buffer);
        char** line = (char**)malloc((space_num+1)*sizeof(char*)); 
        line_size_array[i]=space_num+1;
        findline(i, line, line_size_array[i], buffer);
        file[i] = line;
    }
    return line_size_array;
}

void findline(int row,char** line, int line_size, char* buffer){ 
// row represent the line_number, line represents the string_array, line_size represents the number of element the line_array has 
    for (int j = 0; j < line_size; j++){
        line[j] = find_line_element(row,j,buffer);
    }
}


int find_newline(int row, char* buffer){ // used to find the index of newline symbol at the end of a given line
                                           // for example, index = 0, it means line0, this should return 7
                                           // the end of file of the last lin will be taken care of
    int counter = 0;
    for (int i = 0; i < fsize; i++){
        if (buffer[i] == '\n'){
            counter++;
            if (counter == row+1){
                return i;
            }
        }
    }
    return counter;
}

int find_space_num(int row, char* buffer){ //used to find the number of space in a given line
    int counter = 0;
    int space_num = 0;
    for (int j =0; j < (int)fsize; j++){
        if (buffer[j] == '\n'){
            counter++;
        }

        if (counter == row){
            if (buffer[j] == ' '){
                space_num++;
            }
        } else if (counter > j){
            break;
        }   
    }
    return space_num;
}

int find_space(int column, char* line, int size_of_line){ // used to find the index of space symbol given the column and line
                                           // for example, line is the line1 and column=0, this should return 1
                                           // the last column will be taken care of 
    int counter = 0;
    for (int i = 0; i < size_of_line; i++){
        if (line[i] == ' '){
            counter++;
            if (counter == column+1){
                return i;
            }
        }
    }
    return counter;
}

int find_num_of_lines(char* buffer){
    int counter = 1;
    for (int i = 0; i < fsize; i++){
        if (buffer[i] == '\n'){
            counter++;
        }
    }
    return counter;
}


char* find_line_element(int row, int column, char* buffer){ // given a row and column, find the line_element in the file
    int num_of_lines = find_num_of_lines(buffer);
    char* lines[num_of_lines];
    int size_of_line_array[num_of_lines];
    for (int i = 0; i < num_of_lines; i++){
        if (i == 0){
            size_of_line_array[i] = find_newline(i, buffer); // size of line not including new line or EOF symbol, for example "A 2 B C"
            //printf("line%d = %d\n", i, size_of_line_array[i]);
            lines[i] = (char*)malloc((size_of_line_array[i])*sizeof(char));
            for (int j = 0; j < size_of_line_array[i]; j++){
                lines[i][j] = buffer[j];
            }
            //printf("%s\n", lines[i]);
        } else if (i == num_of_lines-1){
            size_of_line_array[i] = fsize-find_newline(i-1, buffer)-1;
            //printf("line%d = %d\n", i, size_of_line_array[i]);
            lines[i] = (char*)malloc((size_of_line_array[i])*sizeof(char));
            for (int j = 0; j < size_of_line_array[i]; j++){
                lines[i][j] = buffer[find_newline(i-1, buffer)+j+1];
            }
            //printf("%s\n", lines[i]);
        } else {
            size_of_line_array[i] = find_newline(i, buffer)-find_newline(i-1, buffer)-1;
            //printf("line%d = %d\n", i, size_of_line_array[i]);
            lines[i] = (char*)malloc((size_of_line_array[i])*sizeof(char));
            for (int j = 0; j < size_of_line_array[i]; j++){
                lines[i][j] = buffer[find_newline(i-1, buffer)+j+1];
            }
            //printf("%s\n", lines[i]);
        }
    }

    char* line = lines[row];
    char* element;
    int space_num = find_space_num(row, buffer);
    //printf("space%d = %d\n", row, space_num);
    int size_of_line = size_of_line_array[row];
    //printf("size of line%d = %d\n", row, size_of_line_array[row]);
    int element_size;
    if (column == 0){
        element_size = find_space(column, line, size_of_line);
        //printf("element_size = %d\n", element_size);
        element = (char*)malloc((element_size)*sizeof(char));
        for (int i = 0; i < element_size; i++){
            element[i] = line[i];
        }
        for (int i = 0; i < num_of_lines; i++){
            free(lines[i]);
        }
        //printf("%s\n", element);
        return element;
    } else if (column == space_num){
        int space_index = find_space(column-1, line, size_of_line);
        //printf("space_%d_%d_index = %d\n",row,column,space_index);
        element_size = size_of_line-1-space_index;
        //printf("element_size = %d\n", element_size);
        element = (char*)malloc((element_size)*sizeof(char));
        for (int i = 0; i < element_size; i++){
            element[i] = line[space_index+i+1];
        }
        for (int i = 0; i < num_of_lines; i++){
            free(lines[i]);
        }
        //printf("%s\n", element);
        return element;
    } else {
        int space_front_index = find_space(column-1, line, size_of_line);
        int space_rear_index = find_space(column, line, size_of_line);
        element_size = space_rear_index-space_front_index-1;
        //printf("element_size = %d\n", element_size);
        element = (char*)malloc((element_size)*sizeof(char));
        for (int i = 0; i < element_size; i++){
            element[i] = line[space_front_index+i+1];
        }
        for (int i = 0; i < num_of_lines; i++){
            free(lines[i]);
        }
        //printf("%s\n", element);
        return element;
    }

}


struct tree_node* create_node(char* Name, int Num, char** children_Name){
    struct tree_node* node_pointer = (struct tree_node*)malloc(sizeof(struct tree_node));
    node_pointer->name = Name;
    node_pointer->children_num = Num;
    node_pointer->children_name = children_Name;
    node_pointer->children = (struct tree_node**)malloc((Num)*sizeof(struct tree_node*));
    return node_pointer;
}

void print_tree_helper(struct tree_node* root, int level){
    int i;
    for (i = 0; i < level; i++){
        printf("\t");
    }
    printf("%s\n", root->name);
    
    for (i=0; i < root->children_num; i++){
        print_tree_helper(*(root->children+i), level + 1);
    }
}

void print_tree(struct tree_node* root){
    print_tree_helper(root, 0);
}


bool get_file_size(char* filename,size_t *fsize){
    FILE* fp=fopen(filename, "r");
    
    if (fp==NULL){
        printf("%s: No such file or directory\n",filename);
        return false;
    }
    fseek(fp, 0, SEEK_END);
    *fsize = ftell(fp);
    fclose(fp);
    return true;
}

void read_entire_file_to_buffer(char* buffer, char* filename, size_t fsize){
    FILE* fp=fopen(filename, "r");
    fseek(fp, 0, SEEK_SET);
    fread(buffer, 1, fsize, fp);
    buffer[fsize] = 0;
    fclose(fp);
}
