#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "api.h"

// structure to store API response
struct MemoryStruct {
    char *memory;
    size_t size;
};


static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    
    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL) {
        printf("Error: Not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0; 

    return realsize;
}

// function to get environmental data from the API
char* get_env_data(const char *url) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;

   
    chunk.memory = malloc(1);
    chunk.size = 0;

    // initialize libcurl
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (curl) {
       
        curl_easy_setopt(curl, CURLOPT_URL, url);

        
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

        
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            free(chunk.memory);
            chunk.memory = NULL;
        }

        curl_easy_cleanup(curl);
    } else {
        printf("Error: Failed to initialize CURL\n");
    }

    curl_global_cleanup();
    return chunk.memory; // return the response data
}
