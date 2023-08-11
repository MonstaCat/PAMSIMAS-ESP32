# PAMSIMAS-ESP32
PAMSIMAS NodeMCU-ESP32 Dev Board

## Description
PAMSIMAS is the name of the location for the water supply pipe system for residents' homes, this repository is intended for NodeMCU-ESP32 board that responsible to monitor flow rate in water flow sensor and used to determined if the pipe was in normal condition or there's a leak in the pipe in that area.

## Usage 
`pamsimas-esp32` using `NodeMCU-ES32` board that can be programmed via USB Serial, to program with USB serial, you need to connect your `NodeMCU-ESP32` board to your devices, and hit run with your IDE such as Arduino IDE or Visual Studio Code. In the time this was developed, i'am using Visual Studio Code with **[PlatformioIO IDE](https://platformio.org/)** Extension installed in my Visual Studio Code.

## Configuration
You need 2 water flow sensor to run this project, for the upstream sensor, you need to put the water flow sensor into pin number 26, and for the second water flow you need to put into the pin number 27, you need to setup `Firebase Realtime Database` for this project and include your Firebase host and auth key in the code, and you need to put your wifi SSID and Password for the board to be able to send the data to your Firebase.

## Author
>This has been a tough time to develop this project for me. I have all put in a lot of blood, sweat, and tears to make this project working and happen, because this is the first time i'm working with IoT, this put so much challenge for me to keep learn new things everyday, this code is open to public for academic purpose.<br>
🐈‍⬛ **[MonstaCat](https://github.com/MonstaCat)**
