Other comments / follow-ups

- Persistence: currently the meds times (`user_hr`, `user_min`, etc.) are stored in RAM only. If you want them to survive power cycles, write them to flash (HAL flash or option bytes) or use a simple EEPROM emulation.

- UI Improvements: the current HHMM entry is minimal (4-digit entry via keypad). Consider a focused keypad editor screen with:
  - Visual cursor showing HH:MM as user types
  - Separate Save/Cancel buttons on-screen
  - +/- buttons for hour/min increments

- RTC integration: if you want to set the actual RTC alarm instead of just storing a variable, call `HAL_RTC_SetTime`/`HAL_RTC_SetDate` or configure the alarm registers after validating input.

- Input validation UX: show partial typed digits on the screen as the user enters digits. If non-digit input arrives (shouldn't with numeric keypad), reject and beep.

- Internationalization/time formats: code stores 24-hour values; convert to 12-hour display with AM/PM for UI if desired.

- Tests: add unit tests (if possible) around the HHMM parsing/validation logic. For embedded testing, add a debug UART command to print current meds times.

- Refactor idea: create a `settings.c` with a `Settings` struct and accessor functions. This cleans up global exposure via `extern`.

- Safety: when saving user-set times, consider confirming action to prevent accidental changes (e.g., "Save 07:30? Press # again to confirm").
