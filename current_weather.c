#include <stdio.h>
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

    // Allocate memory for time string
    char *time_str = malloc(20); // Sufficient size for HH:MM:SS
    strftime(time_str, 20, "%H:%M:%S", timeinfo);

    return time_str;
}

// Function to get formatted time from timestamp
char *get_formatted_time(double timestamp) {
    time_t time_value = (time_t)timestamp;
    struct tm *timeinfo = localtime(&time_value);

    // Allocate memory for formatted time string
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
void process_weather_and_alert(const char *city) {
    CURL *curl;
    CURLcode res;

    // Initializing the curl library
    curl = curl_easy_init();

    if (curl) {
        // API used: OpenWeatherMap API
        char url[200];
        sprintf(url, "http://api.openweathermap.org/data/2.5/weather?q=%s&appid=4c52ab63a1f5c4ebe2aadc3d32809aad", city);

        // Struct to store the received data
        struct MemoryStruct chunk;
        chunk.memory = malloc(1);  // Initialize with a minimum size
        chunk.size = 0;

        // Setting the URL to fetch
        curl_easy_setopt(curl, CURLOPT_URL, url);

        // Set the callback function and user data
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        // Performing the request
        res = curl_easy_perform(curl);

        // Error checking
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            // Data retrieval successful, try to extract key information using Jansson
            json_error_t error;
            json_t *root = json_loads(chunk.memory, 0, &error);
            if (root != NULL) {
                // Extract key information
                json_t *name = json_object_get(root, "name");
                json_t *main = json_object_get(root, "main");
                json_t *weather = json_object_get(root, "weather");
                json_t *wind = json_object_get(root, "wind");
                json_t *clouds = json_object_get(root, "clouds");
                json_t *sys = json_object_get(root, "sys");

                if (json_is_string(name) && json_is_object(main) && json_is_array(weather) && json_is_object(wind) &&
                    json_is_object(clouds) && json_is_object(sys)) {
                    // Weather Report
                    printf("Weather Report for %s:\n", json_string_value(name));

                    // Temperature
                    printf("Temperature: %.2f°C\n", json_number_value(json_object_get(main, "temp")) - 273.15);

                    // Feels Like
                    printf("Feels Like: %.2f°C\n", json_number_value(json_object_get(main, "feels_like")) - 273.15);

                    // Temperature Min
                    printf("Temperature Min: %.2f°C\n", json_number_value(json_object_get(main, "temp_min")) - 273.15);

                    // Temperature Max
                    printf("Temperature Max: %.2f°C\n", json_number_value(json_object_get(main, "temp_max")) - 273.15);

                    // Pressure
                    printf("Pressure: %.2fhPa\n", json_number_value(json_object_get(main, "pressure")));

                    // Humidity
                    printf("Humidity: %d%%\n", (int)json_number_value(json_object_get(main, "humidity")));

                    // Visibility
                    printf("Visibility: %d meters\n", (int)json_number_value(json_object_get(root, "visibility")));

                    // Wind Speed
                    printf("Wind Speed: %.2fm/s\n", json_number_value(json_object_get(wind, "speed")));

                    // Cloudiness
                    printf("Cloudiness: %d%%\n", (int)json_number_value(json_object_get(clouds, "all")));

                    // Weather Description
                    json_t *weather_description = json_array_get(weather, 0);
                    printf("Weather: %s\n", json_string_value(json_object_get(weather_description, "description")));

                    // Sunrise
                    printf("Sunrise: %s\n", get_formatted_time(json_number_value(json_object_get(sys, "sunrise"))));

                    // Sunset
                    printf("Sunset: %s\n", get_formatted_time(json_number_value(json_object_get(sys, "sunset"))));

                    // Save data to a file
                    save_data_to_file("weather_report.txt", chunk.memory);

                    // Send email alert
                    // send_alert_email("Weather Alert", "Weather conditions have been updated. Check the attached file for details.");

                    // API Response
                    printf("API Response Code: %d\n", (int)json_integer_value(json_object_get(root, "cod")));

                    // API Response Message
                    json_t *api_response_msg = json_object_get(root, "message");
                    if (json_is_string(api_response_msg)) {
                        printf("API Response Message: %s\n", json_string_value(api_response_msg));
                    } else {
                        printf("API Response Message: (not available)\n");
                    }

                    printf("API response indicates success.\n");
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
        free(chunk.memory);

        // Cleanup
        curl_easy_cleanup(curl);
    }
}

int main() {
    char city[100];
    const char city[]="london"
    
    // Process weather and send email alert
    process_weather_and_alert(city);

    return 0;
}

