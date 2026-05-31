# automated-pillbox

An embedded medication-management system that dispenses the right dose at the right time, gates access behind biometric and PIN authentication, and tracks adherence, built on an STM32 Nucleo (Cortex-M4) board.

**Demo**
https://www.youtube.com/watch?v=wKqTfHsi4G4

**Overview**
Medication non-adherence leads to missed doses, overdosing, and incorrect timing, which in turn raises the risk of serious health complications, hospitalizations, and reduced treatment effectiveness. Existing solutions usually solve only one piece: some give reminders, others lock the medication away, but few combine scheduling, security, and tracking in one affordable device.

This project is an automated pillbox that brings those together. A rotating tray with refillable slots dispenses the correct day's medication on schedule, access is controlled through multi-factor authentication for users, caretakers, and physicians, and the device tracks whether each scheduled dose was taken.

**Features**
- Rotating pillbox with refillable per-day slots, indexed by a stepper motor
- Interactive TFT touchscreen for task selection, scheduling, and adherence stats
- Solenoid deadbolt lock that secures the lid against unauthorized refills
- Two-factor authentication: fingerprint sensor or keypad PIN
- Audible alarm played from the SD card through a speaker at scheduled dose times

**Hardware & Interfaces**
- STM32 Nucleo (Cortex-M4): Main system controller responsible for coordinating all device functions.
- TFT Touchscreen (ILI9488) – SPI: Provides the user interface for scheduling medications, viewing system status, and accessing dispensing statistics.
- Fingerprint Sensor (UART): Handles biometric authentication for secure access.
- Keypad (GPIO): Allows users to enter PINs and interact with the system.
- Stepper Motor (12V) – PWM via motor driver: Rotates the dispensing tray to release the correct medication.
- Solenoid Deadbolt (12V) – GPIO: Locks and unlocks the refill compartment to prevent unauthorized access.
- SD Card and Speaker (SPI): Stores and plays audio alarms and notifications.
- Real-Time Clock (RTC): Maintains accurate date and time information for medication scheduling.

Security: Fingerprint templates are stored exclusively on the fingerprint sensor's onboard controller. The STM32 never stores or processes biometric data directly and only receives authentication results (match or no match).


**Tech Stack**
Language: C (bare-metal / STM32 HAL)
Platform: STM32 Nucleo, ARM Cortex-M4
Peripherals: SPI, UART, GPIO, PWM, RTC
Tools: STM32CubeIDE, 3D-printed enclosure


