#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdbool.h>
#define NUM_CHARS 256
#define MAX_FILEPATH_LENGTH 512

bool exit_success = true;
int numErrors;
typedef struct trienode // Structure for trie data structure
{
    struct trienode *children[NUM_CHARS]; 
    bool terminal; // Marks if the node is the end of the word
}trienode;


trienode *createnode() // create an allocated new node and return a pointer
{
    trienode *newnode = malloc(sizeof(*newnode));

    for(int i = 0; i < NUM_CHARS; i++)
    {
        newnode->children[i] = NULL; // start all children lengths as NULL
    }
    newnode->terminal = false;

    return newnode;
}

bool trieinsert(trienode **root, char *signedtext) // If false word is already in the structure
{
    if(*root == NULL)
    {
        *root = createnode(); // If trie is empty make root as the first node
    }

    unsigned char *text = (unsigned char *)signedtext; // cast the signed text to be unsigned for no negative indexes
    trienode *temp = *root; // temporary trienode pointer to the root
    int length = strlen(signedtext); // length of the word to be inserted

    for(int i = 0; i < length; i++) // create a node for every character of the word
    {
        if(temp->children[text[i]] == NULL)
        {
            temp->children[text[i]] = createnode();
        }
        temp = temp->children[text[i]];
    }

    if(temp->terminal)
    {
        return false; // word is already in structure
    }
    else
    {
        temp->terminal = true;
        return true;
    }

}

bool searchtrie(trienode *root, char *signedtext)
{
    unsigned char *text = (unsigned char *)signedtext; // cast the signed text to be unsigned for no negative indexes
    int length = strlen(signedtext); // length of word to be searched
    trienode *tmp = root;

    for(int i = 0; i < length; i++)
    {
        if(tmp->children[text[i]] == NULL)
        {
            return false; // word is not in structure
        }

        tmp = tmp->children[text[i]];
    }

    return tmp->terminal; // return 1 or 0 if word is in search or not
}

void freetrie(trienode *node) {
    if (node == NULL) {
        return; // if structure is empty, there is nothing to be freed so return
    }
    for (int i = 0; i < NUM_CHARS; i++) {
        freetrie(node->children[i]); // iterate through all 265 children with this recursive method
    }
    free(node); // free structure
}


int istxt(const char *filename) //checks if file is a txt file or not
{
    struct stat sbuf;

    if (stat(filename, &sbuf) == 0) {
        const char *extension = ".txt";
        int length = strlen(filename);
        int extLength = strlen(extension);

        if (length >= extLength && strcmp(filename + length - extLength, extension) == 0) {
            return 1;
        } else {
            return 0;
        }
    }
    else
    {
        perror("stat");
        return 0;
    }

}



int read_file(const char* filepath, trienode *root, int fd1) { // reads txt file and checks for errors
    struct stat sbuf; // Initiallize stat strcuture
    if (stat(filepath, &sbuf) != 0) { // if file does not exist return EXIT_FAILURE
        perror("stat");
        return EXIT_FAILURE;
    }

    char file[MAX_FILEPATH_LENGTH]; // Use macro prevent overflow for filepath length
    strcpy(file, filepath); // store filepath pointer into file array


    int fd = open(file, O_RDONLY, 0400); // Open file to read and execute
    if (fd < 0) {
        perror("open");
        return EXIT_FAILURE; // exit out of program if file descriptors are less than 0
    }

    int line = 1; // Initialize line counter
    int column = 1; // Initialize column counter
    int r; // Initialize variable to read buffer
    char buffer; // Initialize buffer
    int i = 0; // Initailize index of word
    char word[50] = ""; // Initialize word to be searched from our .txt file
    int colofword = 1; // Initialize the column of the first charcter of the word we report an error in

    while ((r = read(fd, &buffer, 1)) > 0) // read charcters of our buffer
    {

        if(isalpha(buffer) || buffer == '\'')
        // Store all alphabetical charcters as well as apostraphes in the word
        {
            word[i++] = buffer;
            
            if (i == 1) 
            {
                colofword = column; // If index of word is 1, return that as column of word
            }
            column++; // Increment columns
        }
        else
        {
            if(isspace(buffer)) 
            {
                column++; // if there is a whitespace increment columns
            }
            if(i > 0)
            {
                word[i] = '\0'; // set null terminator for the end of the word
                
                if(searchtrie(root, word) == 0) // search word from our dictionary
                {
                    // print the file name, line number, and column number if not in dictionary
                    printf("%s (%d,%d): %s\n", file, line, colofword, word);
                    char write_buffer[5000];
                    snprintf(write_buffer, sizeof(write_buffer), "%s (%d,%d): %s\n", file, line, colofword, word);

                    if(write(fd1, write_buffer, strlen(write_buffer)) == -1)
                    {
                        perror("write");
                        exit(EXIT_FAILURE);
                    }
                    exit_success = false;
                    numErrors++;
                }

                i = 0; // reset word index
            }
            if(buffer == '\n')
            {
                line++; // if there is a newline character in buffer, increment line number
                column = 1; // reset column since we start at a new line
            }
        }   
    
    }

    if(i > 0)
    {
        word[i] = '\0'; // set null terminator for the end of the word
    }


    close(fd); // close file after done reading
    return EXIT_SUCCESS;
}

int traverse_directory(const char* filepath, trienode *root, int fd1) //traverses directory
{
    DIR *dir = opendir(filepath);
    if(dir == NULL)
    {
        perror("opendir");
        return EXIT_FAILURE;
    }
    struct dirent *entry;
    struct stat sbuf;
    char file[256];
    strcpy(file, filepath);
    char fullfilepath[MAX_FILEPATH_LENGTH];
    while ((entry = readdir(dir)) != NULL)
    {
        // Process the directory entry
        snprintf(fullfilepath, MAX_FILEPATH_LENGTH, "%s/%s", file, entry->d_name);

        // Populate sbuf with information about the file
        if (stat(fullfilepath, &sbuf) != 0)
        {
            perror("stat");
            continue; // Skip to next entry if stat fails
        }

        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }


        if(S_ISDIR(sbuf.st_mode))
        {
            traverse_directory(fullfilepath, root, fd1);
        }

        if (istxt(fullfilepath) == 1) {
            read_file(fullfilepath, root, fd1);
        }
        
    }

    closedir(dir);

    return EXIT_SUCCESS;

}

int main(int argc, char *argv[])
{
    struct stat sbuf; // Initiallize stat strcuture
    trienode *root = NULL; // Initializes root 
    if (stat(argv[1], &sbuf) != 0) {// if dictionary argument does not exist return EXIT_FAILURE
            perror("stat");
            return EXIT_FAILURE;
        }
    int buffersize = sbuf.st_size + 1; 
    char file[256];
    strcpy(file, argv[1]); 
    char *buffer = (char *)malloc(buffersize);
    int fd = open(file, O_RDONLY, 0401); // Attempts to open file for reading 
    if (fd < 0) { // if file does not exist, or error with file, return EXIT_FAILURE
        perror("open");
        return EXIT_FAILURE;
    }

    int r; // Initialize variable to read buffer
    while ((r = read(fd, buffer, buffersize)) > 0) // read charcters of our buffer
    {
        buffer[r] = '\0';

        char *token = strtok(buffer, " .,?\t\n");


        while(token != NULL)
        {
            trieinsert(&root, token); // inserts token into trie structure
            token[0] = toupper(token[0]);
            trieinsert(&root, token); // inserts token with initial uppercase letter into trie structure
            for(int i = 1; i < strlen(token); i++){ 
                token[i] = toupper(token[i]); 
            }
            trieinsert(&root, token); // inserts token with all uppercase letter into trie structure
            token = strtok(NULL, " .,?\t\n"); 
        }

    }

    free(buffer);

    close(fd);

    int fd1 = open("errors", O_WRONLY|O_CREAT|O_TRUNC, 0600);
    if (fd1 < 0) 
    {
        perror("open");
        return EXIT_FAILURE; // exit out of program if file descriptors are less than 0
    }

    for(int i=2; i < argc; i++){
        if(istxt(argv[i])){ //checks if argument is a txt file
            read_file(argv[i], root, fd1); //reads txt file
        } else { //if not a txt file, checks for directories
    traverse_directory(argv[i], root, fd1);  //traverses directory
        }
    }

    freetrie(root);

    close(fd1);

    if(exit_success == false) {
        printf("\nYou have spelling errors!\n");
        printf("Number of Errors: %d\n", numErrors);
        exit(EXIT_FAILURE);
    } else{
        printf("\nNo Spelling Errors in your file!\n");
        return EXIT_SUCCESS;
    }
    

}
