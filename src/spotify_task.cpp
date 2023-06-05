#include <Arduino.h>
#include <WiFiClientSecure.h>

#include <SpotifyArduino.h>
#include <SpotifyArduinoCert.h>
#include <ArduinoJson.h>

#include "../include/config.h"

WiFiClientSecure client;
SpotifyArduino spotify(client, SPOTIFY_CLIENT_ID, SPOTIFY_CLIENT_SECRET, SPOTIFY_REFRESH_TOKEN);

unsigned long delayBetweenRequests = 60000; // Time between requests (1 minute)
unsigned long requestDueTime;               //time when request due


void printCurrentlyPlayingToSerial(CurrentlyPlaying currentlyPlaying) {
    // Use the details in this method or if you want to store them
    // make sure you copy them (using something like strncpy)
    // const char* artist =

    Serial.println("--------- Currently Playing ---------");

    Serial.print("Is Playing: ");
    if (currentlyPlaying.isPlaying) {
        Serial.println("Yes");
    } else {
        Serial.println("No");
    }

    Serial.print("Track: ");
    Serial.println(currentlyPlaying.trackName);
    Serial.print("Track URI: ");
    Serial.println(currentlyPlaying.trackUri);
    Serial.println();

    Serial.println("Artists: ");
    for (int i = 0; i < currentlyPlaying.numArtists; i++) {
        Serial.print("Name: ");
        Serial.println(currentlyPlaying.artists[i].artistName);
        Serial.print("Artist URI: ");
        Serial.println(currentlyPlaying.artists[i].artistUri);
        Serial.println();
    }

    Serial.print("Album: ");
    Serial.println(currentlyPlaying.albumName);
    Serial.print("Album URI: ");
    Serial.println(currentlyPlaying.albumUri);
    Serial.println();

    if (currentlyPlaying.contextUri != NULL) {
        Serial.print("Context URI: ");
        Serial.println(currentlyPlaying.contextUri);
        Serial.println();
    }

    long progress = currentlyPlaying.progressMs;  // duration passed in the song
    long duration = currentlyPlaying.durationMs;  // Length of Song
    Serial.print("Elapsed time of song (ms): ");
    Serial.print(progress);
    Serial.print(" of ");
    Serial.println(duration);
    Serial.println();

    float percentage = ((float)progress / (float)duration) * 100;
    int clampedPercentage = (int)percentage;
    Serial.print("<");
    for (int j = 0; j < 50; j++) {
        if (clampedPercentage >= (j * 2)) {
            Serial.print("=");
        } else {
            Serial.print("-");
        }
    }
    Serial.println(">");
    Serial.println();

    // will be in order of widest to narrowest
    // currentlyPlaying.numImages is the number of images that
    // are stored
    for (int i = 0; i < currentlyPlaying.numImages; i++) {
        Serial.println("------------------------");
        Serial.print("Album Image: ");
        Serial.println(currentlyPlaying.albumImages[i].url);
        Serial.print("Dimensions: ");
        Serial.print(currentlyPlaying.albumImages[i].width);
        Serial.print(" x ");
        Serial.print(currentlyPlaying.albumImages[i].height);
        Serial.println();
    }
    Serial.println("------------------------");
}


void spotify_task(void *param) {
    // Handle HTTPS Verification
    client.setCACert(spotify_server_cert);

    Serial.println("Refreshing Access Tokens");
    if (!spotify.refreshAccessToken()) {
        Serial.println("Failed to get access tokens");
    }

    while (1) {
        if (millis() > requestDueTime) {
            Serial.print("Free Heap: ");
            Serial.println(ESP.getFreeHeap());

            Serial.println("getting currently playing song:");
            // Market can be excluded if you want e.g. spotify.getCurrentlyPlaying()
            int status = spotify.getCurrentlyPlaying(printCurrentlyPlayingToSerial, SPOTIFY_MARKET);
            if (status == 200) {
                Serial.println("Successfully got currently playing");
            } else if (status == 204) {
                Serial.println("Doesn't seem to be anything playing");
            } else {
                Serial.print("Error: ");
                Serial.println(status);
            }
            requestDueTime = millis() + delayBetweenRequests;
        }
    }
    vTaskDelete(NULL);
}
