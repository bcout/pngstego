/*
  Program to embed a message into a given PNG image using the LSB
    (Least Significant Bit) method.

  References:
    www.libpng.org/pub/png/libpng-1.4.0-manual.pdf
    www.codeproject.com/Articles/581298/PNG-Image-Steganography-with-libpng


  Usage: $ ./pngstego filename.png embed message
         $ ./pngstego filename.png extract
*/

#include <png.h>
#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define EMBED_TEXT "EMBED"
#define EXTRACT_TEXT "EXTRACT"
#define FILENAME_MAX_LENGTH 256
#define BITS_NEEDED_TO_STORE_MESSAGE_LENGTH 32
#define HEADER_LENGTH 8
#define BYTE_SIZE 8

png_bytep* row_pointers;
png_infop info_ptr;
png_structp read_ptr;
png_structp write_ptr;
const char* PNG_filename;
const char* output_filename;
FILE* output_fp;
const char* method;
const char* message_filename;
FILE* message_fp;
size_t message_length;
int available_space;
char* PNG_output_filename;

void open_png_file(const char* PNG_filename);

void embed_data();

void extract_data();

void output_embedded_png();

void calculate_available_space(png_structp read_ptr, png_infop info_ptr);

bool check_message_size();

void exit_cleanly();

int main(int argc, char* argv[]){

    //Check number of command line arguments
    if(argc < 4){
        fprintf(stderr, "Usage: \t$ ./pngstego filename.png embed message_filename\n"
                        "\t$ ./pngstego filename.png extract output_filename\n");
        exit_cleanly();
    }

    //Get the PNG filename from the command line
    PNG_filename = argv[1];

    //Uncompress and unfilter the PNG
    open_png_file(PNG_filename);

    //Get the method being requested (embed or extract)
    method = argv[2];

    //If embed, embed the message from the provided file into the PNG
    if(strncasecmp(method, EMBED_TEXT, strlen(EMBED_TEXT)) == 0){

        //Calculate the amount of data able to be embedded
        calculate_available_space(read_ptr, info_ptr);

        //Open the file containing the message to embed
        message_filename = argv[3];
        message_fp = fopen(message_filename, "rb");
        if(message_fp == NULL){
            fprintf(stderr, "Error opening message file(): %s\n", strerror(errno));
            exit_cleanly();
        }

        if(check_message_size()){
            //Create the output png's filename
            char temp[FILENAME_MAX_LENGTH] = "embedded_";
            strcat(temp, PNG_filename);
            PNG_output_filename = temp;
            embed_data();
        }else{
            exit_cleanly();
        }
    }
    //If extract, extract the message from the PNG image and write it to a file.
    else if(strncasecmp(method, EXTRACT_TEXT, strlen(EXTRACT_TEXT)) == 0){
        output_filename = argv[3];
        output_fp = fopen(output_filename, "wb");
        if(output_fp == NULL){
            fprintf(stderr, "Error opening output file(): %s\n", strerror(errno));
            exit_cleanly();
        }

        extract_data();
    }
}

/*
    This function attempts to open the provided file, then checks if it is a
    PNG, then reads the header, then reads the entire image into memory.
*/
void open_png_file(const char* PNG_filename){
    FILE* PNG_file;
    unsigned char header[BYTE_SIZE];

    //Open the file
    PNG_file = fopen(PNG_filename, "rb");
    if(PNG_file == NULL){
        fprintf(stderr, "Error in open_png_file(): %s\n", strerror(errno));
        exit_cleanly();
    }

    //Start reading the file
    fread(header, 1, HEADER_LENGTH, PNG_file);

    //Check if the file is actually a PNG
    if(png_sig_cmp(header, 0, HEADER_LENGTH)){
        fprintf(stderr, "Error in open_png_file(): File is not a .PNG."
                        " Only .PNG files are supported\n");
        exit_cleanly();
    }

    //Initialize data structures
    //  Nulls are for optional custom error handlers. I am using the defaults.
    read_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(read_ptr == NULL){
        fprintf(stderr, "Error in open_png_file(): png_create_read_struct() returned NULL\n");
        exit_cleanly();
    }

    info_ptr = png_create_info_struct(read_ptr);
    if(info_ptr == NULL){
        fprintf(stderr, "Error in open_png_file(): png_create_info_struct() returned NULL\n");
        exit_cleanly();
    }

    //Initialize IO
    png_init_io(read_ptr, PNG_file);

    //HEADER_LENGTH bytes were read at the beginning, we must let libpng know.
    png_set_sig_bytes(read_ptr, HEADER_LENGTH);

    //Read entire PNG into memory
    png_read_png(read_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    row_pointers = png_get_rows(read_ptr, info_ptr);

    //Only accept PNGs with depths of 8 bits
    int bit_depth = png_get_bit_depth(read_ptr, info_ptr);
    if(bit_depth != BYTE_SIZE){
        fprintf(stderr, "Error in open_png_file(): File's bit depth is not valid."
                        " Provided image's bit depth is %d, only 8 bit depths are supported\n",
                        bit_depth);
        exit_cleanly();
    }


    fclose(PNG_file);
}

void embed_data(){

    int row;
    int max_rows = png_get_image_height(read_ptr, info_ptr);
    int max_cols = png_get_image_width(read_ptr, info_ptr);
    char buffer = 0;
    int bits_embedded = 0;

    for(row = 0; row < max_rows; row++){
        int col = 0;
        //Write the size of the message (bytes) into the first BITS_NEEDED_TO_STORE_MESSAGE_LENGTH
        // bytes of the image. If 32 bits are needed to store the message, then
        // 32 bytes of the image will be filled.
        if(row == 0){
            for(col; col < BITS_NEEDED_TO_STORE_MESSAGE_LENGTH; col++){
                //Loops over each bit in message_length and puts that bit into
                // the LSB of the current byte.
                if(message_length & (int)pow(2, col)){
                    *(row_pointers[row] + col) |= 1; //Set the LSB using bitwise or
                }else{
                    *(row_pointers[row] + col) &= 0xFE; //Sets the LSB to 0
                }
            }
        }

        //We need the width to be in bytes, but max_cols is in pixels. So, we
        // multiply it by 3 since there are 3 bytes in a pixel
        for(col; col < max_cols * 3; col++){
            if(col % BYTE_SIZE == 0){
                //If we've encoded 8 bits it is time to read the next byte
                if(!fread(&buffer, 1, 1, message_fp)){
                    break;
                }
            }

            //Do the actual embedding
            if(buffer & (int)pow(2, col % BYTE_SIZE)){
                *(row_pointers[row] + col) |= 1;
            }else{
                *(row_pointers[row] + col) &= 0xFE;
            }
            bits_embedded++;
        }
    }

    fprintf(stdout, "Message has been embedded!\n%d bytes embedded\n", (int)(bits_embedded/BYTE_SIZE));

    output_embedded_png();
}

void extract_data(){
    int row;
    int max_rows = png_get_image_height(read_ptr, info_ptr);
    int max_cols = png_get_image_width(read_ptr, info_ptr);
    char buffer = 0;
    int bits_extracted = 0;
    bool done_extracting = false;

    for(row = 0; row < max_rows; row++){
        if(done_extracting){
            break;
        }
        int col = 0;
        if(row == 0){
            //Extract the size of the message from the first BITS_NEEDED_TO_STORE_MESSAGE_LENGTH bytes
            for(col; col < BITS_NEEDED_TO_STORE_MESSAGE_LENGTH; col++){
                message_length |= ((*(row_pointers[0] + col) & 1) << col);
            }
        }

        //Extract the actual message
        for(col; col < max_cols * 3; col++){
            //If we're beyond the size metadata and we've extracted 8 bits, write the byte to the file
            if((col > BITS_NEEDED_TO_STORE_MESSAGE_LENGTH || row > 0) && col % BYTE_SIZE == 0){
                fwrite(&buffer, 1, 1, output_fp);
                buffer = 0;
            }

            //Check if all the data has been extracted
            int bytes_read = (max_cols * row) * 3 + col;
            if(bytes_read == (message_length * BYTE_SIZE) + BITS_NEEDED_TO_STORE_MESSAGE_LENGTH){
                done_extracting = true;
                break;
            }

            //Do the actual extracting
            buffer |= ((*(row_pointers[row] + col) & 1) << col % BYTE_SIZE);
            bits_extracted++;
        }
    }

    fprintf(stdout, "Done extracting!\n%d bytes extracted\n", (int)(bits_extracted / BYTE_SIZE));
}

void output_embedded_png(){
    FILE* output_png_fp;
    output_png_fp = fopen(PNG_output_filename, "wb");
    if(output_png_fp == NULL){
        fprintf(stderr, "Error in output_embedded_png(): %s\n", strerror(errno));
        exit_cleanly();
    }

    write_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(write_ptr == NULL){
        fprintf(stderr, "Error in output_embedded_png(): png_create_write_struct() returned NULL\n");
        exit_cleanly();
    }

    png_init_io(write_ptr, output_png_fp);
    png_set_rows(write_ptr, info_ptr, row_pointers);
    png_write_png(write_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
    fclose(output_png_fp);
}

void calculate_available_space(png_structp read_ptr, png_infop info_ptr){
    int width = png_get_image_width(read_ptr, info_ptr);
    int height = png_get_image_height(read_ptr, info_ptr);

    fprintf(stdout, "Image is %dpx x %dpx\n", width, height);

    //One pixel is 3 bytes, we can store 1 bit per byte. So, we can store
    // 3 bits per pixel.

    available_space = (width * height) * 3;
    float available_space_kb = available_space * 0.000125;

    fprintf(stdout, "Able to embed %d bytes (%.2f kilobytes) of data\n",
                    available_space, available_space_kb);
}

bool check_message_size(){
    struct stat st;
    int err = stat(message_filename, &st);
    if(err == -1){
        fprintf(stderr, "Error in check_message_size(): %s\n", strerror(errno));
        exit_cleanly();
    }
    message_length = st.st_size;

    if(message_length > available_space){
        fprintf(stderr, "Warning! Message is too large to embed in"
                        " the provided image (%zu bytes too large).\nDo you"
                        " wish to embed only the first %d bytes of the message"
                        " instead? Y/N\n> ", message_length - available_space, available_space);
        char input;
        fscanf(stdin, " %c", &input);
        getchar();
        input = toupper(input);
        if(input != 'Y'){
            return false;
        }

        message_length = available_space;
    }

    return true;
}

void exit_cleanly(){
    //Free memory
    if(read_ptr && info_ptr){
        fprintf(stdout, "Freeing Read Memory...\n");
        png_destroy_read_struct(&read_ptr, &info_ptr, (png_infopp)NULL);
    }
    if(write_ptr){
        fprintf(stdout, "Freeing Write Memory...\n");
        png_destroy_write_struct(&write_ptr, (png_infopp)NULL);
    }

    fprintf(stderr, "Exiting...\n");
    exit(EXIT_SUCCESS);
}
