RF433 Telnet Chat with LED and Morse Code for M5StickC Plus

Description

RF433 Telnet Chat is an innovative tool designed for the M5StickC Plus to transmit text messages over 433 MHz RF signals. This application operates as a one-way messaging system, allowing the user to send concise text messages from the transmitter to a receiver. Additionally, this version includes LED integration and Morse code capabilities, enabling long-distance communication using lights.

Character Limits

The application is optimized to handle messages up to 30 characters in length for standard text messages. However, with the Morse code feature, you can transmit longer messages as Morse code is more concise. When crafting messages to be transmitted, it is essential to consider the limitation for standard text messages to ensure reliable and accurate delivery of the information.

Applications

Instructional Messages:

Useful for disseminating brief instructional or informational messages in various settings like workshops, classes, or guided tours.

Clandestine Communications:

Suitable for transmitting covert or confidential messages in a secure manner.

Teleprompter Script Basis:

Ideal for sending short, real-time cues or lines to speakers, presenters, or performers discreetly.

Morse Code Communication:

With the added Morse code feature, you can use this tool for long-distance communication using lights, expanding its applications further.

LED Integration:

The application includes LED control, allowing you to visually signal and communicate messages using the built-in LED.

Modes

Transmitter Mode: // #define RF433RX and enable Wi-Fi for Telnet messaging capability.

Receiver Mode: #define RF433RX and disable Wi-Fi, converting the device into a dedicated message receiver that displays the messages on the M5StickC Plus while providing Morse code feedback through the LED.

Configuration

Transmitter:

Uncomment // #define RF433RX
Set wifiHotspotEnabled to true.

Receiver:

Uncomment #define RF433RX
Set wifiHotspotEnabled to false.

Usage

Transmitter Mode:

Send a test message using Button A on the device or via Telnet where you can send messages up to 30 characters in length for standard text messages or longer messages using Morse code. The LED will also display Morse code feedback for transmitted messages.

Receiver Mode:

The device will listen for incoming messages and display them upon receipt, whether they are standard text messages or Morse code messages. The LED will provide Morse code feedback for received messages.

Contributions

Feel free to contribute, improve, or suggest enhancements to this project!

Note: Due to the 30-character limitation for standard text messages, users should craft messages concisely to ensure they are transmitted and received accurately and effectively. With the Morse code feature, longer messages can be transmitted more efficiently, and the LED integration enhances communication capabilities.
