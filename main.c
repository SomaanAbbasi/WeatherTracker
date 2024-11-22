#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>  // for system() function to run shell commands
#include "api.h"


// Function prototypes (so functions can be called before they are defined in the code)
void parse_and_save_data(const char *data);
void save_temperature_log(double temp_c, const char *timestamp);
double calculate_average_temperature(const char *filename, const char *current_date);
void free_resources(char **resources, int count);

int main() {
    const char *api_key = "621ab0e6a5954b4689e140919241911"; // api key from weatherapi(dot)com
    const char *location = "Karachi";    // required city

    // constructs API dynamically, so API key or location can easily be changed
    char *url = (char *)malloc(256 * sizeof(char));
    if (!url) {
        fprintf(stderr, "Error: Memory allocation failed for URL.\n");
        return 1;
    }

    snprintf(url, 256, "http://api.weatherapi.com/v1/current.json?key=%s&q=%s", api_key, location);

    // get date using API
    char *data = get_env_data(url);
    free(url); // free URL memory, no longer needed

    if (data) {
        
        parse_and_save_data(data);
        free(data); // free allocated memory for the JSON
    } else {
        fprintf(stderr, "Error: Failed to retrieve data from WeatherAPI.\n");
        return 1;
    }

    return 0;
}

// sends notification is temp is high
void send_high_temp_notification(double temp_c, const char *location) {
    char command[512];
    snprintf(command, sizeof(command), "notify-send 'High Temperature Warning' 'Its %.2f°C in %s'", temp_c, location);
    system(command); 
}

void parse_and_save_data(const char *data) {
    // code to allocate memory dynamically for parsed values
    char *location = (char *)malloc(50 * sizeof(char));
    char *condition = (char *)malloc(50 * sizeof(char));
    char timestamp[20];
    if (!location || !condition) {
        fprintf(stderr, "Error: Memory allocation failed for parsed values.\n");
        free_resources((char *[]){location, condition}, 2);
        return;
    }

    double temp_c = 0.0;
    int humidity = 0;

    // code to parse JSON
    char *location_ptr = strstr(data, "\"name\":\"");
    char *temp_c_ptr = strstr(data, "\"temp_c\":");
    char *humidity_ptr = strstr(data, "\"humidity\":");
    char *condition_ptr = strstr(data, "\"text\":\"");
    char *time_ptr = strstr(data, "\"localtime\":\"");

    if (location_ptr && temp_c_ptr && humidity_ptr && condition_ptr && time_ptr) {
        sscanf(location_ptr + 8, "%[^\"]", location);
        sscanf(condition_ptr + 8, "%[^\"]", condition);
        sscanf(time_ptr + 13, "%19[^\"]", timestamp);
        temp_c = atof(temp_c_ptr + 9);
        humidity = atoi(humidity_ptr + 11);

        // Saves temperature and timestamp to log
        save_temperature_log(temp_c, timestamp);

        
        if (temp_c > 30.0) {
            printf("⚠️ Warning: High temperature detected! %.2f°C at %s\n", temp_c, timestamp);

            // sends a desktop notif
            send_high_temp_notification(temp_c, location);

            // this saves warning notification to log file for future reference
            FILE *notif_file = fopen("notifications.log", "a");
            if (notif_file) {
                fprintf(notif_file, "High temperature warning: %.2f°C at %s in %s\n", temp_c, timestamp, location);
                fclose(notif_file);
            } else {
                fprintf(stderr, "Error: Could not open notifications log file.\n");
            }
        }

        // calcs average temp
        char date[11];
        strncpy(date, timestamp, 10);
        date[10] = '\0'; // Ensure null termination
        double avg_temp = calculate_average_temperature("temperature_log.txt", date);

        // print results cleanly
        printf("Location: %s\n", location);
        printf("Temperature Now: %.2f°C\n", temp_c);
        printf("Humidity: %d%%\n", humidity);
        printf("Condition: %s\n", condition);
        if (avg_temp >= 0) {
            printf("Average Temperature Today: %.2f°C\n", avg_temp);
        } else {
            printf("Average Temperature Today: No data available\n");
        }

        // saves raw data from api call to file
        FILE *raw_file = fopen("raw_weather_data.json", "w");
        if (raw_file) {
            fwrite(data, sizeof(char), strlen(data), raw_file);
            fclose(raw_file);
        } else {
            fprintf(stderr, "Error: Could not open file for raw data.\n");
        }

        // saves formatted/processed data to file
        FILE *processed_file = fopen("processed_weather_data.txt", "w");
        if (processed_file) {
            fprintf(processed_file, "Location: %s\n", location);
            fprintf(processed_file, "Temperature (C): %.2f\n", temp_c);
            fprintf(processed_file, "Humidity: %d%%\n", humidity);
            fprintf(processed_file, "Condition: %s\n", condition);
            fprintf(processed_file, "Timestamp: %s\n", timestamp);
            fclose(processed_file);
        } else {
            fprintf(stderr, "Error: Could not open file for processed data.\n");
        }
    } else {
        fprintf(stderr, "Error: Failed to parse JSON data.\n");
    }

    // frees memory allocated previously
    free_resources((char *[]){location, condition}, 2);
}

// function to save temp and time to log file
void save_temperature_log(double temp_c, const char *timestamp) {
    FILE *log_file = fopen("temperature_log.txt", "a");
    if (log_file) {
        fprintf(log_file, "%s %.2f\n", timestamp, temp_c);
        fclose(log_file);
    } else {
        fprintf(stderr, "Error: Could not open temperature log file.\n");
    }
}

double calculate_average_temperature(const char *filename, const char *current_date) {
    FILE *log_file = fopen(filename, "r");
    if (!log_file) {
        fprintf(stderr, "Error: Could not open temperature log file for reading.\n");
        return -1;
    }

    char date[11];
    char time[6];
    double temp_c;
    double total_temp = 0;
    int count = 0;

    char line[50];
    while (fgets(line, sizeof(line), log_file)) {
        
        if (sscanf(line, "%10s %5s %lf", date, time, &temp_c) == 3) {
            // checks if the date matches the current date
            if (strcmp(date, current_date) == 0) {
                total_temp += temp_c;
                count++;
            }
        }
    }
    fclose(log_file);

    if (count == 0) {
        return -1;
    }
    return total_temp / count;
}



//  this frees resources allocated earlier
void free_resources(char **resources, int count) {
    for (int i = 0; i < count; ++i) {
        if (resources[i]) {
            free(resources[i]);
        }
    }
}
