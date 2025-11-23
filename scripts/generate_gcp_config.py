import json
import os

Import("env")

PROJECT_ROOT = env.Dir("$PROJECT_DIR").abspath
SECRET_PATH = os.path.join(PROJECT_ROOT, "secret.json")
OUTPUT_DIR = os.path.join(PROJECT_ROOT, "include")
OUTPUT_PATH = os.path.join(OUTPUT_DIR, "Secrets.hpp")
REGION = "us-central1"
FUNCTION_NAME = "receive_sensor_data"
PROJECT_NAME = "garage_humidity"

print(f"output dir {OUTPUT_DIR}")

def main():
    print("Generating GCP config header...")
    if os.path.exists(OUTPUT_PATH):
        return

    if not os.path.exists(SECRET_PATH):
        raise SystemExit(f"secret.json not found at {SECRET_PATH}")

    with open(SECRET_PATH, "r", encoding="utf-8") as f:
        secret = json.load(f)

    project_id = secret.get("project_id", PROJECT_NAME)
    client_email = secret.get("client_email", f"esp8266-function-invoker@{PROJECT_NAME}.iam.gserviceaccount.com")
    private_key = secret.get("private_key", "")

    private_key_escaped = private_key.replace("\r\n", "\n").replace("\r", "\n")


    header = f"""#pragma once

// Auto-generated from secret.json by scripts/generate_gcp_config.py
// Do NOT commit this file with real Secrets.

static const char* GCP_TOKEN_URL = "https://oauth2.googleapis.com/token";
static const char* IAM_GENERATE_ID_TOKEN_URL =
    "https://iamcredentials.googleapis.com/v1/projects/-/serviceAccounts/{client_email}:generateIdToken";

static const char* SERVICE_ACCOUNT_EMAIL = "{client_email}";

static const char* PRIVATE_KEY_PEM = R"KEY(
{private_key_escaped}
)KEY";

static const char* function_url = "https://{REGION}-{project_id}.cloudfunctions.net/{FUNCTION_NAME}";
"""

    os.makedirs(OUTPUT_DIR, exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as f:
        f.write(header)

main()