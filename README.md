# Multi-Effect Guitar Pedal (ESP32)

A real-time **multi-effect guitar pedal system** built on ESP32,
featuring a modular 4-effect DSP chain, OLED interface, SD card storage,
WiFi control, and performance tools like tap tempo and auto-tune.

------------------------------------------------------------------------

## Features

-   **4-Effect Chain Processing**
    -   Run up to 4 effects simultaneously
-   **Multi-Effect Routing System**
    -   Flexible stacking and ordering of effects
-   **BPM Tap Tempo**
    -   Controlled via STOMP footswitch
-   **Auto-Tune Mode**
    -   Real-time pitch correction / tuning mode
-   **OLED Display (0.96" SSD1306)**
    -   Full menu system and live status display
-   **Expression Pedal Support**
    -   Analog control via GPIO34 (ADC)
-   **STOMP Footswitch**
    -   Fast bypass + tap tempo (GPIO33)
-   **SD Card System**
    -   Save/load presets\
    -   Looper support\
    -   Session logging
-   **WiFi Access Point**
    -   Built-in web server\
    -   HTTP API for external control

------------------------------------------------------------------------

## Hardware

### Core Board

-   ESP32 (CH9102)

### Audio Input

-   INMP441 I2S Microphone
    -   SCK → GPIO 14\
    -   WS → GPIO 15\
    -   SD → GPIO 32

### Audio Output

-   DAC Output → GPIO 25 → 10µF capacitor → 3.5mm jack

### Display

-   OLED 0.96" SSD1306 (I2C)
    -   SDA → GPIO 21\
    -   SCL → GPIO 22

### SD Card (SPI)

-   CS → GPIO 5\
-   SCK → GPIO 18\
-   MISO → GPIO 19\
-   MOSI → GPIO 23

### Controls

-   STOMP switch → GPIO 33 (to GND)\
-   Expression pedal → GPIO 34 (ADC input)

------------------------------------------------------------------------

## Libraries Required

-   ArduinoJson (Benoit Blanchon)
-   Adafruit SSD1306
-   Adafruit GFX
-   SD (built-in ESP32 core)

------------------------------------------------------------------------

## System Architecture

Audio Flow:

    INMP441 → Audio Engine → 4-Effect Chain → DAC Output

Control Flow:

    STOMP → Tap Tempo / Bypass  
    Expression Pedal → Real-time parameter control  
    OLED → UI navigation & status  
    SD Card → Presets / logs / loops  
    WiFi → HTTP API control  

------------------------------------------------------------------------

## WiFi / HTTP API

    GET  /status  
    POST /effect/set  
    POST /preset/load  
    POST /bpm/set  
    POST /bypass  

------------------------------------------------------------------------

## Project Structure

    src/        → Core firmware (audio, effects, control)  
    effects/    → DSP effect modules  
    ui/         → OLED menu system  
    storage/    → SD card system (presets/logs)  
    network/    → WiFi + HTTP API  
    include/    → Config + pin mapping  
    docs/       → Full documentation  
    web/        → Control web interface  
    schematics/ → Wiring diagrams  
    tools/      → Utility scripts  

------------------------------------------------------------------------

## License

kiraii.

------------------------------------------------------------------------
