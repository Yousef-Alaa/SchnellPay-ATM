# Schnell-Pay ATM

This project contains the microcontroller-based ATM interface for the Schnell-Pay digital wallet. It utilizes an ATmega32 microcontroller and can be run on physical hardware (ETA32 kit or ETA32mini kit) or simulated using Proteus.

## Features
- Microcontroller-based physical ATM interface
- Hardware and software simulation support (ETA32 kits or Proteus)
- Bridge servers in Python and Node.js for serial-to-HTTP communication with the backend
- Realistic user flows mimicking ATM interactions

## Project Structure
- Contains the embedded C code for the ATmega32 microcontroller.
- Provides bridging scripts (`server.py` or `server.js`) to connect the serial ATM interface with the Backend API.

## Installation and Usage Guide

### Prerequisites (Bridge Server)
Depending on whether you choose to run the Node.js or Python bridge server, you must install the necessary packages first:
- **For Node.js (`server.js`)**: 
  ```bash
  npm install serialport axios
  ```
- **For Python (`server.py`)**:
  ```bash
  pip install pyserial requests
  ```

You can run this project in two scenarios: on physical hardware or via simulation.

### Option 1: Physical Kit (ETA32 or ETA32mini)

1. **Flash the Microcontroller**:
   - Compile the embedded C code.
   - Flash the resulting `.hex` file onto your ETA32 or ETA32mini kit.

2. **Connect Hardware**:
   - Connect the kit to your computer via USB/Serial cable.
   - Note the COM port assigned to the device (e.g., `COM3`).

3. **Run the Bridge Server**:
   - Open either `server.py` or `server.js`.
   - **Important**: Update the `COM` port variable inside the script to match the port assigned to your hardware kit.
   - Start the server:
     ```bash
     node server.js
     # OR
     python server.py
     ```

### Option 2: Simulation (Proteus)

To run the simulation, you need to set up virtual serial ports.

1. **Install Virtual Serial Port Emulator (VSPE)**:
   - Download and install VSPE or a similar virtual COM port tool.

2. **Create a Virtual Pair**:
   - Open VSPE and create a new "Pair" device.
   - Select two unused COM ports (e.g., `COM1` and `COM2`).

3. **Configure Proteus**:
   - Open the Proteus simulation file for the ATM project.
   - Locate the `COMPIM` component in the schematic.
   - Double-click the `COMPIM` component and configure it to use the first COM port of your virtual pair (e.g., `COM1`). Ensure the baud rate matches the code.
   - Double-click the ATmega32 microcontroller component in Proteus. In the "Program File" field, browse and select your compiled `firmware.hex` file.

4. **Run the Bridge Server**:
   - Open either `server.py` or `server.js`.
   - **Important**: Update the `COM` port variable inside the script to use the second COM port of your virtual pair (e.g., `COM2`).
   - Start the server:
     ```bash
     node server.js
     # OR
     python server.py
     ```

5. **Start Simulation**:
   - Play the simulation in Proteus. The virtual ATM will communicate with the bridge server over the virtual serial connection.

## Related Repositories

Explore the other components of the Schnell-Pay platform:
- [Frontend Repository](https://github.com/Mahmoud-Nasser1/SchnellPayy) - React-based web interface for users.
- [Backend Repository](https://github.com/Yousef-Alaa/SchnellPay-BackEnd) - Node.js APIs and MS SQL server configuration.
