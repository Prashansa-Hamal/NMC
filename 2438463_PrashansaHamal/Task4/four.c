#include <stdio.h>  
#include <stdlib.h> 
#include <pthread.h> 
#include "lodepng.h" 

// Structure to represent a pixel with RGB values
typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} Pixel;

// Structure to pass data to each thread
typedef struct {
    Pixel** original;  
    Pixel** blurred;   
    int start_row;     
    int end_row;       
    int width;        
    int height;        
} ThreadData;

// Function to allocate memory for a 2D image array of pixels
Pixel** allocate_image(int width, int height) {
    Pixel** image = malloc(height * sizeof(Pixel*));  // Allocate memory for rows
    if (!image) return NULL;

    // Allocate memory for columns in each row
    for (int i = 0; i < height; i++) {
        image[i] = malloc(width * sizeof(Pixel));
        if (!image[i]) {
            // Free already allocated memory in case of failure
            for (int j = 0; j < i; j++) free(image[j]);
            free(image);
            return NULL;
        }
    }
    return image;
}

// Function to free the memory allocated for a 2D image array
void free_image(Pixel** image, int height) {
    for (int i = 0; i < height; i++) free(image[i]);  // Free each row
    free(image);  // Free the row pointers
}

// Function to read a PNG image and convert it to a 2D array of Pixels
Pixel** read_png(const char* filename, int* width, int* height) {
    unsigned char* image_data;
    unsigned width_in_pixels, height_in_pixels;

    // Decode the PNG file into RGBA format
    unsigned error = lodepng_decode32_file(&image_data, &width_in_pixels, &height_in_pixels, filename);
    if (error) {
        printf("Error %u: %s\n", error, lodepng_error_text(error));  // Error decoding the file
        return NULL;
    }

    //Store the image dimension in width and height
    *width = width_in_pixels;
    *height = height_in_pixels;

    // Allocate memory for the image
    Pixel** image = allocate_image(*width, *height);
    if (!image) {
        free(image_data);
        return NULL;
    }

    // Convert the decoded image data into a 2D array of Pixel structs
    for (int y = 0; y < *height; y++) {
        for (int x = 0; x < *width; x++) {
            int index = 4 * (*width * y + x);  // Calculate index for RGBA
            image[y][x].r = image_data[index];
            image[y][x].g = image_data[index + 1];
            image[y][x].b = image_data[index + 2];
        }
    }

    free(image_data);  // Free the original image data after conversion
    return image;
}

// Function to write the blurred image back to a PNG file
void write_png(const char* filename, Pixel** image, int width, int height) {
    unsigned char* image_data = malloc(4 * width * height * sizeof(unsigned char));  // Allocate memory for the image data
    if (!image_data) {
        perror("Failed to allocate memory for PNG data");
        return;
    }

    // Convert the 2D array of Pixels back to a byte array for writing
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = 4 * (width * y + x);
            image_data[index] = image[y][x].r;
            image_data[index + 1] = image[y][x].g;
            image_data[index + 2] = image[y][x].b;
            image_data[index + 3] = 255;  // Set alpha to 255 (opaque)
        }
    }

    // Encode the image data into a PNG file
    unsigned error = lodepng_encode32_file(filename, image_data, width, height);
    if (error) {
        printf("Error %u: %s\n", error, lodepng_error_text(error));  // Error encoding the file
    }

    free(image_data);  // Free the allocated memory for image data
}

// Function to apply blur on a part of the image using multiple threads
void* apply_blur(void* arg) {
    ThreadData* data = (ThreadData*)arg;

    // Process the assigned rows for blur effect
    for (int y = data->start_row; y < data->end_row; y++) {
        for (int x = 0; x < data->width; x++) {
            int sum_r = 0, sum_g = 0, sum_b = 0, count = 0;

            // Loop through the 3x3 neighborhood around the pixel
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int nx = x + dx;
                    int ny = y + dy;
                    if (nx >= 0 && nx < data->width && ny >= 0 && ny < data->height) {
                        sum_r += data->original[ny][nx].r;
                        sum_g += data->original[ny][nx].g;
                        sum_b += data->original[ny][nx].b;
                        count++;
                    }
                }
            }

            // Assign the averaged color to the blurred image
            data->blurred[y][x].r = (unsigned char)(sum_r / count);
            data->blurred[y][x].g = (unsigned char)(sum_g / count);
            data->blurred[y][x].b = (unsigned char)(sum_b / count);
        }
    }

    pthread_exit(NULL);  // End the thread
}

int main(int argc, char** argv) {
    // Check for correct number of arguments (input file, output file, number of threads)
    if (argc != 4) {
        printf("Usage: %s <input.png> <output.png> <threads>\n", argv[0]);
        return 1;
    }

    int width, height;
    Pixel** original = read_png(argv[1], &width, &height);  // Read input PNG file
    if (!original) return 1;

    Pixel** blurred = allocate_image(width, height);  // Allocate memory for blurred image
    if (!blurred) {
        free_image(original, height);
        return 1;
    }

    int T = atoi(argv[3]);  // Convert number of threads to integer
    if (T <= 0) {
        printf("Invalid number of threads.\n");  // Check for valid number of threads
        free_image(original, height);
        free_image(blurred, height);
        return 1;
    }

    // Allocate memory for thread data
    ThreadData* thread_data = malloc(T * sizeof(ThreadData));
    if (!thread_data) {
        perror("Failed to allocate memory for thread data");
        free_image(original, height);
        free_image(blurred, height);
        return 1;
    }

    // Calculate rows per thread and distribute remaining rows
    int rows_per_thread = height / T;
    int remainder = height % T;
    int current_start = 0;

    // Assign each thread its part of the image
    for (int i = 0; i < T; i++) {
        thread_data[i].original = original;
        thread_data[i].blurred = blurred;
        thread_data[i].start_row = current_start;
        thread_data[i].end_row = current_start + rows_per_thread + (i < remainder ? 1 : 0);
        thread_data[i].width = width;
        thread_data[i].height = height;
        current_start = thread_data[i].end_row;
    }

    // Create threads to apply blur on different parts of the image
    pthread_t threads[T];
    for (int i = 0; i < T; i++) {
        if (pthread_create(&threads[i], NULL, apply_blur, &thread_data[i]) != 0) {
            perror("Failed to create thread");
            free_image(original, height);
            free_image(blurred, height);
            free(thread_data);
            return 1;
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < T; i++) pthread_join(threads[i], NULL);

    // Write the blurred image to the output file
    write_png(argv[2], blurred, width, height);

    // Free allocated memory
    free_image(original, height);
    free_image(blurred, height);
    free(thread_data);

    printf("Gaussian blur applied successfully.\n");
    return 0;
}
