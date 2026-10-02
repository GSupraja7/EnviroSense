# EnviroSense — IoT Environment Monitoring System

A real-time temperature & humidity monitoring system: an ESP32 microcontroller
reads sensor data and streams it to the cloud, where a live web dashboard
displays the readings and flags threshold alerts.

## Architecture

```
┌─────────────┐      Wi-Fi       ┌──────────────────┐      HTTPS       ┌──────────────────┐
│   DHT22      │ ───────────────▶│      ESP32        │────────────────▶│  Firebase         │
│  Sensor      │   analog/digital │  (reads + logic)  │   REST API PUT   │  Realtime DB      │
└─────────────┘                  └──────────────────┘                  └────────┬─────────┘
                                                                                  │
                                                                                  │ GET (polling)
                                                                                  ▼
                                                                        ┌──────────────────┐
                                                                        │  Web Dashboard    │
                                                                        │  (HTML/JS/Chart.js)│
                                                                        └──────────────────┘
```

**Flow:** Sensor → Microcontroller reads + validates data → sends to cloud DB over HTTP →
dashboard polls the DB and renders live charts + alerts.

## Tech Stack

| Layer | Choice | Why |
|---|---|---|
| Sensor | DHT22 | Reliable, cheap, gives both temp + humidity in one module |
| Microcontroller | ESP32 | Built-in Wi-Fi, more RAM than ESP8266, active community |
| Protocol | HTTP (REST) | Simplest to implement and debug for a first IoT project; MQTT is the natural upgrade for scale |
| Backend | Firebase Realtime DB | Free tier, zero server management, REST API works directly from Arduino code |
| Dashboard | HTML/CSS/JS + Chart.js | No framework overhead needed; shows you can build UI without relying on a template |

## Files in this project

- `firmware/esp32_envirosense.ino` — Arduino code that runs on the ESP32: reads the sensor, validates data, checks alert thresholds, and pushes to Firebase.
- `dashboard/index.html` — Single-file web dashboard. Ships in **demo mode** (simulates sensor data client-side) so it's fully demoable without hardware — swap one function to point it at your real Firebase database.

## How to actually build this (if you have hardware)

1. **Wire it up:** DHT22 data pin → GPIO 4 on the ESP32, with a 10k pull-up resistor between data and VCC.
2. **Firebase setup:** Create a free Firebase project → enable Realtime Database → copy your database URL into the firmware's `FIREBASE_HOST` constant.
3. **Flash the ESP32:** Open `esp32_envirosense.ino` in Arduino IDE, install the `DHT sensor library` and `Adafruit Unified Sensor` libraries, fill in your Wi-Fi credentials, upload.
4. **Go live on the dashboard:** In `index.html`, replace the `fetchLatestReading()` function with the real Firebase GET call (the exact code is commented directly above it in the file).
5. **Push to GitHub** with this README — that's your project link for your resume.

## If you don't have hardware before the interview

You can **still demo this fully** — the dashboard runs standalone with simulated data out of the box. Open `dashboard/index.html` in any browser and it "just works," showing live-updating charts and alerts exactly as it would with a real sensor. This is a completely legitimate thing to show and explain honestly in an interview: *"I built and tested the full pipeline in simulation/demo mode; the same code path connects directly to a real ESP32 + Firebase deployment."*

---

## Resume bullet point (copy/adapt this)

> **EnviroSense — IoT Environment Monitoring System** *(Personal Project)*
> Built an end-to-end IoT pipeline using an ESP32 microcontroller and DHT22 sensor to collect real-time temperature/humidity data, transmit it via HTTP to a Firebase Realtime Database, and visualize it on a custom web dashboard with live charting and threshold-based alerting.

---

## Interview Prep: Likely Questions & How to Answer Them

**Q: Walk me through the architecture.**
Sensor → microcontroller reads and validates data → pushed over HTTP to Firebase → dashboard polls Firebase and renders it. Mention it's a simple 3-tier pattern: edge device, cloud storage, presentation layer.

**Q: Why HTTP instead of MQTT?**
HTTP is simpler to implement and debug for a first project and works fine at low frequency (sending every 10s). MQTT is a pub/sub protocol, lighter-weight over the wire, and the better choice if you scale to many devices or need near-instant push updates — that's the natural "next step" you'd mention.

**Q: What happens if the Wi-Fi drops?**
The firmware detects `WiFi.status() != WL_CONNECTED` in the main loop and automatically retries the connection — this is a real resilience concern with physical IoT devices that's worth calling out.

**Q: What if the sensor gives bad data?**
DHT sensors occasionally return `NaN`. The firmware checks `isnan()` before using or sending a reading, and just skips that cycle rather than sending garbage data — a small detail that shows you understand real hardware isn't always reliable.

**Q: How would you scale this to 100 devices?**
Move from HTTP polling to MQTT with a broker (e.g., Mosquitto/AWS IoT Core), give each device a unique ID, and switch the dashboard to a time-series database like InfluxDB instead of a simple key-value store, since Firebase Realtime DB doesn't scale well for high-frequency multi-device writes.

**Q: What would you improve with more time?**
- Security: currently no auth on the Firebase writes — would add Firebase Auth tokens or move to a proper backend with API keys
- OTA (over-the-air) firmware updates instead of physical re-flashing
- Store real epoch timestamps via NTP instead of `millis()` (which just counts since boot)
- Add a second sensor (e.g., air quality) to make the alerting more meaningful

**Q: What was the hardest part?**
Good honest answers: handling intermittent Wi-Fi drops, dealing with occasional bad sensor reads, or getting the Firebase REST payload format right on the first try. Interviewers like hearing about a real, specific debugging moment more than a polished "everything worked perfectly" story.
