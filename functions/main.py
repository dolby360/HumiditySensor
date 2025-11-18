# Welcome to Cloud Functions for Firebase for Python!
# To get started, simply uncomment the below code or create your own.
# Deploy with `firebase deploy`

from firebase_functions import https_fn
from firebase_functions.options import set_global_options
from firebase_admin import initialize_app, db
import json
from datetime import datetime

# For cost control, you can set the maximum number of containers that can be
# running at the same time. This helps mitigate the impact of unexpected
# traffic spikes by instead downgrading performance. This limit is a per-function
# limit. You can override the limit for each function using the max_instances
# parameter in the decorator, e.g. @https_fn.on_request(max_instances=5).
set_global_options(max_instances=10)

initialize_app(options={
    'databaseURL': 'https://garagehumedity.firebaseio.com'
})


@https_fn.on_request()
def on_request_example(req: https_fn.Request) -> https_fn.Response:
    return https_fn.Response("Hello world!")


@https_fn.on_request()
def receive_sensor_data(req: https_fn.Request) -> https_fn.Response:
    """
    Receives humidity and temperature data from ESP32 sensor.
    Expected JSON format:
    {
        "temperature": 25.5,
        "humidity": 60.3,
        "device_id": "esp32_garage"
    }
    """
    try:
        # Only accept POST requests
        if req.method != "POST":
            return https_fn.Response(
                json.dumps({"error": "Only POST method is allowed"}),
                status=405,
                headers={"Content-Type": "application/json"}
            )
        
        # Parse JSON data from request
        data = req.get_json()
        
        if not data:
            return https_fn.Response(
                json.dumps({"error": "No data provided"}),
                status=400,
                headers={"Content-Type": "application/json"}
            )
        
        # Validate required fields
        required_fields = ["temperature", "humidity"]
        missing_fields = [field for field in required_fields if field not in data]
        
        if missing_fields:
            return https_fn.Response(
                json.dumps({"error": f"Missing required fields: {', '.join(missing_fields)}"}),
                status=400,
                headers={"Content-Type": "application/json"}
            )
        
        # Extract data
        temperature = data.get("temperature")
        humidity = data.get("humidity")
        device_id = data.get("device_id", "unknown")
        
        # Validate data types and ranges
        try:
            temperature = float(temperature)
            humidity = float(humidity)
            
            if not (-50 <= temperature <= 100):
                return https_fn.Response(
                    json.dumps({"error": "Temperature out of valid range (-50 to 100°C)"}),
                    status=400,
                    headers={"Content-Type": "application/json"}
                )
            
            if not (0 <= humidity <= 100):
                return https_fn.Response(
                    json.dumps({"error": "Humidity out of valid range (0 to 100%)"}),
                    status=400,
                    headers={"Content-Type": "application/json"}
                )
        except (ValueError, TypeError):
            return https_fn.Response(
                json.dumps({"error": "Temperature and humidity must be numeric values"}),
                status=400,
                headers={"Content-Type": "application/json"}
            )
        
        # Store data in Firebase Realtime Database
        ref = db.reference('sensor_readings')
        
        # Prepare data to save
        sensor_reading = {
            "temperature": temperature,
            "humidity": humidity,
            "device_id": device_id,
            "timestamp": datetime.utcnow().isoformat()
        }
        
        # Push new reading to database
        new_reading_ref = ref.push(sensor_reading)
        
        response_data = {
            "status": "success",
            "message": "Sensor data saved successfully",
            "data": {
                "temperature": temperature,
                "humidity": humidity,
                "device_id": device_id,
                "reading_id": new_reading_ref.key
            }
        }
        
        return https_fn.Response(
            json.dumps(response_data),
            status=200,
            headers={"Content-Type": "application/json"}
        )
        
    except Exception as e:
        return https_fn.Response(
            json.dumps({"error": f"Internal server error: {str(e)}"}),
            status=500,
            headers={"Content-Type": "application/json"}
        )