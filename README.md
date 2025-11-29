## Humidity Sensor Garage – ESP32 + GCP

This project uses an ESP32‑C3 and a DHT11 sensor to monitor humidity and temperature in a garage. Readings are sent securely to Google Cloud, stored in Firebase Realtime Database, and a Telegram alert is sent when humidity is too high.

### What this project does

- Measures temperature and humidity in a garage using an ESP32‑C3 and DHT11.
- Sends sensor readings securely over HTTPS to a Google Cloud Function.
- Stores the data in Firebase Realtime Database for history/visualization.
- Sends a Telegram message if humidity goes above a threshold.

### High‑level architecture

![arch](./assets/1.png)


- **ESP32‑C3 (device)**
  - Connects to Wi‑Fi.
  - Periodically reads the DHT11 sensor.
  - Builds a small JSON payload with `temperature`, `humidity`, and `device_id`.
  - Authenticates with Google Cloud and calls a secured HTTP endpoint.

- **Google Cloud backend**
  - An HTTPS Cloud Function receives the request.
  - Firebase Realtime Database stores the sensor readings.
  - Google Secret Manager stores sensitive values (Telegram bot token, chat ID).
  - A Telegram bot sends alerts when humidity is above a configured limit.

### How the ESP32 authenticates and talks to GCP

1. **Create a JWT on the device**  
	The ESP32 holds a Google Cloud service account email and private key (baked into the firmware from a JSON key file). When it needs to talk to Google, it:
	- Builds a JWT header: algorithm `RS256`, type `JWT`.
	- Builds a JWT payload with:
	  - `iss` and `sub`: the service account email.
	  - `aud`: `https://oauth2.googleapis.com/token` (Google OAuth token endpoint).
	  - `iat` / `exp`: issued‑at and expiry times (about 1 hour lifetime).
	  - `target_audience`: the Cloud Function URL it wants to call.
	- Signs this JWT with the service account private key using RSA + SHA‑256 (mbedTLS).

2. **Exchange the JWT for an ID token**  
	The ESP32 sends an HTTPS POST request to `https://oauth2.googleapis.com/token` with:
	- `grant_type=urn:ietf:params:oauth:grant-type:jwt-bearer`
	- `assertion=<signed_JWT>`

	Google verifies the signature and returns an **ID token**, which is a short‑lived token representing the service account.

3. **Cache the ID token**  
	The device remembers this ID token for about 50 minutes, so it doesn’t need to repeat the whole JWT exchange for every reading.

4. **Call the Cloud Function with the ID token**  
	When sending a sensor reading, the ESP32:
	- Opens an HTTPS connection to the Cloud Function URL.
	- Sends headers:
	  - `Content-Type: application/json`
	  - `Authorization: Bearer <ID_TOKEN>`
	- Sends the JSON body with temperature, humidity and device id.

### What happens in Google Cloud

1. The HTTPS Cloud Function is configured to require authentication, so only requests with a valid ID token are accepted.
2. Google automatically validates the ID token (issuer, audience, signature) and passes the request to the function code.
3. The Cloud Function:
	- Parses the incoming JSON.
	- Writes the reading into Firebase Realtime Database under a `sensor_readings` path, with a timestamp.
	- Checks the humidity value; if it is above the threshold, it prepares an alert.

4. To send the alert, the function:
	- Reads the Telegram bot token and chat ID from Google Secret Manager (never hard‑coded in the source).
	- Calls the Telegram Bot API `sendMessage` endpoint with a simple text message like "Humidity is too high in the garage".

### Summary

In short, the ESP32 reads humidity and temperature, authenticates to Google Cloud using a signed JWT and ID token, sends the data over HTTPS to a secured Cloud Function, and the backend stores it in Firebase and uses Secret Manager + Telegram to notify when the garage is getting too humid.


