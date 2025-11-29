#!/usr/bin/env bash

# Common Firebase emulator ports
PORTS=(
  9000   # Realtime Database
  5001   # Functions
  4000   # Emulator UI
  5000   # Hosting
  8085   # Pub/Sub
  9199   # Auth
)

echo "🔍 Searching for Firebase emulator processes..."

for port in "${PORTS[@]}"; do
  echo ""
  echo "▶ Checking port $port..."

  # Find all PIDs using this port (LISTENING or ESTABLISHED)
  # netstat -ano output columns: Proto LocalAddr ForeignAddr State PID
  PIDS=$(netstat -ano | findstr ":$port" | awk '{print $5}' | sort -u)

  if [[ -z "$PIDS" ]]; then
    echo "  No processes found on port $port"
    continue
  fi

  echo "  Found PIDs on port $port: $PIDS"

  for pid in $PIDS; do
    if [[ "$pid" == "-" ]]; then
      continue
    fi
    echo "  -> Killing PID $pid"
    # Use cmd.exe so /PID is treated as a flag, not a path by Git Bash
    taskkill //F //PID "$pid"
    if [[ $? -eq 0 ]]; then
      echo "     ✅ Killed PID $pid"
    else
      echo "     ⚠️  Failed to kill PID $pid (might need admin)"
    fi
  done
done

echo ""
echo "✅ Done. Firebase emulator ports should now be free."
