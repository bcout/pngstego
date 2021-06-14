/*
    Check number of arguments
    Program needs 3 to extract and 4 to embed
*/
if(argc < 3){
    fprintf(stderr, "Usage: \t$ ./pngstego filename.png embed \"message\""
                    "\n\t$ ./pngstego filename.png extract\n");
    exit(EXIT_FAILURE);
}

const char* filename = argv[1];
const char* method = argv[2];

/*
    Check whether the user is embedding or extracting data
*/
bool embedding = NULL;
if(strncasecmp(method, EMBED_TEXT, strlen(EMBED_TEXT)) == 0){
    embedding = true;
}else if(strncasecmp(method, EXTRACT_TEXT, strlen(EXTRACT_TEXT)) == 0){
    embedding = false;
}else{
    fprintf(stderr, "%s is not a valid operation\nUse %s or %s\n",
                    method, EMBED_TEXT, EXTRACT_TEXT);
    exit(EXIT_FAILURE);
}

/*
    Open the png image provided as a command line argument
*/
FILE* fp = fopen(filename, "rb");
if(fp == NULL){
    fprintf(stderr, "file \"%s\" not found\n", filename);
    exit(EXIT_FAILURE);
}

/*
    Display a helpful message to the user, letting them know exactly
    what the program is doing.
*/
if(embedding){
    fprintf(stdout, "Embedding message in %s...\n", filename);
}else{
    fprintf(stdout, "Extracting message from %s...\n", filename);
}
