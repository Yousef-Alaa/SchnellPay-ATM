import serial
import requests
import json

# Configuration
SERIAL_PORT = 'COM8'  # ⚠️ Change this to match your setup
BAUD_RATE = 9600
BASE_URL = 'https://schnell-pay-back-end.vercel.app/api/v1/atm'

try:
    # Initialize Serial connection
    # timeout=1 ensures the read_line() doesn't block forever
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    print(f"Serial Bridge is OPEN on {SERIAL_PORT} and listening to Proteus...")

    while True:
        # Read data until newline
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8').strip()
            
            if not line:
                continue

            print(f"\n[ATM REQUEST]: {line}")

            # Split the incoming string by commas
            parts = line.split(',')
            command = parts[0]
            
            endpoint = ""
            payload = {}

            # Logic matching your ESP8266/Node.js structure
            try:
                if command == 'G':
                    endpoint = '/generate-pin'
                    payload = {'phone': parts[1]}
                elif command == 'V':
                    endpoint = '/verify'
                    payload = {'phone': parts[1], 'atm_code': parts[2]}
                elif command == 'D':
                    endpoint = '/deposit'
                    payload = {'phone': parts[1], 'atm_code': parts[2], 'amount': int(parts[3])}
                elif command == 'W':
                    endpoint = '/withdraw'
                    payload = {'phone': parts[1], 'atm_code': parts[2], 'amount': int(parts[3])}

                if endpoint:
                    full_url = BASE_URL + endpoint
                    print(f"Sending to backend: {endpoint} {payload}")
                    
                    response = requests.post(full_url, json=payload)

                    if response.status_code in [200, 201]:
                        print("Result: PASS")
                        ser.write(b'PASS\n') # b'' converts string to bytes
                    else:
                        print(f"Result: FAIL (Status: {response.status_code})")
                        ser.write(b'FAIL\n')

            except (IndexError, ValueError) as e:
                print(f"Data format error: {e}")
                ser.write(b'FAIL\n')
            except requests.exceptions.RequestException as e:
                print(f"Backend Error: {e}")
                ser.write(b'FAIL\n')

except serial.SerialException as e:
    print(f"Error opening serial port: {e}")
except KeyboardInterrupt:
    print("\nClosing bridge...")
finally:
    if 'ser' in locals() and ser.is_open:
        ser.close()