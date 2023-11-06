#include <driver/rmt.h>
#include "M5StickCPlus.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <WiFiGeneric.h>
#include <WiFiMulti.h>

#define RF433RX

#define RMT_TX_CHANNEL  RMT_CHANNEL_0
#define RMT_RX_CHANNEL  RMT_CHANNEL_1
#define RTM_TX_GPIO_NUM 32
#define RTM_RX_GPIO_NUM 33
#define RTM_BLOCK_NUM   1

#define RMT_CLK_DIV   80
#define RMT_1US_TICKS (80000000 / RMT_CLK_DIV / 1000000)
#define RMT_1MS_TICKS (RMT_1US_TICKS * 1000)

rmt_item32_t rmtbuff[2048];

#define T0H 670
#define T1H 320
#define T0L 348
#define T1L 642

#define RMT_CODE_H \
    { 670, 1, 320, 0 }
#define RMT_CODE_L \
    { 348, 1, 642, 0 }
#define RMT_START_CODE0 \
    { 4868, 1, 2469, 0 }
#define RMT_START_CODE1 \
    { 1647, 1, 315, 0 }

char textMessage[31];
char receivedMessage[256] = "";

bool wifiHotspotEnabled = false; // Set to true to enable Wi-Fi hotspot, false to disable

#define LED_PIN 10 // Define the GPIO pin for the LED

void initRMT() {
#ifndef RF433RX
    rmt_config_t txconfig;
    txconfig.rmt_mode                 = RMT_MODE_TX;
    txconfig.channel                  = RMT_TX_CHANNEL;
    txconfig.gpio_num                 = gpio_num_t(RTM_TX_GPIO_NUM);
    txconfig.mem_block_num            = RTM_BLOCK_NUM;
    txconfig.tx_config.loop_en        = false;
    txconfig.tx_config.carrier_en     = false;
    txconfig.tx_config.idle_output_en = true;
    txconfig.tx_config.idle_level     = rmt_idle_level_t(0);
    txconfig.clk_div                  = RMT_CLK_DIV;

    ESP_ERROR_CHECK(rmt_config(&txconfig));
    ESP_ERROR_CHECK(rmt_driver_install(txconfig.channel, 0, 0));
#else
    rmt_config_t rxconfig;
    rxconfig.rmt_mode            = RMT_MODE_RX;
    rxconfig.channel             = RMT_RX_CHANNEL;
    rxconfig.gpio_num            = gpio_num_t(RTM_RX_GPIO_NUM);
    rxconfig.mem_block_num       = 6;
    rxconfig.clk_div             = RMT_CLK_DIV;
    rxconfig.rx_config.filter_en = true;
    rxconfig.rx_config.filter_ticks_thresh =
        200 * RMT_1US_TICKS;
    rxconfig.rx_config.idle_threshold = 3 * RMT_1MS_TICKS;

    ESP_ERROR_CHECK(rmt_config(&rxconfig));
    ESP_ERROR_CHECK(rmt_driver_install(rxconfig.channel, 2048, 0));
#endif
}

void sendTextMessage(const char* message, size_t size) {
    uint8_t* buff = (uint8_t*)message;

    rmtbuff[0] = (rmt_item32_t){RMT_START_CODE0};
    rmtbuff[1] = (rmt_item32_t){RMT_START_CODE1};
    for (size_t i = 0; i < size; i++) {
        uint8_t mark = 0x80;
        for (int n = 0; n < 8; n++) {
            rmtbuff[2 + i * 8 + n] = ((buff[i] & mark))
                                         ? ((rmt_item32_t){RMT_CODE_H})
                                         : ((rmt_item32_t){RMT_CODE_L});
            mark >>= 1;
        }
    }
    for (int i = 0; i < 8; i++) {
        ESP_ERROR_CHECK(rmt_write_items(RMT_TX_CHANNEL, rmtbuff, (2 + size * 8), false));
        ESP_ERROR_CHECK(rmt_wait_tx_done(RMT_TX_CHANNEL, portMAX_DELAY));
    }
}

// Set the Telnet server IP address to 192.168.4.1
IPAddress telnetServerIP(192, 168, 4, 1);

// Define your Wi-Fi credentials
const char* ssid = "RF433TelnetChat";
const char* password = "66666666";

WiFiServer telnetServer(23);
WiFiClient telnetClient;

void setupWiFi() {
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(telnetServerIP, telnetServerIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(ssid, password);
}

// Morse code lookup table and functions
const char* morseCodeTable[26] = {
    ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---", "-.-", ".-..",
    "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-", "..-", "...-", ".--", "-..-",
    "-.--", "--.."
};

const char* morseCodePunctuation[6] = {
    ".-.-.-", "--..--", "---...", "-...-", ".----.", "-....-"
};

const char* getMorseCode(char c) {
    if (isAlpha(c)) {
        return morseCodeTable[toupper(c) - 'A'];
    } else if (isDigit(c)) {
        return morseCodeTable[c - '0' + 26];
    } else if (c == ' ') {
        return " ";
    } else if (ispunct(c)) {
        switch (c) {
            case '.':
                return morseCodePunctuation[0];
            case ',':
                return morseCodePunctuation[1];
            case ':':
                return morseCodePunctuation[2];
            case '-':
                return morseCodePunctuation[3];
            case '\'':
                return morseCodePunctuation[4];
            case '"':
                return morseCodePunctuation[5];
            default:
                return "";
        }
    } else {
        return "";
    }
}

void textToMorse(const char* text, char* morse, size_t maxMorseSize) {
    memset(morse, 0, maxMorseSize);
    size_t morsePos = 0;

    for (size_t i = 0; i < strlen(text); i++) {
        char morseChar[6] = {0};
        const char* code = getMorseCode(text[i]);

        if (strlen(code) > 0) {
            strcpy(morseChar, code);
            strcat(morse, morseChar);
            strcat(morse, " "); // Add space between characters
        }
    }
}

void setup() {
    M5.begin();
    M5.Lcd.setRotation(1);
    M5.Lcd.fillScreen(TFT_BLACK);
    M5.Lcd.setCursor(5, 40);
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.setTextSize(2);
    
    // Initialize the LED pin and turn it off
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    pinMode(RTM_TX_GPIO_NUM, OUTPUT);
    pinMode(RTM_RX_GPIO_NUM, INPUT);
    initRMT();

    // Check and set up the Wi-Fi hotspot based on the wifiHotspotEnabled variable
    if (wifiHotspotEnabled) {
        setupWiFi();
        telnetServer.begin();
    }

    delay(100);
}

int parsedData(rmt_item32_t* item, size_t size, char* textptr, size_t maxsize) {
    if (size < 4) return -1;
    int cnt        = 0;
    uint8_t data   = 0;
    uint8_t bitcnt = 0, char_cnt = 0;
    if (((item[0].level0 == 0)) && (item[0].duration0 > 2300) &&
        (item[0].duration0 < 2600)) {
        rmt_item32_t dataitem;
        dataitem.level0    = 1;
        dataitem.level1    = 0;
        dataitem.duration0 = item[0].duration1;
        do {
            cnt++;
            dataitem.duration1 = item[cnt].duration0;
            if (cnt > 1) {
                if (((dataitem.duration0 + dataitem.duration1) < 1100) &&
                    ((dataitem.duration0 + dataitem.duration1) > 800)) {
                    data <<= 1;
                    if (dataitem.duration0 > dataitem.duration1) {
                        data += 1;
                    }

                    bitcnt++;
                    if (bitcnt >= 8) {
                        if (char_cnt < maxsize) {
                            textptr[char_cnt] = data;
                            char_cnt++;
                        }
                        data = 0;
                        bitcnt = 0;
                    }
                } else {
                    if (char_cnt >= maxsize) {
                        return char_cnt;
                    }
                    textptr[char_cnt] = '\0';
                    return char_cnt;
                }
            }
            dataitem.duration0 = item[cnt].duration1;
        } while (cnt < size);
    }
    if (char_cnt >= maxsize) {
        return char_cnt;
    }
    textptr[char_cnt] = '\0';
    return char_cnt;
}

void handleError() {
    // Add error handling logic here.
}

void loop() {
#ifndef RF433RX
    if (M5.BtnA.wasPressed()) {
        Serial.println("SEND");
        const char* messageToSend = "This is a test message!";
        sendTextMessage(messageToSend, strlen(messageToSend));
    }
#else
    static bool messageDisplayed = false;
    
    RingbufHandle_t rb = nullptr;
    rmt_get_ringbuf_handle(RMT_RX_CHANNEL, &rb);
    rmt_rx_start(RMT_RX_CHANNEL, true);
    
    size_t rx_size = 0;
    rmt_item32_t* item = (rmt_item32_t*)xRingbufferReceive(rb, &rx_size, 500);
    if (item != nullptr) {
        if (rx_size != 0) {
            int size = parsedData(item, rx_size, receivedMessage, 255);
            if (size >= 1 && !messageDisplayed) {
                // Clear the screen before displaying the new message
                M5.Lcd.fillScreen(TFT_BLACK);
                M5.Lcd.setCursor(5, 40);
                M5.Lcd.setTextColor(TFT_WHITE);
                M5.Lcd.setTextSize(2);
                M5.Lcd.println(receivedMessage);

                // Convert the received message to Morse code
                char morseMessage[1024] = {0};
                textToMorse(receivedMessage, morseMessage, sizeof(morseMessage));

                // Play Morse code on the built-in buzzer
                for (size_t i = 0; i < strlen(morseMessage); i++) {
                    char morseChar = morseMessage[i];
                    if (morseChar == '.') {
                        M5.Beep.tone(1000); // Dot: 1000 Hz
                        digitalWrite(LED_PIN, HIGH); // Turn on the LED
                        delay(300);         // Short beep duration
                        digitalWrite(LED_PIN, LOW); // Turn off the LED
                    } else if (morseChar == '-') {
                        M5.Beep.tone(1000); // Dash: 1000 Hz
                        digitalWrite(LED_PIN, HIGH); // Turn on the LED
                        delay(900);         // Long beep duration
                        digitalWrite(LED_PIN, LOW); // Turn off the LED
                    } else if (morseChar == ' ') {
                        delay(300);         // Pause between characters
                    }
                    M5.Beep.mute(); // Stop the tone
                    delay(100);     // Pause between elements
                }

                messageDisplayed = true; // Prevent repetitive display and beep
            }
        }
        vRingbufferReturnItem(rb, (void*)item);
    } else {
        messageDisplayed = false; // Ready to display and beep for the next message
    }
    
    rmt_rx_stop(RMT_RX_CHANNEL);
#endif

    // Check for Telnet connections if Wi-Fi hotspot is enabled
    if (wifiHotspotEnabled) {
        telnetClient = telnetServer.available();

        if (telnetClient) {
            // Handle incoming Telnet data
            while (telnetClient.connected()) {
                if (telnetClient.available()) {
                    String receivedText = telnetClient.readStringUntil('\n');
                    if (receivedText.length() > 0) {
                        // Send the received text message over RF433RX
                        sendTextMessage(receivedText.c_str(), receivedText.length());
                    }
                }
            }
            telnetClient.stop();
        }
    }

    delay(10);
    M5.update();
}
