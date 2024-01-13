#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <jansson.h>
#include <time.h>

struct MemoryStruct {
    char *memory;
    size_t size;
};

// Callback function to handle the received data
size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total_size = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    // Reallocate memory to fit the new data
    char *new_memory = realloc(mem->memory, mem->size + total_size + 1);
    if (new_memory == NULL) {
        fprintf(stderr, "Failed to reallocate memory.\n");
        return 0;  // Stop further processing
    }

    // Copy data to the buffer and null-terminate
    memcpy(&(new_memory[mem->size]), contents, total_size);
    new_memory[mem->size + total_size] = '\0';

    // Update the memory and size
    mem->memory = new_memory;
    mem->size += total_size;

    return total_size;
}

// Function to get current time as a string
char *get_current_time() {
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    // Allocate memory for the time string
    char *time_str = malloc(20); // Sufficient size for HH:MM:SS
    strftime(time_str, 20, "%H:%M:%S", timeinfo);

    return time_str;
}

// Function to get formatted time from timestamp
char *get_formatted_time(double timestamp) {
    time_t time_value = (time_t)timestamp;
    struct tm *timeinfo = localtime(&time_value);

    // Allocate memory for the formatted time string
    char *time_str = malloc(20); // Sufficient size for HH:MM:SS
    strftime(time_str, 20, "%H:%M:%S", timeinfo);

    return time_str;
}

// Function to save data to a file
void save_data_to_file(const char *filename, const char *data) {
    FILE *file = fopen(filename, "a"); // Open in append mode
    if (file != NULL) {
        fprintf(file, "Data stored at %s:\n%s\n\n", get_current_time(), data);
        fclose(file);
        printf("Data successfully saved in %s.\n", filename);
    } else {
        fprintf(stderr, "Error: Unable to open file for writing.\n");
    }
    free(get_current_time()); // Free memory allocated by get_current_time
}

// Function to process weather and send alert
void process_weather_and_alert(const char *city, const char *date) {
    CURL *curl;
    CURLcode res;

    // Initializing the curl library
    curl = curl_easy_init();

    if (curl) {
        // API used: OpenWeatherMap API
        char url[200];
        sprintf(url, "http://api.openweathermap.org/data/2.5/forecast?q=%s&dt=%s&appid=259cad1082d5039077642678f8a2cc69", city, date);

        // Struct to store the received data
        struct MemoryStruct raw_chunk;
        raw_chunk.memory = malloc(1);  // Initialize with a minimum size
        raw_chunk.size = 0;

        // Setting the URL to fetch
        curl_easy_setopt(curl, CURLOPT_URL, url);

        // Set the callback function and user data
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&raw_chunk);

        // Performing the request
        res = curl_easy_perform(curl);

        // Error checking
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            // Data retrieval successful, try to extract key information using Jansson
            json_error_t error;
            json_t *root = json_loads(raw_chunk.memory, 0, &error);
            if (root != NULL) {
                // Save raw data to a file
                save_data_to_file("raw_weather_forecast.txt", raw_chunk.memory);

                // Extract key information
                // (Note: This is a basic example, you may need to adapt it based on the actual JSON structure)
                json_t *city_obj = json_object_get(root, "city");
                json_t *list = json_object_get(root, "list");

                if (json_is_object(city_obj) && json_is_array(list)) {
                    printf(" --------------- Weather Forecast Report for %s ---------------:\n", json_string_value(json_object_get(city_obj, "name")));

                    // Process forecast data (this is a basic example, you may need to adapt based on the JSON structure)
                    for (size_t i = 0; i < json_array_size(list); i++) {
                        json_t *item = json_array_get(list, i);
                        if (json_is_object(item)) {
                            json_t *main = json_object_get(item, "main");
                            json_t *dt = json_object_get(item, "dt_txt");
                            json_t *weather_array = json_object_get(item, "weather");

                            if (json_is_array(weather_array) && json_is_string(dt)) {
                                json_t *weather_obj = json_array_get(weather_array, 0);
                                if (json_is_object(weather_obj)) {
                                    // Temperature
                                    double temperature = json_number_value(json_object_get(main, "temp"));
                                    printf("Date/Time: %s, Temperature: %.2f°C\n", json_string_value(dt), temperature - 273.15);

                                    // Weather Description
                                    const char *description = json_string_value(json_object_get(weather_obj, "description"));
                                    printf("Weather: %s\n", description);

                                    // Humidity
                                    double humidity = json_number_value(json_object_get(main, "humidity"));
                                    printf("Humidity: %.2f%%\n", humidity);

                                    // Print a separator
                                    printf("----------------------------\n");
                                }
                            }
                        }
                    }

                    // Processed data (this is a basic example, you may need to adapt based on the JSON structure)
                    char processed_data[500]; // Adjust the size as needed
                    sprintf(processed_data, "Processed data for %s on %s goes here.", city, date);

                    // Save processed data to a file
                    save_data_to_file("processed_weather_forecast.txt", processed_data);
                } else {
                    printf("Error: Invalid or missing data in API response.\n");
                }

                // Free Jansson root
                json_decref(root);
            } else {
                // Print detailed Jansson error
                fprintf(stderr, "Jansson error: %s (line %d, column %d)\n", error.text, error.line, error.column);
                printf("Error: Unable to parse JSON data.\n");
            }
        }

        // Free allocated memory
        free(raw_chunk.memory);

        // Cleanup
        curl_easy_cleanup(curl);
    }
}

int main() {
    char city[100] = "London";  // Hardcoded city name
    char date[100] = "2024-01-13";  // Hardcoded date in the format "YYYY-MM-DD"

    // Process weather and send email alert
    process_weather_and_alert(city, date);

    return 0;
}
