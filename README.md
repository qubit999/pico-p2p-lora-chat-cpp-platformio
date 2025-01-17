# 📡 Pico P2P LoRa Chat 🚀

This project demonstrates a half-duplex peer-to-peer (P2P) cipher chat application using Raspberry Pi Pico 2W and Waveshare SX1262 LoRa module. I tested a distance of a few centimeters between each peer and the speed was amazingly fast (realtime).

## Hardware Used

- Raspberry Pi Pico 2W
- Waveshare SX1262 LoRa Module

For more information on the Waveshare SX1262 LoRa Module, visit the [Waveshare Wiki](https://www.waveshare.com/wiki/Pico-LoRa-SX1262).

## Prerequisites

- VS Code installed with PlatformIO extension
- [RadioLib library for Arduino-Core](https://github.com/jgromes/RadioLib) installed (search in PlatformIO)

## Wiring

Connect the Pico 2W to the Waveshare SX1262 module. It's an expansion module that sits on top of the Pico.

## Setup

1. Clone the repository:
    ```sh
    git clone https://github.com/qubit999/pico-p2p-lora-chat-cpp-platformio.git
    ```

2. Import the project with PlatformIO and adjust `radio.setFrequency(868.0);` to match your region (default setting: 868 MHz).

3. Build and flash the code to the Pico 2W.

## Usage

1. Power on both Pico 2W devices with the SX1262 modules connected.
2. Open the Serial Monitor in VS Code for both devices.
3. Type a message in one Serial Monitor and press enter.
4. The message should appear on the other device's Serial Monitor.

Now you're ready to chat! :-)

## Examples

The examples shown here show the Arduino IDE Serial Monitor but it also works with VS Code Terminal mode (I was just too lazy to screenshot 🥸).

![Peer #1](https://i.imgur.com/Nr0fo4C.png)
![Peer #2](https://i.imgur.com/TOd5P33.png)

## Show Support

If you find this project useful, [you can buy me a coffee](https://ko-fi.com/alexsla). 😇