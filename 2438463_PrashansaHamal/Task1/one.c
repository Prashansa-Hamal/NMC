#include <stdio.h>
#include <string.h>

// Load data from a file and append it to arrays
int load_data(const char *file_name, double data_x[], double data_y[], int *total_points, int *current_pos) {
    FILE *file = fopen(file_name, "r");  // Open the file for reading
    if (!file) {
        printf("Warning: Could not open file %s\n", file_name);  // Handle file opening failure
        return 0;
    }

    int count = 0;
    // Read each data pair (x, y) from the file and store in arrays
    while (fscanf(file, "%lf,%lf", &data_x[*current_pos], &data_y[*current_pos]) == 2) {
        (*current_pos)++;  // Increment position for the next data point
        count++;            // Count the number of data points read
    }
    fclose(file);  // Close the file after reading

    *total_points += count;  // Update the total number of data points
    return count > 0;  // Return 1 if at least one data point was read
}

// Compute the slope and intercept for linear regression
int compute_lr(const double data_x[], const double data_y[], int total_points, double *intercept, double *slope) {
    if (total_points == 0) {
        printf("Error: No data points available.\n");  // Error if no data is available
        return 0;
    }

    // Variables to store the sums required for the regression calculation
    double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
    // Calculate sums for x, y, x*y, and x^2
    for (int i = 0; i < total_points; i++) {
        sum_x += data_x[i];
        sum_y += data_y[i];
        sum_xy += data_x[i] * data_y[i];
        sum_x2 += data_x[i] * data_x[i];
    }

    // Compute means of x and y
    double mean_x = sum_x / total_points;
    double mean_y = sum_y / total_points;

    // Calculate the denominator for the slope formula
    double denominator = sum_x2 - total_points * mean_x * mean_x;

    if (denominator == 0) {
        printf("Error: Cannot compute regression (denominator is zero).\n");  // Error if denominator is zero
        return 0;
    }

    // Calculate the slope and intercept using the regression formula
    *slope = (sum_xy - total_points * mean_x * mean_y) / denominator;
    *intercept = mean_y - (*slope) * mean_x;
    return 1;  // Return 1 if computation is successful
}

int main() {
    // List of files containing data to be processed
    const char *file_list[] = {"datasetLR1.txt", "datasetLR2.txt", "datasetLR3.txt", "datasetLR4.txt"};
    const int dataset_count = 4;

    // Arrays to hold x and y values
    double data_x[1000], data_y[1000];
    int total_points = 0, current_pos = 0;

    // Process data from each file and load into arrays
    for (int i = 0; i < dataset_count; i++) {
        printf("Processing %s...\n", file_list[i]);
        load_data(file_list[i], data_x, data_y, &total_points, &current_pos);  // Load data from file
    }

    // Compute the linear regression coefficients (slope and intercept)
    double intercept, slope;
    if (!compute_lr(data_x, data_y, total_points, &intercept, &slope)) {
        return 1;  // Exit if regression calculation fails
    }

    // Output the final regression equation
    printf("Regression equation: y = %.2fx + %.2f\n", slope, intercept);

    // Ask the user for an x-value to calculate the corresponding y-value
    double user_input_x;
    printf("Enter a value for x: ");
    if (scanf("%lf", &user_input_x) != 1) {
        printf("Invalid input.\n");  // Error if input is not a valid number
        return 1;
    }

    // Calculate the corresponding y-value using the regression equation
    double result_y = slope * user_input_x + intercept;
    printf("For x = %.2f, y = %.2f\n", user_input_x, result_y);  

    return 0;  // Program executed successfully
}
