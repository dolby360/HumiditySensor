from firebase_functions import https_fn
from firebase_functions.options import set_global_options
from firebase_admin import initialize_app, db
import json
from datetime import datetime
from pydantic import BaseModel, Field, ValidationError
import requests
from google.cloud import secretmanager

set_global_options(max_instances=1)

def get_secret(secret_id: str) -> str | None:
    """Retrieve a secret value from Google Secret Manager by its ID."""
    try:
        client = secretmanager.SecretManagerServiceClient()
        project_id = "garagehumedity"  # Your Firebase project ID
        version_id = "latest"

        name = f"projects/{project_id}/secrets/{secret_id}/versions/{version_id}"
        response = client.access_secret_version(request={"name": name})

        return response.payload.data.decode("UTF-8").strip()
    except Exception as e:
        print(f"Error retrieving secret '{secret_id}' from Secret Manager: {e}")
        return None

database_url = get_secret("database_url")

if not database_url:
    # Fallback or raise error; here we log and let initialize_app fail loudly if misconfigured
    print("Database URL not available from Secret Manager")

initialize_app(options={
    "databaseURL": database_url,
})

def send_telegram_alert(humidity: float, temperature: float, device_id: str):
    """Send telegram alert when humidity exceeds threshold"""
    token = get_secret("telegram-bot-token")
    chat_id = get_secret("telegram-chat-id")
    
    if not token:
        print("Telegram token not available")
        return False
    
    if not chat_id:
        print("Telegram chat_id not available")
        return False
    
    message = (
        f"High Humidity Alert!\n\n"
        f"Device: {device_id}\n"
        f"Humidity: {humidity}%\n"
        f"Temperature: {temperature}°C\n"
        f"Threshold exceeded: 65%"
    )
    
    url = f"https://api.telegram.org/bot{token}/sendMessage"
    payload = {
        "chat_id": chat_id,
        "text": message,
        # remove parse_mode for now to avoid Markdown issues
        # "parse_mode": "Markdown"
    }
    
    try:
        print(f"[telegram] sending payload: {payload}")
        response = requests.post(url, json=payload, timeout=10)
        print(f"[telegram] status: {response.status_code}")
        print(f"[telegram] raw response: {response.text}")
        response.raise_for_status()
        print(f"Telegram alert sent successfully for humidity {humidity}%")
        return True
    except requests.exceptions.HTTPError as e:
        print(f"[telegram] HTTPError: {e}")
        try:
            print("[telegram] error json:", response.json())
        except Exception:
            print("[telegram] error text:", response.text)
        return False
    except Exception as e:
        print(f"[telegram] other error: {e}")
        return False


class SensorData(BaseModel):
    """Pydantic model for sensor data validation"""
    temperature: float = Field(..., ge=-50, le=100, description="Temperature in Celsius")
    humidity: float = Field(..., ge=0, le=100, description="Humidity percentage")
    device_id: str = Field(default="unknown", description="Device identifier")


@https_fn.on_request()
def receive_sensor_data(req: https_fn.Request) -> https_fn.Response:
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
        
        # Validate data using Pydantic model
        try:
            sensor_data = SensorData(**data)
        except ValidationError as e:
            return https_fn.Response(
                json.dumps({"error": "Validation error", "details": e.errors()}),
                status=400,
                headers={"Content-Type": "application/json"}
            )
        
        # Store data in Firebase Realtime Database
        ref = db.reference('sensor_readings')
        
        # Prepare data to save
        sensor_reading = {
            "temperature": sensor_data.temperature,
            "humidity": sensor_data.humidity,
            "device_id": sensor_data.device_id,
            "timestamp": datetime.utcnow().isoformat()
        }
        
        # Push new reading to database
        new_reading_ref = ref.push(sensor_reading)
        
        # Check humidity threshold and send telegram alert if needed
        alert_sent = False
        if sensor_data.humidity > 65:
            alert_sent = send_telegram_alert(
                sensor_data.humidity,
                sensor_data.temperature,
                sensor_data.device_id
            )
        
        response_data = {
            "status": "success",
            "message": "Sensor data saved successfully",
            "data": {
                "temperature": sensor_data.temperature,
                "humidity": sensor_data.humidity,
                "device_id": sensor_data.device_id,
                "reading_id": new_reading_ref.key,
                "alert_sent": alert_sent
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
