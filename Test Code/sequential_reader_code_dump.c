/*
    Read the first 8 bytes of the file to tell whether it is
    a png or not.
*/
const unsigned char header[HEADER_LENGTH];
fread(header, 1, HEADER_LENGTH, fp);
bool is_png = !png_sig_cmp(header, 0, HEADER_LENGTH);
if(!is_png){
    fprintf(stderr, "%s is not a png, only png images are supported\n",
                    filename);
    exit(EXIT_FAILURE);
}

/*
    Initialize the png_struct and png_info structs required by libpng
    The NULL parameters are optional error function pointers that I am not
    using.
*/
png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
if(png_ptr == NULL){
    fprintf(stderr, "Error at png_create_read_struct(): png_ptr is null\n");
    exit(EXIT_FAILURE);
}

png_infop info_ptr = png_create_info_struct(png_ptr);
if(info_ptr == NULL){
    fprintf(stderr, "Error at png_create_info_struct(): info_ptr is null\n");
    png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
    exit(EXIT_FAILURE);
}

png_infop end_info = png_create_info_struct(png_ptr);
if(end_info == NULL){
    fprintf(stderr, "Error at png_create_info_struct(): end_info is null\n");
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
    exit(EXIT_FAILURE);
}

/*
    Initialize libpng's IO
*/
png_init_io(png_ptr, fp);

/*
    Mark the first 8 bytes as 'read', since they were used earlier to
    check whether the file was a png or not.
*/
png_set_sig_bytes_read(png_ptr, HEADER_LENGTH);

/*
    Make libpng call read_row_callback after each row is read
*/
png_set_read_status_fn(png_ptr, read_row_callback);

/*
    Set the maximum accepted height and width for the input file (in pixels)
*/
//png_set_user_limits(png_ptr, width_max, height_max);

/*
    Get the maximum accepted height and width for the input file (in pixels)
*/
//width_max = png_get_user_width_max(png_ptr);
//height_max = png_get_user_height_max(png_ptr);

/*
    Read file information up to the actual image data
*/
png_read_info(png_ptr, info_ptr);

/*
    Get information about the png
    See manual page 14 for list of functions (USEFUL!)
*/

/*
    Read the image
*/
png_bytep row_pointers[png_get_image_height(png_ptr, info_ptr)];
png_read_image(png_ptr, row_pointers);

/*
    Finish reading the file
*/
png_read_end(png_ptr, end_info);

/*
    Free memory
*/
png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
