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

                        client.println("<!DOCTYPE HTML>");
                        client.println("<html>");
                        client.println("<h1>Stopwatch</h1>");
                        client.println("<p><a href=\"/start\"><button>Start</button></a></p>");
                        client.println("<p><a href=\"/stop\"><button>Stop</button></a></p>");
                        client.println("<p><a href=\"/reset\"><button>Reset</button></a></p>");
                        client.print("<p>Tempo: ");
                        if (running)
                        {
                            client.print((elapsedTime + (millis() - startTime)) / 1000);
                        }
                        else
                        {
                            client.print(elapsedTime / 1000);
                        }
                        client.println(" secondi</p>");
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
