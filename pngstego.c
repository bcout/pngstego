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

#define EMBED_TEXT "EMBED"
#define EXTRACT_TEXT "EXTRACT"
#define FILENAME_MAX_LENGTH 256
#define HEADER_LENGTH 8
#define BYTE_SIZE 8

png_bytep* row_pointers;
png_infop info_ptr;
png_structp read_ptr;
png_structp write_ptr;
const char* PNG_filename;
const char* output_filename;
const char* method;
const char* message;
int available_space;

void open_png_file(const char* PNG_filename);

void encode(const char* to_encode_filename);

void decode(const char* output_filename);

void output_png(const char* output_filename);

int calculate_available_space(png_structp read_ptr, png_infop info_ptr);

int main(int argc, char* argv[]){

    //Check number of command line arguments
    if(argc < 4){
        fprintf(stderr, "Usage: \t$ ./pngstego filename.png embed message\n"
                        "\t$ ./pngstego filename.png extract output_filename\n");
        exit(EXIT_FAILURE);
    }

    //Get the PNG filename from the command line
    PNG_filename = argv[1];

    //Uncompress and unfilter the PNG
    open_png_file(PNG_filename);

    //Calculate the amount of data able to be embedded
    calculate_available_space(read_ptr, info_ptr);

    //Get the method being requested (embed or extract)
    method = argv[2];

    //If embed, embed the message from the command line into the PNG
    if(strncasecmp(method, EMBED_TEXT, strlen(EMBED_TEXT)) == 0){
        message = argv[3];
    }

    //If extract, extract the message from the PNG image and write it to a file.
    if(strncasecmp(method, EXTRACT_TEXT, strlen(EXTRACT_TEXT)) == 0){
        output_filename = argv[3];
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
        exit(EXIT_FAILURE);
    }

    //Start reading the file
    fread(header, 1, HEADER_LENGTH, PNG_file);

    //Check if the file is actually a PNG
    if(png_sig_cmp(header, 0, HEADER_LENGTH)){
        fprintf(stderr, "Error in open_png_file(): File is not a .PNG."
                        " Only .PNG files are supported\n");
        exit(EXIT_FAILURE);
    }

    //Initialize data structures
    //  Nulls are for optional custom error handlers. I am using the defaults.
    read_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(read_ptr == NULL){
        fprintf(stderr, "Error in open_png_file(): png_create_read_struct() returned NULL\n");
        exit(EXIT_FAILURE);
    }

    info_ptr = png_create_info_struct(read_ptr);
    if(info_ptr == NULL){
        fprintf(stderr, "Error in open_png_file(): png_create_info_struct() returned NULL\n");
        png_destroy_read_struct(&read_ptr, &info_ptr, (png_infopp)NULL);
        exit(EXIT_FAILURE);
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
        exit(EXIT_FAILURE);
    }


    fclose(PNG_file);
}

int calculate_available_space(png_structp read_ptr, png_infop info_ptr){
    int width = png_get_image_width(read_ptr, info_ptr);
    int height = png_get_image_height(read_ptr, info_ptr);

    fprintf(stdout, "Image is %dpx x %dpx\n", width, height);

    //One pixel is 3 bytes, we can store 1 bit per byte. So, we can store
    // 3 bits per pixel.

    available_space = (width * height) * 3;
    float available_space_kb = available_space * 0.000125;

    fprintf(stdout, "Able to embed %d bits (%.2f kilobytes) of data\n",
                    available_space, available_space_kb);
}
