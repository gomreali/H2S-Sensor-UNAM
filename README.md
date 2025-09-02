# Low-cost H2S Sensor

This repository contains the Arduino code for a **low-cost hydrogen sulfide (H₂S) sensor**, developed at the **Universidad Nacional Autónoma de México (UNAM)**.

The system measures H₂S gas concentration, sensor temperature, and RTC temperature. Data are displayed on an OLED screen and stored on an SD card in CSV format. Calibration parameters are read from a configuration file (`Cal.txt`) stored on the SD card.

---

## Authors
- Pech Figueroa  
- Gómez-Reali M.A.  
- Rosales-Fuerte  
- Rodríguez-Martínez  

---

## Features
- Reads H₂S concentration using **DFRobot MultiGas Sensor** (I2C or UART).
- Logs data with timestamp from **RTC DS3231**.
- Displays information on **OLED SSD1306 (128x32)**.
- Stores readings on an **SD card** in `.csv` format.
- Supports calibration from `Cal.txt` file.
- On/off control using an **interrupt button**.

---

## Hardware Requirements
- Arduino-compatible microcontroller (tested on **Arduino MKRZero**).
- **DFRobot MultiGas Sensor** (H₂S compatible).
- **RTC DS3231** module.
- **OLED SSD1306 display** (128x32, I2C).
- **SD card module**.
- Push button (connected to pin `6`).
- Power supply (battery with JST connector or USB).

---

## Wiring
| Component                  | Connection                          |
|-----------------------------|-------------------------------------|
| DFRobot MultiGas Sensor     | I2C: SDA → A4, SCL → A5 (UNO)       |
| RTC DS3231                  | SDA → A4, SCL → A5 (shared I2C bus) |
| OLED SSD1306 (128x32)       | SDA → A4, SCL → A5 (shared I2C bus) |
| SD card module              | SPI (MOSI, MISO, SCK, CS)           |
| Push button                 | Pin 6 (with `INPUT_PULLDOWN`)       |
| LED (status)                | Built-in LED (pin 13 on UNO)        |

*(Adjust pins depending on the board used — MKRZero and ESP32 have different pin mappings.)*

---

## Required Libraries
Install the following libraries via **Arduino IDE Library Manager**:
- `DFRobot_MultiGasSensor`
- `RTClib`
- `SD`
- `Adafruit_GFX`
- `Adafruit_SSD1306`

---

## Calibration File (`Cal.txt`)
The sensor reads calibration parameters from a file named **`Cal.txt`** located on the SD card.  
The file must contain the following lines:

