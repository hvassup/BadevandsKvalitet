// api/copenhagen-beaches.js
export default async function handler(req, res) {
  // Set CORS headers for ESP32 requests
  res.setHeader("Access-Control-Allow-Origin", "*");
  res.setHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
  res.setHeader("Access-Control-Allow-Headers", "Content-Type");

  // Handle preflight requests
  if (req.method === "OPTIONS") {
    res.status(200).end();
    return;
  }

  // Only allow GET requests
  if (req.method !== "GET") {
    return res.status(405).json({
      error: "Method not allowed",
      message: "Only GET requests are supported",
    });
  }

  try {
    console.log("Fetching beach data from Danish API...");

    // Fetch data from the Danish beach API
    const response = await fetch("https://api.badevand.dk/api/beaches/dk", {
      headers: {
        "User-Agent": "Vercel-Middleware/1.0",
        Accept: "application/json",
      },
      // 30 second timeout
      signal: AbortSignal.timeout(30000),
    });

    if (!response.ok) {
      throw new Error(
        `Danish API returned ${response.status}: ${response.statusText}`
      );
    }

    const allBeaches = await response.json();
    console.log(`Fetched ${allBeaches.length} total beaches`);

    // Filter for Copenhagen beaches only
    const copenhagenBeaches = allBeaches.filter(
      (beach) => beach.municipality === "København"
    );

    console.log(`Found ${copenhagenBeaches.length} Copenhagen beaches`);

    // Clean up the data - only return fields we need
    const cleanedBeaches = copenhagenBeaches.map((beach) => ({
      name: beach.name || "Unknown",
      municipality: beach.municipality || "Unknown",
      region: beach.region || "Unknown",
      latitude: beach.latitude || 0,
      longitude: beach.longitude || 0,
      status: beach.status || "Unknown",
      lastUpdated: beach.lastUpdated || "Unknown",
    }));

    // Return successful response
    res.status(200).json({
      success: true,
      count: cleanedBeaches.length,
      data: cleanedBeaches,
      timestamp: new Date().toISOString(),
      source: "api.badevand.dk",
    });
  } catch (error) {
    console.error("Error fetching beach data:", error);

    // Return error response
    res.status(500).json({
      success: false,
      error: "Failed to fetch beach data",
      message: error.message,
      timestamp: new Date().toISOString(),
    });
  }
}
