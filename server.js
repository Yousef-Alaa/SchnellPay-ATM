const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');
const axios = require('axios');

// ⚠️ CHANGE THIS to match the Virtual COM Port connected to Proteus ⚠️
const port = new SerialPort({ path: 'COM8', baudRate: 9600 }); 
const parser = port.pipe(new ReadlineParser({ delimiter: '\n' }));

const BASE_URL = 'https://schnell-pay-back-end.vercel.app/api/v1/atm';

parser.on('data', async (data) => {
    const incoming = data.trim();
    if (!incoming) return;

    console.log(`\n[ATM REQUEST]: ${incoming}`);
    
    // Split the incoming string by commas (e.g., "G,01012345678")
    const parts = incoming.split(',');
    const command = parts[0];

    let endpoint = '';
    let payload = {};

    // Match the logic we had in the ESP8266
    if (command === 'G') {
        endpoint = '/generate-pin';
        payload = { phone: parts[1] };
    } 
    else if (command === 'V') {
        endpoint = '/verify';
        payload = { phone: parts[1], atm_code: parts[2] };
    } 
    else if (command === 'D') {
        endpoint = '/deposit';
        payload = { phone: parts[1], atm_code: parts[2], amount: parseInt(parts[3]) };
    } 
    else if (command === 'W') {
        endpoint = '/withdraw';
        payload = { phone: parts[1], atm_code: parts[2], amount: parseInt(parts[3]) };
    }
    

    if (endpoint !== '') {
        try {
            console.log(`Sending to backend: ${endpoint}`, payload);
            
            const response = await axios.post(BASE_URL + endpoint, payload);
            
            if (response.status === 200 || response.status === 201) {
                console.log('Result: PASS');
                // The \n is CRITICAL so the ATmega knows the message is finished!
                port.write('PASS\n'); 
            } else {
                console.log('Result: FAIL');
                port.write('FAIL\n');
            }
        } catch (err) {
            console.error(`Backend Error:`, err.response ? err.response.data : err.message);
            port.write('FAIL\n');
        }
    }
});

port.on('open', () => {
    console.log('Serial Bridge is OPEN and listening to Proteus...');
});