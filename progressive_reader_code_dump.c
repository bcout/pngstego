png_structp png_ptr;
png_infop info_ptr;

/*
    This function gets called as blocks of data are received
    Takes in a buffer of bytes with the specified length.
        It should not be more than 64K, and 4K is said to work nice.
    Returns -1 if there is an error, 0 otherwise
*/
int process_data(png_bytep buffer, png_uint_32 length){
    if(setjmp(png_jmpbuf(png_ptr))){
        fprintf(stderr, "Error in process_data():"
                        "setjmp(png_jmpbuf(png_ptr)) returned true");
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
        return -1;
    }

    png_process_data(png_ptr, info_ptr, buffer, length);
    return 0;
}

/*
    This function is called when enough data has been supplied so all of the
    header has been read
*/
void info_callback(png_structp png_ptr, png_infop info){
    /*
        Do setup for the image read here.
    */

    png_start_read_image(png_ptr);
}

/*
    This function is called when each row of the image is read
*/
void row_callback(png_structp png_ptr, png_bytep new_row,
                    png_uint_32 row_num, int pass){

    /*
        The manual includes this line, but doesn't explain its purpose,
        nor does it explain how to initialize old_row.
    */
    //png_progressive_combine_row(png_ptr, old_row, new_row);
}

/*
    This function is called when the whole file has been read
*/
void end_callback(png_structp png_ptr, png_infop info){
    /*
        Do whatever you want
    */
}

/*
    Code based on example from www.libpng.org/pub/png/libpng-1.4.0-manual.pdf
    page 27, read on June 12, 2021

    This function initializes a progressive png reader.
    Returns -1 if there is an error, 0 otherwise
*/
int initialize_png_reader(){
    /*
        Nulls are for optional custom error functions. I am using the defaults
    */
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(png_ptr == NULL){
        fprintf(stderr, "Error in initialize_png_reader(): "
                        "png_create_read_struct() returned NULL\n");
        return -1;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if(info_ptr == NULL){
        fprintf(stderr, "Error in initialize_png_reader(): "
                        "png_create_info_struct() returned NULL\n");
        png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
        return -1;
    }

    if(setjmp(png_jmpbuf(png_ptr))){
        fprintf(stderr, "Error in initialize_png_reader():"
                        "setjmp(png_jmpbuf(png_ptr)) returned true");
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
        return -1;
    }

    png_set_progressive_read_fn(png_ptr, (void*)NULL, info_callback,
                                row_callback, end_callback);

    return 0;
}
