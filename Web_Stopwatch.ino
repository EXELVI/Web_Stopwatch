#include <NTPClient.h>
#include "WiFiS3.h"
#include <WiFiUdp.h>
#include <Arduino_JSON.h>
#include "WiFiSSLClient.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "RTC.h"
#include <Arduino_JSON.h>

#include "arduino_secrets.h"

int timeZoneOffsetHours = 2;

const char *ssid = SECRET_SSID;
const char *pass = SECRET_PASS;

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

unsigned long startTime = 0;
unsigned long elapsedTime = 0;
unsigned long accumulatedTime = 0;
bool running = false;

unsigned long laps[10]; // max 10 laps
int lapIndex = 0;

int status = WL_IDLE_STATUS;
WiFiServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

#include "icons.h"

const int epd_bitmap_allArray_LEN = 4;
const unsigned char *epd_bitmap_allArray[4] = {
    // 32x23px
    epd_bitmap_hourglass_bottom,
    epd_bitmap_hourglass_split,
    epd_bitmap_hourglass_top,
    epd_bitmap_stopwatch};

const unsigned char *getIcon(const char *icon)
{
    if (strcmp(icon, "hourglass_bottom") == 0)
    {
        return epd_bitmap_allArray[0];
    }
    else if (strcmp(icon, "hourglass_split") == 0)
    {
        return epd_bitmap_allArray[1];
    }
    else if (strcmp(icon, "hourglass_top") == 0)
    {
        return epd_bitmap_allArray[2];
    }
    else if (strcmp(icon, "stopwatch") == 0)
    {
        return epd_bitmap_allArray[3];
    }
}

void repeatString(const char *str, int times, char *result)
{
    result[0] = '\0';

    for (int i = 0; i < times; ++i)
    {
        strcat(result, str);
    }
}

void setup()
{

    Serial.begin(115200);
    RTC.begin();
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print("Web Stopwatch");
    display.drawBitmap(0, 7, getIcon("stopwatch"), 32, 23, WHITE);
    display.display();

    delay(5000);
    if (WiFi.status() == WL_NO_MODULE)
    {
        Serial.println("Communication with WiFi module failed!");
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("Communication with WiFi module failed!");
        display.display();
        while (true)
            ;
    }

    String fv = WiFi.firmwareVersion();
    if (fv < WIFI_FIRMWARE_LATEST_VERSION)
    {
        Serial.println("Please upgrade the firmware");
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("Please upgrade the firmware");
        display.display();
    }

    int frame = 0;
    const char *dot = ".";
    while (status != WL_CONNECTED)
    {

        char result[100];
        repeatString(dot, frame + 1, result);

        Serial.print("Attempting to connect to SSID: ");
        Serial.println(ssid);
        display.clearDisplay();
        display.setCursor(35, 10);
        display.print("Connecting" + String(result));
        display.setCursor(35, 20);
        display.print(ssid);
        if (frame == 0)
        {
            display.drawBitmap(0, 10, getIcon("hourglass_top"), 32, 23, WHITE);
        }
        else if (frame == 1)
        {
            display.drawBitmap(0, 10, getIcon("hourglass_split"), 32, 23, WHITE);
        }
        else if (frame == 2)
        {
            display.drawBitmap(0, 10, getIcon("hourglass_bottom"), 32, 23, WHITE);
        }
        display.display();
        frame++;
        if (frame > 2)
        {
            frame = 0;
        }
        status = WiFi.begin(ssid, pass);
    }

    printWifiStatus();
    delay(5000);
    timeClient.begin();

    timeClient.update();

    auto unixTime = timeClient.getEpochTime();
    Serial.print("Unix time = ");
    Serial.println(unixTime);
    RTCTime timeToSet = RTCTime(unixTime);
    RTC.setTime(timeToSet);

    RTCTime currentTime;
    RTC.getTime(currentTime);
    Serial.println("The RTC was just set to: " + String(currentTime));

    server.begin();
}

void loop()
{
    WiFiClient client = server.available();

    if (client)
    {
        Serial.println("New client");
        String currentLine = "";
        while (client.connected())
        {
            if (client.available())
            {
                char c = client.read();
                Serial.write(c);
                if (c == '\n')
                {
                    if (currentLine.length() == 0)
                    {
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println();
                        client.println("<!DOCTYPE html>");
                        client.println("<html lang=\"en\" data-bs-theme=\"dark\">");
                        client.println("");
                        client.println("<head>");
                        client.println("    <meta charset=\"UTF-8\">");
                        client.println("    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">");
                        client.println("    <title>Stop Watch</title>");
                        client.println("    <link href=\"https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/css/bootstrap.min.css\" rel=\"stylesheet\" integrity=\"sha384-QWTKZyjpPEjISv5WaRU9OFeRpok6YctnYmDr5pNlyT2bRjXh0JMhjY6hW+ALEwIH\" crossorigin=\"anonymous\">");
                        client.println("</head>");
                        client.println("");
                        client.println("<body>");
                        client.println("    <div class=\"container mt-5\">");
                        client.println("        <div class=\"row\">");
                        client.println("            <div class=\"col text-center\">");
                        client.println("                <h1 id=\"timer\">00:00:00,000</h1>");
                        client.println("                <button id=\"startButton\" class=\"btn btn-success\">Start</button>");
                        client.println("                <button id=\"stopButton\" class=\"btn btn-danger\">Stop</button>");
                        client.println("                <button id=\"resetButton\" class=\"btn btn-warning\">Reset</button>");
                        client.println("            </div>");
                        client.println("        </div>");
                        client.println("    </div>");
                        client.println("");
                        client.println("    <script>");
                        client.println("        let intervalId;");
                        client.println("        let startTime;");
                        client.println("        let localElapsedTime = 0;");
                        client.println("        let isRunning = false;");
                        client.println("");
                        client.println("        function updateTimerDisplay(elapsedTime) {");
                        client.println("            const seconds = Math.floor(elapsedTime / 1000) % 60, minutes = Math.floor(elapsedTime / (1000 * 60)) % 60, hours = Math.floor(elapsedTime / (1000 * 60 * 60)) % 24, ms = elapsedTime % 1000; const display = `${String(hours).padStart(2, '0')}:${String(minutes).padStart(2, '0')}:${String(seconds).padStart(2, '0')},${String(ms).padStart(3, '0')}`;");
                        client.println("            document.getElementById('timer').textContent = display;");
                        client.println("        }");
                        client.println("");
                        client.println("        async function fetchData() {");
                        client.println("            const res = await fetch('/data'), data = await res.json();");
                        client.println("            isRunning = data.running; localElapsedTime = data.elapsedTime;");
                        client.println("            if (isRunning) { startTime = Date.now() - localElapsedTime; updateLocalTimer(); } else { updateTimerDisplay(localElapsedTime); clearInterval(intervalId); }");
                        client.println("        }");
                        client.println("");
                        client.println("        function updateLocalTimer() {");
                        client.println("            if (isRunning) { intervalId = setInterval(() => { localElapsedTime = Date.now() - startTime; updateTimerDisplay(localElapsedTime); }, 10); }");
                        client.println("        }");
                        client.println("        document.getElementById('startButton').addEventListener('click', async () => { await fetch('/start', { method: 'GET' }); fetchData(); });");
                        client.println("        document.getElementById('stopButton').addEventListener('click', async () => { await fetch('/stop', { method: 'GET' }); isRunning = false; fetchData(); });");
                        client.println("        document.getElementById('resetButton').addEventListener('click', async () => { await fetch('/reset', { method: 'GET' }); localElapsedTime = 0; updateTimerDisplay(localElapsedTime); clearInterval(intervalId); });");
                        client.println("");
                        client.println("        fetchData();");
                        client.println("    </script>");
                        client.println("    <script src=\"https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/js/bootstrap.bundle.min.js\" integrity=\"sha384-YvpcrYf0tY3lHB60NNkmXc5s9fDVZLESaAA55NDzOxhy9GkcIdslK1eN7N6jIeHz\" crossorigin=\"anonymous\"></script>");
                        client.println("</body>");
                        client.println("");
                        client.println("</html>");
                        break;
                    }
                    else
                    {
                        if (currentLine.startsWith("GET /start"))
                        {
                            if (!running)
                            {
                                startTime = millis();
                                running = true;
                            }
                        }
                        else if (currentLine.startsWith("GET /stop"))
                        {
                            if (running)
                            {
                                elapsedTime += millis() - startTime;
                                running = false;
                            }
                        }
                        else if (currentLine.startsWith("GET /reset"))
                        {
                            running = false;
                            elapsedTime = 0;
                        }
                        else if (currentLine.startsWith("GET /data"))
                        {
                            client.println("HTTP/1.1 200 OK");
                            client.println("Content-type:application/json");
                            client.println();

                            JSONVar myObject;
                            JSONVar lapsArray;
                            for (int i = 0; i < 10; i++)
                            {
                                lapsArray[i] = laps[i];
                            }
                            myObject["laps"] = lapsArray;
                            myObject["running"] = running;
                            myObject["elapsedTime"] = elapsedTime + (running ? accumulatedTime : 0);

                            String jsonString = JSON.stringify(myObject);
                            client.println(jsonString);

                            break;
                        }
                        else if (currentLine.startsWith("GET /lap"))
                        {
                            if (running)
                            {
                                int lapsTotalTime;

                                for (int i = 0; i < 10; i++)
                                {
                                    lapsTotalTime += laps[i];
                                }

                                laps[lapIndex] = millis() - startTime - lapsTotalTime;

                                lapIndex++;
                                if (lapIndex >= 10)
                                {
                                    lapIndex = 0;
                                }
                            }
                        }
                        currentLine = "";
                    }
                }
                else if (c != '\r')
                {
                    currentLine += c;
                }
            }
        }
        client.stop();
        Serial.println("Client disconnected");
    }

    if (running)
    {
        accumulatedTime = millis() - startTime;
    }

    RTCTime currentTime;

    RTC.getTime(currentTime);
    display.clearDisplay();
    printWifiBar();
    display.setCursor(0, 0);

    int adjustedHour = (currentTime.getHour() + timeZoneOffsetHours) % 24;
    String hours = String(adjustedHour);
    String minutes = String(currentTime.getMinutes());
    String seconds = String(currentTime.getSeconds());

    display.print(hours.length() == 1 ? "0" + hours : hours);
    display.print(":");
    display.print(minutes.length() == 1 ? "0" + minutes : minutes);
    display.print(":");
    display.print(seconds.length() == 1 ? "0" + seconds : seconds);
    display.setCursor(0, 10);
    display.setTextSize(2);

    int millisecondsT = (elapsedTime + (running ? accumulatedTime : 0)) % 1000;
    int secondsT = (elapsedTime + (running ? accumulatedTime : 0)) / 1000 % 60;
    int minutesT = (elapsedTime + (running ? accumulatedTime : 0)) / (1000 * 60) % 60;
    int hoursT = (elapsedTime + (running ? accumulatedTime : 0)) / (1000 * 60 * 60) % 24;

    int lastLap = laps[lapIndex - 1];

    display.print(String(hoursT < 10 ? "0" + String(hoursT) : String(hoursT)) + ":" +
                  String(minutesT < 10 ? "0" + String(minutesT) : String(minutesT)) + ":" +
                  String(secondsT < 10 ? "0" + String(secondsT) : String(secondsT)));
    display.setTextSize(1);
    display.print(", " + String(millisecondsT < 10 ? "00" + String(millisecondsT) : millisecondsT < 100 ? "0" + String(millisecondsT) : String(millisecondsT)));
    
    display.display();
}

void printWifiBar()
{
    long rssi = WiFi.RSSI();

    if (rssi > -55)
    {
        display.fillRect(102, 7, 4, 1, WHITE);
        display.fillRect(107, 6, 4, 2, WHITE);
        display.fillRect(112, 4, 4, 4, WHITE);
        display.fillRect(117, 2, 4, 6, WHITE);
        display.fillRect(122, 0, 4, 8, WHITE);
    }
    else if (rssi<-55 & rssi> - 65)
    {
        display.fillRect(102, 7, 4, 1, WHITE);
        display.fillRect(107, 6, 4, 2, WHITE);
        display.fillRect(112, 4, 4, 4, WHITE);
        display.fillRect(117, 2, 4, 6, WHITE);
        display.drawRect(122, 0, 4, 8, WHITE);
    }
    else if (rssi<-65 & rssi> - 75)
    {
        display.fillRect(102, 7, 4, 1, WHITE);
        display.fillRect(107, 6, 4, 2, WHITE);
        display.fillRect(112, 4, 4, 4, WHITE);
        display.drawRect(117, 2, 4, 6, WHITE);
        display.drawRect(122, 0, 4, 8, WHITE);
    }
    else if (rssi<-75 & rssi> - 85)
    {
        display.fillRect(102, 7, 4, 1, WHITE);
        display.fillRect(107, 6, 4, 2, WHITE);
        display.drawRect(112, 4, 4, 4, WHITE);
        display.drawRect(117, 2, 4, 6, WHITE);
        display.drawRect(122, 0, 4, 8, WHITE);
    }
    else if (rssi<-85 & rssi> - 96)
    {
        display.fillRect(102, 7, 4, 1, WHITE);
        display.drawRect(107, 6, 4, 2, WHITE);
        display.drawRect(112, 4, 4, 4, WHITE);
        display.drawRect(117, 2, 4, 6, WHITE);
        display.drawRect(122, 0, 4, 8, WHITE);
    }
    else
    {
        display.drawRect(102, 7, 4, 1, WHITE);
        display.drawRect(107, 6, 4, 2, WHITE);
        display.drawRect(112, 4, 4, 4, WHITE);
        display.drawRect(117, 2, 4, 6, WHITE);
        display.drawRect(122, 0, 4, 8, WHITE);
    }
}

void printWifiStatus()
{
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print(String(WiFi.SSID()));

    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    display.setCursor(0, 20);
    display.println(ip);

    long rssi = WiFi.RSSI();

    printWifiBar();

    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");

    display.setCursor(0, 10);
    display.print(String(rssi) + " dBm");
    display.display();
}
