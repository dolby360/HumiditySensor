import json
from pathlib import Path

import requests
import google.auth.transport.requests
from google.oauth2 import service_account


SERVICE_ACCOUNT_FILE = Path(__file__).resolve().parents[1] / "secret.json"
TARGET_AUDIENCE = "https://receive-sensor-data-u7fbhevhoa-uc.a.run.app"
ENDPOINT = (
    "https://receive-sensor-data-u7fbhevhoa-uc.a.run.app/"
    "garagehumedity/us-central1/receive_sensor_data"
)


def get_id_token() -> str:
    creds = service_account.IDTokenCredentials.from_service_account_file(
        str(SERVICE_ACCOUNT_FILE),
        target_audience=TARGET_AUDIENCE,
    )
    creds.refresh(google.auth.transport.requests.Request())
    return creds.token


def main() -> None:
    id_token = get_id_token()

    payload = {
        "temperature": 25.6,
        "humidity": 60.3,
        "device_id": "esp32_garage",
    }

    headers = {
        "Authorization": f"Bearer {id_token}",
        "Content-Type": "application/json",
    }

    response = requests.post(ENDPOINT, json=payload, headers=headers, timeout=10)
    print(f"Status: {response.status_code}")
    try:
        print("Response:", response.json())
    except Exception:
        print("Response text:", response.text)


if __name__ == "__main__":
    main()
