.PHONY: emulator, send_sensor_data_to_local, send_sensore_data_to_cloud, list_active_emulators, deploy

emulator:
	firebase emulators:start
send_sensor_data_to_local:
	curl -X POST http://127.0.0.1:5001/garagehumedity/us-central1/receive_sensor_data \
	-H "Content-Type: application/json" \
	-d '{"temperature": 25.6, "humidity": 60.3, "device_id": "esp32_garage"}'
kill_emulators:
	bash ./scripts/kill_firebase_emulators.sh
send_sensore_data_to_cloud:
	python ./scripts/send_to_cloud.py
deploy:
	firebase deploy --only functions
