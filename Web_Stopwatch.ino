#include "WiFiS3.h"
#include <WiFiUdp.h>
#include <Arduino_JSON.h>
#include "WiFiSSLClient.h"
#include "RTC.h"
#include <Wire.h>
#include <NTPClient.h>
#include <Arduino_JSON.h>

#include "arduino_secrets.h"

const char *ssid = SECRET_SSID;
const char *password = SECRET_PASS;

WiFiUDP Udp;
NTPClient timeClient(Udp);

unsigned long startTime = 0;
unsigned long elapsedTime = 0;
unsigned long accumulatedTime = 0;
bool running = false;

WiFiServer server(80);

void setup()
{
    Serial.begin(115200);

    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

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
                        client.println("    <title>Stopwatch</title>");
                        client.println("    <link href=\"https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/css/bootstrap.min.css\" rel=\"stylesheet\"");
                        client.println("        integrity=\"sha384-QWTKZyjpPEjISv5WaRU9OFeRpok6YctnYmDr5pNlyT2bRjXh0JMhjY6hW+ALEwIH\" crossorigin=\"anonymous\">");
                        client.println("</head>");
                        client.println("");
                        client.println("<body>");
                        client.println("    <div class=\"container mt-5\">");
                        client.println("        <div class=\"row\">");
                        client.println("            <div class=\"col text-center\">");
                        client.println("                <h1 id=\"timer\">00:00:00</h1>");
                        client.println("                <button id=\"startButton\" class=\"btn btn-success\">Start</button>");
                        client.println("                <button id=\"stopButton\" class=\"btn btn-danger\">Stop</button>");
                        client.println("                <button id=\"resetButton\" class=\"btn btn-warning\">Reset</button>");
                        client.println("            </div>");
                        client.println("        </div>");
                        client.println("    </div>");
                        client.println("    <script>");
                        client.println("        document.addEventListener(\"DOMContentLoaded\", function () {");
                        client.println("            document.getElementById('startButton').addEventListener('click', async () => { await fetch('/start'}); });");
                        client.println("            document.getElementById('stopButton').addEventListener('click', async () => { await fetch('/stop'}); });");
                        client.println("            document.getElementById('resetButton').addEventListener('click', async () => { await fetch('/reset'); });");
                        client.println("        });");
                        client.println("    </script>");
                        client.println("    <script src=\"https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/js/bootstrap.bundle.min.js\"");
                        client.println("        integrity=\"sha384-YvpcrYf0tY3lHB60NNkmXc5s9fDVZLESaAA55NDzOxhy9GkcIdslK1eN7N6jIeHz\"");
                        client.println("        crossorigin=\"anonymous\"></script>");
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
                            myObject["running"] = running;
                            myObject["elapsedTime"] = elapsedTime + (running ? accumulatedTime : 0);

                            String jsonString = JSON.stringify(myObject);
                            client.println(jsonString);

                            break;
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
}
