#ifndef MENU_H
#define MENU_H

#include "ili9488.h"
#include "xpt2046.h"

/* ── Screen states ────────────────────────────────────────────────── */
typedef enum {
    SCREEN_MAIN,
	SCREEN_SETTINGS,
    SCREEN_OPEN_VERIFY,
	SCREEN_TAKE_VERIFY,
	SCREEN_ADD_VERIFY,
	SCREEN_F_OPEN,
	SCREEN_F_TAKE,
	SCREEN_F_ADD,
	SCREEN_K_OPEN,
	SCREEN_K_TAKE,
	SCREEN_K_ADD,
	SCREEN_ADD_FINGERPRINT,
	SCREEN_SET_MEDS,
	SCREEN_NAME_ENTRY,
	SCREEN_VIEW_STATS,
	SCREEN_REFILL_PROMPT,
	SCREEN_CHANGE_VOICE,
	SCREEN_SET_TIME,
} ScreenState;

void Menu_Draw(void);
void Menu_HandleTouch(XPT2046_Touch *touch);
void Menu_UpdateClock(void);

void Screen_Settings_Draw(void);
void Screen_Settings_HandleTouch(XPT2046_Touch *touch);

void Screen_SetMeds_Draw(void);
void Screen_SetMeds_HandleTouch(XPT2046_Touch *touch);
void Screen_SetMeds_UpdateEntry(void);

void Screen_Verify_Draw(void);
void Screen_Verify_HandleTouch(XPT2046_Touch *touch);

void Screen_Fingerprint_Draw(void);
void Screen_Fingerprint_SetStatus(uint8_t success, uint8_t open, const unsigned char *user_bmp);
void Screen_FK_HandleTouch(XPT2046_Touch *touch);
void Screen_Keypad_Draw(void);
void Screen_Keypad_UpdateEntry(uint8_t masked);
void Screen_KeypadAdd_HandleTouch(XPT2046_Touch *touch);

void Screen_VerifyToAdd_Draw(void);
void Screen_AddUser_Draw(void);
void Screen_SetMessage(const char *msg);
void Screen_AddVerify_HandleTouch(XPT2046_Touch *touch);

void Screen_ChangeVoice_Draw(void);
void Screen_ChangeVoice_HandleTouch(XPT2046_Touch *touch);

void Screen_SetTime_Draw(void);
void Screen_SetTime_HandleTouch(XPT2046_Touch *touch);
void Screen_SetTime_UpdateEntry(void);


void Screen_NameEntry_Draw(void);
void Screen_NameEntry_HandleTouch(XPT2046_Touch *touch);
void Screen_NameEntry_Update(void);

void Screen_ViewStats_Draw(void);
void Screen_ViewStats_HandleTouch(XPT2046_Touch *touch);

void Screen_RefillPrompt_Draw(void);
void Screen_RefillPrompt_HandleTouch(XPT2046_Touch *touch);


void draw_bmp_scaled(uint16_t cx, uint16_t cy, const unsigned char *bmp, uint8_t scale);
void Screen_SetEnrollStep(const char *msg, const unsigned char *bmp);


#endif /* MENU_H */
