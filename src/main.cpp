#include <Arduino.h>
#include <OpenAI.h>
#include <Preferences.h>
#include <WiFi.h>
#include <SerialCommand.h>
#include <BluetoothSerial.h>

// Dev mode
#define DEV

// Constants
const String PRODUCT_NAME = "BComfy";
const String OPENAI_API_KEY = "<API_KEY>";
const String OPENAI_MODEL = "gpt-3.5-turbo-0125";
const String OPENAI_CHAT_SYSTEM = "Your job is to generate motivational messages for a person with anxiety, these messages must not exceed 100 tokens, return only the message";
const int OPENAI_CHAT_TOKENS = 200;
const float OPENAI_CHAT_TEMPERATURE = 0.7;
const float OPENAI_CHAT_PRESENCE_PENALTY = 0.5;
const float OPENAI_CHAT_FREQUENCY_PENALTY = 0.5;

// WiFi credentials
String ssid;
String password;

// Device
String device_id;

// Preferences
Preferences preferences;

// OpenAI
OpenAI openai(OPENAI_API_KEY.c_str());
OpenAI_ChatCompletion chat(openai);

// Serial Commands
SerialCommand commands;

class Commands {
    public:
        class Chat {
            public:
                static void ask() {
                    String message = String();

                    while (char* chunk = commands.next()) {
                        if (chunk == NULL)
                            break;
                        message += chunk + ' ';
                    }
                    message.remove(message.length() - 1);

                    OpenAI_StringResponse response = chat.message(message);
                    Serial.println(response.getAt(0));
                }
        };

        class Config {
            public:
                static void set() {
                    String key = String(commands.next());
                    String value = String();

                    while (char* chunk = commands.next()) {
                        if (chunk == NULL)
                            break;
                        value += chunk + ' ';
                    }
                    value.remove(value.length() - 1);

                    if (key == "wifi/ssid") {
                        preferences.putString(key.c_str(), value);
                        ssid = value;
                    } else if (key == "wifi/password") {
                        preferences.putString(key.c_str(), value);
                        password = value;
                    }
                }

                static void get() {
                    Serial.println(preferences.getString(commands.next(), ""));
                }
        };

        class WIFI {
            public:
                static void connect() {
                    WiFi.begin(ssid.c_str(), password.c_str());
                    wl_status_t status = (wl_status_t)WiFi.waitForConnectResult();

                    if (status == WL_CONNECTED) {
                        Serial.println("Connected to WiFi");
                    } else {
                        Serial.println("Failed to connect to WiFi");
                    }
                }

                static void disconnect() {
                    WiFi.disconnect();
                    Serial.println("Disconnected from WiFi");
                }
        };
};

void setup() {
    // Initialize
    #ifdef DEV
        esp_log_level_set("*", ESP_LOG_NONE);
    #endif

    // Set device ID
    device_id = PRODUCT_NAME + "-" + WiFi.macAddress();
    
    // Start serial interface
    Serial.begin(115200);

    // Start preferences
    preferences.begin(PRODUCT_NAME.c_str(), false);

    // Load WiFi credentials
    try {
        ssid = preferences.getString("wifi/ssid", "");
    } catch (const std::exception& e) {
        Serial.println("SSID not found in preferences");
    }

    try {
        password = preferences.getString("wifi/password", "");
    } catch (const std::exception& e) {
        Serial.println("Password not found in preferences");
    }

    // Connect to WiFi
    WiFi.begin(ssid.c_str(), password.c_str());

    // Wait for connection
    wl_status_t status = (wl_status_t)WiFi.waitForConnectResult();

    // Check connection status
    if (status == WL_CONNECTED) {
        Serial.println("Connected to WiFi");
    } else {
        Serial.println("Failed to connect to WiFi");
    }

    // Initialize OpenAI
    chat.setModel(OPENAI_MODEL.c_str());
    chat.setSystem(OPENAI_CHAT_SYSTEM.c_str());
    chat.setMaxTokens(OPENAI_CHAT_TOKENS);
    chat.setTemperature(OPENAI_CHAT_TEMPERATURE);
    chat.setPresencePenalty(OPENAI_CHAT_PRESENCE_PENALTY);
    chat.setFrequencyPenalty(OPENAI_CHAT_FREQUENCY_PENALTY);
    chat.setUser(device_id.c_str());

    // Initialize serial commands
    commands.addCommand("ask", Commands::Chat::ask);
    commands.addCommand("config/set", Commands::Config::set);
    commands.addCommand("config/get", Commands::Config::get);
    commands.addCommand("wifi/connect", Commands::WIFI::connect);
    commands.addCommand("wifi/disconnect", Commands::WIFI::disconnect);
}

void loop() {
    // Read commands
    commands.readSerial();
    commands.clearBuffer();
}
