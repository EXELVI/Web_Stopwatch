# 🚪 Web_Stopwatch

![GitHub last commit](https://img.shields.io/github/last-commit/EXELVI/Web_Stopwatch)
![GitHub issues](https://img.shields.io/github/issues/EXELVI/Web_Stopwatch)
![GitHub stars](https://img.shields.io/github/stars/EXELVI/Web_Stopwatch?style=social)

## 📜 Description

**Web_Stopwatch** is an advanced Arduino-based project that functions as a Web-connected stopwatch. It is designed to provide precise lap timings, while also being integrated with an OLED display to show real-time data, including network connection status and current time. This project utilizes various libraries to achieve its functionality, including `WiFiS3`, `NTPClient`, and `Adafruit_SSD1306`.

## 🚀 Features

- **Start/Stop Functionality**: Start and stop the stopwatch with a button press.
- **Lap Recording**: Record lap times during a run with a dedicated button.
- **OLED Display**: Displays current time, stopwatch status, and lap times.
- **WiFi Connectivity**: Connects to a WiFi network to fetch the accurate time via NTP.
- **Web Interface**: Provides a simple web-based interface to control the stopwatch and view lap times.

## 🛠️ Setup

### Prerequisites

- **Arduino IDE**: Ensure you have the latest version installed.
- **Libraries**: The following libraries must be installed in your Arduino IDE:
  - `WiFiS3`
  - `NTPClient`
  - `Adafruit_GFX`
  - `Adafruit_SSD1306`
  - `Arduino_JSON`
  - `WiFiSSLClient`
- **Hardware**:
  - Arduino UNO R4 WIFI or similar board.
  - OLED Display (SSD1306).
  - Two push buttons (with lights).
  - Breadboard and jumper wires.

### Installation

1. **Clone the Repository:**

    ```bash
    git clone https://github.com/EXELVI/Web_Stopwatch.git
    cd Web_Stopwatch
    ```

2. **Load the Sketch:**

    Open the `Web_Stopwatch.ino` file in your Arduino IDE.

3. **Configure WiFi:**

    Update your WiFi credentials in the `arduino_secrets.h` file:

    ```cpp
    #define SECRET_SSID "Your_SSID"
    #define SECRET_PASS "Your_PASSWORD"
    ```

4. **Upload to Your Board:**

    Select the correct board and port from the Arduino IDE, then upload the code.

### Usage

- **Button 1**:
  - Press while running: Records a lap time.
  - Press while stopped: Resets the stopwatch.
  
- **Button 2**:
  - Press while running: Stops the stopwatch.
  - Press while stopped: Starts the stopwatch.
  - The Button 2 light will remain on when the stopwatch is running.

- **Web Interface**:
  - Access the web interface through the IP address displayed on the OLED screen after connecting to WiFi.

## 📚 Libraries Used

- **WiFiS3**: Handles WiFi connection and communication.
- **NTPClient**: Synchronizes time with an NTP server.
- **Adafruit GFX & SSD1306**: Manages the OLED display.
- **Arduino_JSON**: Manages JSON data for web communication.
- **WiFiSSLClient**: Provides SSL connectivity.

## 🤝 Contributions

Contributions, issues, and feature requests are welcome! Feel free to check the [issues page](https://github.com/EXELVI/Web_Stopwatch/issues).
