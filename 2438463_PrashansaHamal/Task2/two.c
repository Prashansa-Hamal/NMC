#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
    int start;
    int end;
} ThreadData;

pthread_mutex_t lock;
double pi_value = 0.0;

// Function to calculate pi for a given range
void *calculate_pi(void *arg) {
    // Cast the argument to ThreadData
    ThreadData *data = (ThreadData *)arg;
    int start = data->start;
    int end = data->end;
    double local_sum = 0.0;

    // Leibniz series calculation
    for (int i = start; i < end; i++) {
        double term = pow(-1, i) / (2 * i + 1);
        local_sum += term;
    }

    // Lock before updating shared variable
    pthread_mutex_lock(&lock);
    pi_value += local_sum;
    pthread_mutex_unlock(&lock);

    // Free allocated memory
    free(data);
    return NULL;
}

int main() {
    int n, nThreads;
    
    pthread_mutex_init(&lock, NULL);

    printf("Enter the number of iterations: ");
    scanf("%d", &n);

    printf("Enter the number of threads to be used: ");
    scanf("%d", &nThreads);

    // Ensure we don't have more threads than iterations
    if (nThreads > n) {
        nThreads = n;
    }

    pthread_t threads[nThreads];

    // Calculate the number of iterations per thread
    int base_iterations = n / nThreads;
    int extra_iterations = n % nThreads;
    int start = 0;

    // Create threads
    for (int i = 0; i < nThreads; i++) {
        // Allocate memory for thread data
        ThreadData *data = malloc(sizeof(ThreadData));
        if (data == NULL) {
            fprintf(stderr, "Memory allocation failed!\n");
            return 1;
        }

        // Set start index for this thread
        data->start = start;

        // Each thread gets base_iterations; the first 'extra_iterations' threads get 1 extra
        if (i < extra_iterations) {
            data->end = start + base_iterations + 1;
        } else {
            data->end = start + base_iterations;
        }

        // Update start for the next thread
        start = data->end;

        // Create the thread and check for failure
        if (pthread_create(&threads[i], NULL, calculate_pi, data) != 0) {
            printf("Thread creation failed!\n");
            free(data);
            return 1;
        }
    }

    // Wait for all threads to complete
    for (int i = 0; i < nThreads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Destroy mutex
    pthread_mutex_destroy(&lock);

    // Multiply final sum by 4 to get pi
    pi_value *= 4;

    // Print result
    printf("The value of pi after %d iterations is %.15f\n", n, pi_value);

    return 0;
}