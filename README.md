# Indicator Control Module

## Overview

This project implements an automotive Indicator Control Module ECU using embedded software. It simulates left/right indicators and hazard lights using a microcontroller.

The system is designed using a **layered architecture**:

* Base Software (BSW)
* Application Software (ASW)

The ASW is implemented using a **state machine with a 100 ms scheduler**.

---

## Features

* Left & Right indicator control
* Indicator switching logic
* Hazard mode
* 300 ms LED blinking
* 1-second button hold detection
* Software debouncing
* UART logging
* Fault-tolerant design

---

## Architecture

### BSW

* GPIO Driver
* UART Driver
* PWM Driver
* 100 ms Scheduler

### ASW

* State machine
* Button handling
* Indicator logic
* Hazard logic

> The Application Software was implemented directly in C using a state machine approach instead of Simulink, while maintaining clear separation between BSW and ASW layers.

---

## Timing

* Scheduler: 100 ms
* Blink: 300 ms
* Hold: ≥ 1000 ms
* Debounce: ~200 ms

---

## Hardware

* Arduino Uno
* 2 Push Buttons
* 2 LEDs
* Resistors

---

## How to Run

1. Open `src/main.ino`
2. Upload to Arduino
3. Open Serial Monitor (9600 baud)

---

## Demo Video

(Add your video link here)

---

## 📄 Documentation

See: `docs/Indicator_Control_Module.pdf`
