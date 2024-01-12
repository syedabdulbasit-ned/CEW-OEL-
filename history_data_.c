#include <stdio.h>
#include <curl/curl.h>
#include <jansson.h>

// Callback function to write the response from the API
size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    FILE* fp = (FILE*)userp;
    return fwrite(contents, size, nmemb, fp);
}

// Function to print weather information to the terminal
void print_weather_info(json_t* root) {
    // Extract relevant information and print to the terminal
    json_t* observations = json_object_get(root, "observations");
    size_t num_observations = json_array_size(observations);

    for (size_t i = 0; i < num_observations; ++i) {
        json_t* observation = json_array_get(observations, i);

        const char* timestamp = json_string_value(json_object_get(observation, "timestamp"));
        const char* temperature = json_string_value(json_object_get(observation, "temperature"));
        const char* humidity = json_string_value(json_object_get(observation, "humidity"));
        const char* sunrise = json_string_value(json_object_get(observation, "sunrise"));
        const char* sunset = json_string_value(json_object_get(observation, "sunset"));

        printf("Timestamp: %s, Temperature: %s, Humidity: %s, Sunrise: %s, Sunset: %s\n",
               timestamp, temperature, humidity, sunrise, sunset);
    }
}

int main() {
    CURL* curl;
    CURLcode res;
    
    // URL of the API
    const char* url_template = "https://api.weather.com/v3/wx/conditions/historical?apiKey=YOUR_API_KEY&language=en-US&format=json&date=%s&city=%s";
    
    // Prompt the user to enter the date in YYYYMMDD format
    char date[9];
    printf("Enter the date in YYYYMMDD format: ");
    scanf("%8s", date);

    // Prompt the user to enter the city
    char city[100];
    printf("Enter the city: ");
    scanf("%s", city);
    
    // Construct the URL with the user-provided date and city
    char url[256];
    snprintf(url, sizeof(url), url_template, date, city);
    
    // File to save raw JSON data
    char raw_data_file[256];
    snprintf(raw_data_file, sizeof(raw_data_file), "history_raw_data.json");
    FILE* raw_data_fp = fopen(raw_data_file, "wb");
    
    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    
    // Set the URL for the API request
    curl_easy_setopt(curl, CURLOPT_URL, url);
    
    // Set the callback function to write raw JSON data to file
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, raw_data_fp);
    
    // Perform the request
    res = curl_easy_perform(curl);
    
    // Check for errors
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }
    else {
        printf("Raw JSON data saved to %s\n", raw_data_file);
    }
    
    // Clean up libcurl
    curl_easy_cleanup(curl);
    
    // Close the raw data file
    fclose(raw_data_fp);

    // Now parse the JSON data and extract information for the previous three days
    FILE* raw_data_fp_read = fopen(raw_data_file, "rb");
    if (!raw_data_fp_read) {
        fprintf(stderr, "Error opening raw data file for reading.\n");
        return 1;
    }

    json_t* root;
    json_error_t error;
    root = json_loadf(raw_data_fp_read, 0, &error);
    fclose(raw_data_fp_read);

    if (!root) {
        fprintf(stderr, "Error parsing raw JSON data: %s (line %d)\n", error.text, error.line);
        return 1;
    }

    // Print weather information to the terminal
    print_weather_info(root);

    // Clean up the JSON object
    json_decref(root);

    return 0;
}
