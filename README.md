# automated-pillbox

An embedded medication-management system that dispenses the right dose at the right time, gates access behind biometric and PIN authentication, and tracks adherence — built on an STM32 Nucleo (Cortex-M4) board.

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
ComponentInterfaceRoleSTM32 Nucleo (Cortex-M4)—Main controllerTFT touchscreen (ILI9488)SPIUI, status, scheduling, statsFingerprint sensorUARTBiometric authenticationKeypadGPIOPIN authentication / inputStepper motor (12V)PWM → driverRotates the tray to dispense pillsSolenoid deadbolt (12V)GPIOLocks/unlocks the lid for refillsSD card + speakerSPIStored audio alarm playbackRTC—Real-time scheduling of doses
Security note: fingerprint biometric data is never stored on the main MCU — it lives only on the fingerprint module's own controller, so the host firmware only ever sees match/no-match results.


**Tech Stack**
Language: C (bare-metal / STM32 HAL)
Platform: STM32 Nucleo, ARM Cortex-M4
Peripherals: SPI, UART, GPIO, PWM, RTC
Tools: STM32CubeIDE, 3D-printed enclosure


Working video: https://www.youtube.com/watch?v=wKqTfHsi4G4


