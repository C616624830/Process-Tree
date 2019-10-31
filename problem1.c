#include"header.h"

int main(int argc, char* argv[]){
    
    /*this function will take the reference of fsize and assign the size of the file to fsize*/
   if (get_file_size("description_file",&fsize)==false){
       printf("error");
       return 0;
   }

   /*create a buffer to read all characters in the file*/
   char* buffer = (char*)malloc((fsize + 1)*sizeof(char));

   /*this function will read entire file including EOF to the buffer
    For example: the file format should be like this
    "A 2 B C\nB 1 D\nC 0\nD 0\0"
    */
   read_entire_file_to_buffer(buffer, "description_file", fsize);

   /*num_of_lines will be 4 by using the above example*/
   int num_of_lines = find_num_of_lines(buffer);

   /*used to hold all tree_node pointers*/
   struct tree_node* tree_node_arr[num_of_lines]; 

   /*Using the buffer obtained above to create the tree structure
     create_tree() will return the root node of the tree*/
   struct tree_node* root = create_tree(buffer, tree_node_arr);

   printf("%s\n", "Data structure tree: ");
   print_tree(root);

   printf("%s\n", "Process tree: ");
   create_process_tree(root, 0);


   /*delete all dynamically allocated memory space*/
   for (int i = 0; i < num_of_lines; i++){
        free(tree_node_arr[i]->name);
        free(tree_node_arr[i]->children);
        free(tree_node_arr[i]->children_name); // the name pointer is actaully from the file_line_element
        free(tree_node_arr[i]);
   }

}
