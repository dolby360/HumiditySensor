.PHONY: emulator, send_sensor_data_to_local, send_sensore_data_to_cloud, list_active_emulators, deploy

emulator:
	firebase emulators:start
send_sensor_data_to_local:
	curl -X POST http://127.0.0.1:5001/garagehumedity/us-central1/receive_sensor_data \
	-H "Content-Type: application/json" \
	-d '{"temperature": 25.5, "humidity": 60.3, "device_id": "esp32_garage"}'
kill_emulators:
	bash kill_firebase_emulators.sh
send_sensore_data_to_cloud:
	curl -X POST https://receive-sensor-data-u7fbhevhoa-uc.a.run.app/garagehumedity/us-central1/receive_sensor_data \
	-H "Content-Type: application/json" \
	-d '{"temperature": 25.5, "humidity": 60.3, "device_id": "esp32_garage"}'
deploy:
	firebase deploy --only functions