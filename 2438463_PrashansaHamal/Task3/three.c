#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>

// Mutex for thread synchronization
pthread_mutex_t mutex;

// Structure to hold data for each thread
typedef struct {
    int* numbers;
    int start;
    int end;
    int* primes;
    int* prime_count;
} ThreadData;

// Function to check if a number is prime
bool is_prime(int n) {
    if (n < 2) return false;
    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) return false;
    }
    return true;
}

// Function executed by each thread to find prime numbers
void* find_primes(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    for (int i = data->start; i < data->end; i++) {
        if (is_prime(data->numbers[i])) {
            pthread_mutex_lock(&mutex); // Lock before modifying shared resources
            data->primes[data->prime_count[0]] = data->numbers[i];
            data->prime_count[0]++;
            pthread_mutex_unlock(&mutex); // Unlock after modification
        }
    }
    pthread_exit(NULL);
}

int main() {
    int num_files;
    printf("Enter the number of files: ");
    if (scanf("%d", &num_files) != 1 || num_files <= 0) {
        printf("Invalid number of files.\n");
        return 1;
    }

    // Allocate memory for file names
    char** filenames = malloc(num_files * sizeof(char*));
    if (!filenames) {
        perror("Failed to allocate memory");
        return 1;
    }

    // Read file names from the user
    for (int i = 0; i < num_files; i++) {
        filenames[i] = malloc(256 * sizeof(char)); // Allocate memory for each filename
        if (!filenames[i]) {
            perror("Failed to allocate memory");
            return 1;
        }
        printf("Enter the name of file %d: ", i + 1);
        scanf("%255s", filenames[i]);
    }

    int total_numbers = 0;
    int* numbers = NULL;
    FILE* file;

    // Read numbers from each file
    for (int i = 0; i < num_files; i++) {
        file = fopen(filenames[i], "r");
        if (!file) {
            fprintf(stderr, "Failed to open file: %s\n", filenames[i]);
            perror("Error");
            return 1;
        }
        int num;
        while (fscanf(file, "%d", &num) == 1) {
            numbers = realloc(numbers, (total_numbers + 1) * sizeof(int));
            if (!numbers) {
                perror("Failed to allocate memory");
                return 1;
            }
            numbers[total_numbers++] = num;
        }
        fclose(file);
    }

    // Free filenames
    for (int i = 0; i < num_files; i++) {
        free(filenames[i]);
    }
    free(filenames);

    int T;
    printf("Enter number of threads: ");
    if (scanf("%d", &T) != 1 || T <= 0) {
        printf("Invalid number of threads.\n");
        return 1;
    }

    // Initialize mutex
    pthread_mutex_init(&mutex, NULL);

    // Allocate memory for thread data
    ThreadData* thread_data = malloc(T * sizeof(ThreadData));
    int* primes = malloc(total_numbers * sizeof(int));
    int* prime_counts = calloc(1, sizeof(int)); // Store the total prime count

    // Divide numbers among threads
    int base_chunk = total_numbers / T;
    int remainder = total_numbers % T;
    int current_start = 0;

    pthread_t threads[T];

    for (int i = 0; i < T; i++) {
        int chunk = base_chunk + (i < remainder ? 1 : 0);
        thread_data[i].numbers = numbers;
        thread_data[i].start = current_start;
        thread_data[i].end = current_start + chunk;
        thread_data[i].primes = primes;
        thread_data[i].prime_count = prime_counts;
        current_start += chunk;
        pthread_create(&threads[i], NULL, find_primes, &thread_data[i]);
    }

    // Wait for threads to finish
    for (int i = 0; i < T; i++) {
        pthread_join(threads[i], NULL);
    }

    // Write prime numbers to output file
    file = fopen("output.txt", "w");
    if (!file) {
        perror("Failed to open output file");
        return 1;
    }

    fprintf(file, "Total prime numbers found: %d\n", *prime_counts);
    fprintf(file, "Prime numbers:\n");
    for (int i = 0; i < *prime_counts; i++) {
        fprintf(file, "%d\n", primes[i]);
    }
    fclose(file);

    // Free allocated memory
    free(numbers);
    free(thread_data);
    free(primes);
    free(prime_counts);

    // Destroy mutex
    pthread_mutex_destroy(&mutex);

    printf("Prime numbers have been written to output.txt\n");
    return 0;
}
