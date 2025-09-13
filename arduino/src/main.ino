#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

// WiFi credentials
const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";

// Your Vercel API endpoint
const char *apiUrl = "https://your-project-name.vercel.app/api/copenhagen-beaches";

// Structure to hold beach data
struct Beach
{
  String name;
  String municipality;
  String region;
  double latitude;
  double longitude;
  String status;
  String lastUpdated;
};

// Array to store beaches
Beach beaches[20];
int beachCount = 0;

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println("=== Copenhagen Beach Monitor ===");
  Serial.println("Using Vercel middleware API");

  // Connect to WiFi
  connectToWiFi();

  // Fetch beach data from your API
  fetchBeachData();

  // Display results
  displayBeaches();
}

void loop()
{
  // Update every 10 minutes (API has caching)
  delay(10 * 60 * 1000);

  Serial.println("\n--- Refreshing beach data ---");
  fetchBeachData();
  displayBeaches();
}

void connectToWiFi()
{
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20)
  {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\nWiFi connected!");
    Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Signal strength: %d dBm\n", WiFi.RSSI());
  }
  else
  {
    Serial.println("\nFailed to connect to WiFi!");
  }
}

void fetchBeachData()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi not connected. Reconnecting...");
    connectToWiFi();
    if (WiFi.status() != WL_CONNECTED)
    {
      return;
    }
  }

  WiFiClientSecure client;
  client.setInsecure(); // Skip certificate validation for simplicity

  HTTPClient http;

  Serial.println("Fetching data from Vercel API...");

  if (!http.begin(client, apiUrl))
  {
    Serial.println("Failed to initialize HTTP client");
    return;
  }

  // Set headers
  http.addHeader("User-Agent", "ESP32-BeachMonitor/1.0");
  http.addHeader("Accept", "application/json");
  http.setTimeout(15000); // 15 second timeout

  // Make the request
  int httpResponseCode = http.GET();

  Serial.printf("HTTP Response Code: %d\n", httpResponseCode);

  if (httpResponseCode == 200)
  {
    String payload = http.getString();
    Serial.printf("Received %d bytes of data\n", payload.length());

    // Parse the JSON response
    parseAPIResponse(payload);
  }
  else
  {
    Serial.printf("HTTP request failed with code: %d\n", httpResponseCode);
    String errorResponse = http.getString();
    Serial.printf("Error response: %s\n", errorResponse.c_str());
  }

  http.end();
  client.stop();
}

void parseAPIResponse(const String &jsonResponse)
{
  Serial.println("Parsing API response...");

  // The response is much smaller now, so we can use a reasonable document size
  DynamicJsonDocument doc(8192);

  DeserializationError error = deserializeJson(doc, jsonResponse);

  if (error)
  {
    Serial.printf("JSON parsing failed: %s\n", error.c_str());
    return;
  }

  // Check if the API call was successful
  bool success = doc["success"] | false;

  if (!success)
  {
    Serial.println("API returned error:");
    Serial.println(doc["message"] | "Unknown error");
    return;
  }

  // Get the beaches array
  JsonArray beachesArray = doc["data"];
  beachCount = min((int)beachesArray.size(), 20);

  Serial.printf("API returned %d Copenhagen beaches\n", beachCount);

  // Parse each beach
  for (int i = 0; i < beachCount; i++)
  {
    JsonObject beach = beachesArray[i];

    beaches[i].name = beach["name"] | "Unknown";
    beaches[i].municipality = beach["municipality"] | "Unknown";
    beaches[i].region = beach["region"] | "Unknown";
    beaches[i].latitude = beach["latitude"] | 0.0;
    beaches[i].longitude = beach["longitude"] | 0.0;
    beaches[i].status = beach["status"] | "Unknown";
    beaches[i].lastUpdated = beach["lastUpdated"] | "Unknown";
  }

  // Show metadata
  String timestamp = doc["timestamp"] | "Unknown";
  String source = doc["source"] | "Unknown";

  Serial.printf("Data timestamp: %s\n", timestamp.c_str());
  Serial.printf("Data source: %s\n", source.c_str());
  Serial.printf("Free heap after parsing: %d bytes\n", ESP.getFreeHeap());
}

void displayBeaches()
{
  Serial.println("\n=== COPENHAGEN BEACHES ===");

  if (beachCount == 0)
  {
    Serial.println("No beach data available");
    return;
  }

  for (int i = 0; i < beachCount; i++)
  {
    Serial.printf("\n--- Beach %d ---\n", i + 1);
    Serial.printf("Name: %s\n", beaches[i].name.c_str());
    Serial.printf("Municipality: %s\n", beaches[i].municipality.c_str());
    Serial.printf("Region: %s\n", beaches[i].region.c_str());
    Serial.printf("Location: %.6f, %.6f\n", beaches[i].latitude, beaches[i].longitude);
    Serial.printf("Status: %s\n", beaches[i].status.c_str());
    Serial.printf("Last Updated: %s\n", beaches[i].lastUpdated.c_str());
  }

  Serial.println("\n==========================");
  Serial.printf("Total: %d beaches\n", beachCount);
}

// Utility function to check memory usage
void printMemoryUsage()
{
  Serial.printf("Free heap: %d bytes (%.1f%%)\n",
                ESP.getFreeHeap(),
                (float)ESP.getFreeHeap() / ESP.getHeapSize() * 100);
}