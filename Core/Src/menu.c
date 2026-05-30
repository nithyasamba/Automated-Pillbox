#include "menu.h"
#include "main.h"
#include "pond_bg.h"
#include "ili9488.h"
extern unsigned char turtle_bmp[];
extern unsigned int  turtle_bmp_len;

extern unsigned char happyturtle_bmp[];
extern unsigned int  happyturtle_bmp_len;

extern unsigned char sadturtle_bmp[];
extern unsigned int  sadturtle_bmp_len;

extern unsigned char menu_bmp[];
extern unsigned int  menu_bmp_len;

/* ── Layout constants — matches your original design ─────────────── */
#define SCREEN_W     480
#define SCREEN_H     320
#define MENU_W       140

/* Button definitions */
#define BTN_W        120
#define BTN_H         50
#define BTN_X         10
#define BTN_RADIUS     8

#define BTN_OPEN_Y    40
#define BTN_MEDS_Y   110
#define BTN_SET_Y    180

#define STATS_X      (MENU_W + 40)
#define STATS_Y      220
#define STATS_W      200
#define STATS_H       60

/* ── Back button (used on sub-screens) ───────────────────────────── */
#define BACK_X		  10
#define BACK_Y		  10
#define BACK_W		  80
#define BACK_H		  35

// Declare for real time clock
extern RTC_HandleTypeDef hrtc;

extern uint32_t user_hr;
extern uint32_t user_min;
extern int pills_remaining;
extern int dispenses_since_refill;
extern int last_dispense_valid;
extern uint8_t last_dispense_hour;
extern uint8_t last_dispense_minute;
extern int dose_overdue;
extern uint32_t missed_count;
extern DoseStatus today_dose_status;
extern DoseHistoryEntry dose_history[7];

extern UserProfile profiles[];
extern int num_profiles;
extern int selected_voice;

/*
 * Access to runtime state and keypad buffer in `main.c`.
 * We update these here so the Settings touch handler can fully
 * transition into the meds-time screen (clear buffer, update state,
 * then draw). This keeps the user-visible transition atomic.
 */
extern int buffer_idx;
extern char password_buffer[16];
extern char name_buffer[24];
extern int name_buffer_idx;
extern int adding_name;
extern int pending_profile_idx;
extern int adding_passcode;


void draw_bmp_scaled(uint16_t cx, uint16_t cy,
                            const unsigned char *bmp, uint8_t scale)
{
    uint32_t pix_offset = bmp[10] | (bmp[11]<<8) | (bmp[12]<<16) | (bmp[13]<<24);
    int32_t  w = (int32_t)(bmp[18] | (bmp[19]<<8) | (bmp[20]<<16) | (bmp[21]<<24));
    int32_t  h = (int32_t)(bmp[22] | (bmp[23]<<8) | (bmp[24]<<16) | (bmp[25]<<24));
    uint32_t row_size = ((w * 3 + 3) / 4) * 4;

    uint16_t x0 = cx - (w * scale) / 2;
    uint16_t y0 = cy - (h * scale) / 2;

    for (int32_t row = 0; row < h; row++) {
        const unsigned char *rowptr = bmp + pix_offset + (h - 1 - row) * row_size;
        for (int32_t col = 0; col < w; col++) {
            uint8_t b_val = rowptr[col * 3 + 0];
            uint8_t g_val = rowptr[col * 3 + 1];
            uint8_t r_val = rowptr[col * 3 + 2];
            uint32_t color = ILI9488_COLOR(r_val, g_val, b_val);
            for (uint8_t sy = 0; sy < scale; sy++)
                for (uint8_t sx = 0; sx < scale; sx++)
                    ILI9488_DrawPixel(x0 + col*scale + sx,
                                      y0 + row*scale + sy, color);
        }
    }
}


static void format_keypad_entry(char *out, size_t out_size, uint8_t masked)
{
    int max_chars = (int)out_size - 1;
    int count = (buffer_idx < max_chars) ? buffer_idx : max_chars;

    for (int i = 0; i < count; i++) {
        out[i] = masked ? '*' : password_buffer[i];
    }
    out[count] = '\0';
}

static void format_display_time(uint8_t hour24, uint8_t minute, char *out, size_t out_size)
{
    uint8_t hour = hour24;
    const char *ampm = (hour >= 12) ? "PM" : "AM";
    if (hour == 0) hour = 12;
    else if (hour > 12) hour -= 12;

    snprintf(out, out_size, "%u:%02u %s", hour, minute, ampm);
}

static uint32_t dose_status_color(DoseStatus status)
{
	switch (status) {
	case ON_TIME:
		return COLOR_GREEN;
	case LATE:
		return COLOR_ORANGE;
	case MISSED:
		return COLOR_RED;
	default:
		return POND_SURFACE;
	}
}

//static DoseStatus today_chart_status(void)
//{
//	if (today_dose_status != NONE) {
//		return today_dose_status;
//	}
//
//	if (dose_overdue) {
//		return MISSED;
//	}
//	return NONE;
//}

static void format_next_dose(char *out, size_t out_size)
{
    format_display_time((uint8_t)(user_hr), (uint8_t)(user_min), out, out_size);
}

static void draw_name_status(const char *msg, uint32_t color)
{
    ILI9488_FillRect(40, 104, 400, 24, POND_DEEP);
    ILI9488_DrawStringCentered(40, 104, 400, 24, msg, color, POND_MID, 1);
}

static void format_meds_entry(char *out, size_t out_size)
{
    char digits[4] = {'_', '_', '_', '_'};

    for (int i = 0; i < buffer_idx && i < 4; i++) {
        digits[i] = password_buffer[i];
    }

    snprintf(out, out_size, "%c%c:%c%c", digits[0], digits[1], digits[2], digits[3]);
}

// draw a labeled button 
static void draw_button(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                        const char *label, uint32_t bg)
{
    ILI9488_FillRoundRect(x, y, w, h, BTN_RADIUS, bg);
    ILI9488_DrawStringCentered(x, y, w, h, label, COLOR_WHITE, bg, 2);
}

static void draw_name_key(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                          const char *label, uint32_t bg)
{
    ILI9488_FillRoundRect(x, y, w, h, 6, bg);
    ILI9488_DrawStringCentered(x, y, w, h, label, COLOR_WHITE, bg, 1);
}

static void draw_refill_status(void)
{
    char buf[40];
    uint32_t reminder_color = COLOR_WHITE;

    ILI9488_FillRect(MENU_W + 40, 158, 280, 58, POND_DEEP);

    snprintf(buf, sizeof(buf), "Pills left: %d", pills_remaining);
    if (pills_remaining <= 3) {
        reminder_color = (pills_remaining == 0) ? COLOR_RED : COLOR_YELLOW;
    }
    ILI9488_DrawString(MENU_W + 40, 160, buf, COLOR_WHITE, POND_DEEP, 2);

    if (pills_remaining == 0) {
        ILI9488_DrawString(MENU_W + 40, 186, "Please refill", reminder_color, POND_DEEP, 2);
    } else if (pills_remaining <= 3) {
        snprintf(buf, sizeof(buf), "Reminder: %d left", pills_remaining);
        ILI9488_DrawString(MENU_W + 40, 186, buf, reminder_color, POND_DEEP, 2);
    } else if (dose_overdue) {
        ILI9488_DrawString(MENU_W + 40, 186, "Medication not taken", COLOR_ORANGE, POND_DEEP, 2);
    }
}

static void draw_next_med_time(void)
{
    char next_time[24];
    char line[40];

    format_next_dose(next_time, sizeof(next_time));
    snprintf(line, sizeof(line), "Next Med: %s", next_time);
    ILI9488_FillRect(MENU_W + 40, 96, 220, 26, POND_DEEP);
    ILI9488_DrawString(MENU_W + 40, 100, line, COLOR_WHITE, POND_DEEP, 2);
}

static uint8_t stats_page = 0;
static uint8_t users_page = 0;

static void draw_stats_tabs(void)
{
    draw_button(80, 98, 100, 34, "System", stats_page == 0 ? COLOR_GREEN : COLOR_BLUE);
    draw_button(190, 98, 100, 34, "History", stats_page == 1 ? COLOR_GREEN : COLOR_BLUE);
    draw_button(300, 98, 100, 34, "Users", stats_page == 2 ? COLOR_GREEN : COLOR_BLUE);
}

static void draw_dose_history_chart(void)
{
    const int chart_x = 52;
    const int chart_y = 258;
    const int chart_h = 44;
    const int bar_w = 40;
    const int gap = 10;

    ILI9488_DrawHLine(chart_x, chart_y + chart_h, 330, POND_BLUE);

    for (int i = 0; i < 7; i++) {
        int x = chart_x + i * (bar_w + gap);
        DoseStatus status = dose_history[i].status;
        char label[8];

        if (status != NONE) {
            ILI9488_FillRect(x, chart_y, bar_w, chart_h, dose_status_color(status));
        } else {
            ILI9488_DrawHLine(x, chart_y, bar_w, POND_BLUE);
            ILI9488_DrawHLine(x, chart_y + chart_h, bar_w, POND_BLUE);
            ILI9488_DrawVLine(x, chart_y, chart_h, POND_BLUE);
            ILI9488_DrawVLine(x + bar_w, chart_y, chart_h, POND_BLUE);
        }

        if (dose_history[i].day != 0 || dose_history[i].month != 0) {
            snprintf(label, sizeof(label), "%02u:%02u",
                     (unsigned)dose_history[i].hour,
                     (unsigned)dose_history[i].minute);
        } else {
            snprintf(label, sizeof(label), "#%d", i+1);
        }

        ILI9488_DrawStringCentered(x - 4, chart_y + chart_h + 4, bar_w + 8, 14,
                                   label, POND_BLUE, POND_DEEP, 1);
    }
}

static void draw_system_stats(void)
{
    char buf[48];
    char next_dose[24];
    char last_dispense[24];
    int most_active_idx = -1;
    uint32_t most_uses = 0;
    int row_y = 146;

    format_next_dose(next_dose, sizeof(next_dose));
    if (last_dispense_valid) {
        format_display_time(last_dispense_hour, last_dispense_minute, last_dispense, sizeof(last_dispense));
    } else {
        snprintf(last_dispense, sizeof(last_dispense), "None yet");
    }

    for (int i = 0; i < num_profiles; i++) {
        if ((most_active_idx < 0) || (profiles[i].access_count > most_uses)) {
            most_active_idx = i;
            most_uses = profiles[i].access_count;
        }
    }

    ILI9488_DrawString(50, row_y, "Pills remaining:", POND_BLUE, POND_DEEP, 2);
    snprintf(buf, sizeof(buf), "%d", pills_remaining);
    ILI9488_DrawString(260, row_y, buf, COLOR_WHITE, POND_DEEP, 2);
    row_y += 26;

    ILI9488_DrawString(50, row_y, "Dispenses since refill:", POND_BLUE, POND_DEEP, 2);
    snprintf(buf, sizeof(buf), "%d", dispenses_since_refill);
    ILI9488_DrawString(350, row_y, buf, COLOR_WHITE, POND_DEEP, 2);
    row_y += 26;

    ILI9488_DrawString(50, row_y, "Next scheduled dose:", POND_BLUE, POND_DEEP, 2);
    ILI9488_DrawString(320, row_y, next_dose, COLOR_WHITE, POND_DEEP, 2);
    row_y += 26;

    ILI9488_DrawString(50, row_y, "Last dispense:", POND_BLUE, POND_DEEP, 2);
    ILI9488_DrawString(250, row_y, last_dispense, COLOR_WHITE, POND_DEEP, 2);
    row_y += 26;

    ILI9488_DrawString(50, row_y, "Dose overdue:", POND_BLUE, POND_DEEP, 2);
    ILI9488_DrawString(250, row_y, dose_overdue ? "Yes" : "No",
                       dose_overdue ? COLOR_ORANGE : COLOR_WHITE, POND_DEEP, 2);
    row_y += 26;

    ILI9488_DrawString(50, row_y, "Most active user:", POND_BLUE, POND_DEEP, 2);
    if (most_active_idx >= 0) {
        snprintf(buf, sizeof(buf), "%s (%lu)", profiles[most_active_idx].name,
                 (unsigned long)profiles[most_active_idx].access_count);
    } else {
        snprintf(buf, sizeof(buf), "None");
    }
    ILI9488_DrawString(290, row_y, buf, COLOR_WHITE, POND_DEEP, 2);
}

static void draw_history_stats(void)
{
	char buf[48];
	uint32_t on_time_count = 0;
	uint32_t late_total = 0;
	uint32_t recorded_uses = 0;

	for (int i = 0; i < 7; i++) {
		DoseStatus status = dose_history[i].status;

		if (status == ON_TIME) {
			on_time_count++;
			recorded_uses++;
		} else if (status == LATE) {
			late_total++;
			recorded_uses++;
		}
	}
	 ILI9488_DrawString(50, 146, "Missed doses:", POND_BLUE, POND_DEEP, 2);
	 snprintf(buf, sizeof(buf), "%lu", (unsigned long)missed_count);
	 ILI9488_DrawString(290, 146, buf, COLOR_RED, POND_DEEP, 2);

	 ILI9488_DrawString(50, 172, "Uses shown:", POND_BLUE, POND_DEEP, 2);
	 snprintf(buf, sizeof(buf), "%lu", (unsigned long)recorded_uses);
	 ILI9488_DrawString(290, 172, buf, POND_BLUE, POND_DEEP, 2);

	 ILI9488_DrawString(50, 198, "On-time uses shown:", POND_BLUE, POND_DEEP, 2);
	 snprintf(buf, sizeof(buf), "%lu", (unsigned long)on_time_count);
	 ILI9488_DrawString(290, 198, buf, COLOR_GREEN, POND_DEEP, 2);

	 ILI9488_DrawString(50, 224, "Late uses shown:", POND_BLUE, POND_DEEP, 2);
	 snprintf(buf, sizeof(buf), "%lu", (unsigned long)late_total);
	 ILI9488_DrawString(290, 224, buf, COLOR_ORANGE, POND_DEEP, 2);

	 ILI9488_DrawString(50, 244, "Chart shows last 7 dispense times", POND_BLUE, POND_DEEP, 1);

	    draw_dose_history_chart();
}

#define USERS_PER_PAGE 4

static void draw_user_stats(void)
{
    ILI9488_DrawString(50,  142, "Name",  POND_BLUE, POND_DEEP, 2);
    ILI9488_DrawString(130, 142, "Uses",  POND_BLUE, POND_DEEP, 2);
    ILI9488_DrawString(190, 142, "% Late", POND_BLUE, POND_DEEP, 2);
    ILI9488_DrawString(280, 142, "Avg Late Min", POND_BLUE, POND_DEEP, 2);
    ILI9488_DrawHLine(40, 160, SCREEN_W - 80, POND_BLUE);

    int start = users_page * USERS_PER_PAGE;
    int end   = start + USERS_PER_PAGE;
    if (end > num_profiles) end = num_profiles;

    for (int i = start; i < end; i++) {
    	uint16_t row_y = 170 + (i - start) * 28;
        char count_str[8];
        char late_str[8];
        uint32_t pct_late = 0;
        char avg_str[12];
        uint32_t avg_late_minutes = 0;

        if (profiles[i].access_count > 0) {
            pct_late = (profiles[i].late_count * 100u) / profiles[i].access_count;
        }
        if (profiles[i].late_count > 0) {
        	avg_late_minutes = profiles[i].total_late_minutes / profiles[i].late_count;
        }

        snprintf(count_str, sizeof(count_str), "%lu",
                 (unsigned long)profiles[i].access_count);
        snprintf(late_str, sizeof(late_str), "%lu%%",
                 (unsigned long)pct_late);

        if (profiles[i].late_count > 0) {
        	snprintf(avg_str, sizeof(avg_str), "%lum",
        	                 (unsigned long)avg_late_minutes);
        } else {
        	 snprintf(avg_str, sizeof(avg_str), "--");
        }

        uint32_t late_color = (pct_late >= 50) ? COLOR_RED :
                              (pct_late >= 25) ? COLOR_ORANGE :
                              COLOR_WHITE;

        uint32_t avg_color = (avg_late_minutes >= 10) ? COLOR_RED :
                                      (avg_late_minutes >= 5) ? COLOR_ORANGE :
                                      COLOR_WHITE;
        ILI9488_DrawString(50,  row_y, profiles[i].name, COLOR_WHITE, POND_DEEP, 2);
        ILI9488_DrawString(130, row_y, count_str, COLOR_WHITE, POND_DEEP, 2);
        ILI9488_DrawString(190, row_y, late_str, late_color, POND_DEEP, 2);
        ILI9488_DrawString(280, row_y, avg_str, avg_color, POND_DEEP, 2);

    }

    if (num_profiles == 0) {
        ILI9488_DrawStringCentered(0, 200, SCREEN_W, 40, "No users yet",
                                   POND_BLUE, POND_MID, 2);
    }

    int total_pages = (num_profiles + USERS_PER_PAGE - 1) / USERS_PER_PAGE;
    char page_str[12];
    snprintf(page_str, sizeof(page_str), "%d / %d", users_page + 1, total_pages);
    ILI9488_DrawStringCentered(0, 290, SCREEN_W, 24, page_str, POND_BLUE, POND_DEEP, 1);

    if (users_page > 0)
        draw_button(50,  284, 80, 30, "< Prev", POND_SURFACE);
    if (users_page + 1 < total_pages)
        draw_button(350, 284, 80, 30, "Next >", POND_SURFACE);
}

// Draw UI 
void Menu_Draw(void)
{
    /* Background */
    ILI9488_FillScreen(POND_DEEP);

    /* Left menu panel */
    ILI9488_FillRect(0, 0, MENU_W, SCREEN_H, POND_SURFACE);

	Pond_DrawMainBG();

    /* Left menu buttons */
    draw_button(BTN_X, BTN_OPEN_Y, BTN_W, BTN_H, "Open Lid",  COLOR_BLUE);
    draw_button(BTN_X, BTN_MEDS_Y, BTN_W, BTN_H, "Take Meds", COLOR_BLUE);
    draw_button(BTN_X, BTN_SET_Y, BTN_W, BTN_H, "Settings",  COLOR_BLUE);

    /* Divider line */
    ILI9488_DrawVLine(MENU_W, 0, SCREEN_H, COLOR_WHITE);

    /* Right panel — time display */
    Menu_UpdateClock();

    draw_next_med_time();

    draw_refill_status();

    /* View Stats button */
    draw_button(STATS_X, STATS_Y, STATS_W, STATS_H, "View Stats", COLOR_BLUE);

    draw_bmp_scaled(MENU_W / 2, 268, turtle_bmp, 1);

}


// Update clock
void Menu_UpdateClock(void)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);  // MUST call GetDate after GetTime

    char time_str[12];
    uint8_t hour = sTime.Hours;
    const char *ampm = (hour >= 12) ? "PM" : "AM";
    if (hour == 0)  hour = 12;
    else if (hour > 12) hour -= 12;

    snprintf(time_str, sizeof(time_str), "%2d:%02d %s", hour, sTime.Minutes, ampm);

    ILI9488_FillRect(MENU_W + 40, 40, 160, 30, POND_DEEP);
    ILI9488_DrawString(MENU_W + 40, 40, time_str, COLOR_WHITE, POND_DEEP, 3);

    draw_next_med_time();
    draw_refill_status();
}


/* ── Handle touch — check which button was tapped ────────────────── */
void Menu_HandleTouch(XPT2046_Touch *touch)
{
    if (!touch->touched) return;

    uint16_t tx = touch->x;
    uint16_t ty = touch->y;

    /* Left panel buttons */
    if (tx >= BTN_X && tx <= BTN_X + BTN_W) {
        if (ty >= BTN_OPEN_Y && ty <= BTN_OPEN_Y + BTN_H) {
            draw_button(BTN_X, BTN_OPEN_Y, BTN_W, BTN_H, "Open Lid", COLOR_GREEN);
            HAL_Delay(100);
            Screen_Verify_Draw();
            return;

        } else if (ty >= BTN_MEDS_Y && ty <= BTN_MEDS_Y + BTN_H) {
            draw_button(BTN_X, BTN_MEDS_Y, BTN_W, BTN_H, "Take Meds", COLOR_GREEN);
            HAL_Delay(100);
            Screen_Verify_Draw();
            return;

        } else if (ty >= BTN_SET_Y && ty <= BTN_SET_Y + BTN_H) {
            draw_button(BTN_X, BTN_SET_Y, BTN_W, BTN_H, "Settings", COLOR_GREEN);
            HAL_Delay(100);
            Screen_Settings_Draw();
            return;
        }
    }

    /* View Stats button */
    if (tx >= STATS_X && tx <= STATS_X + STATS_W &&
        ty >= STATS_Y  && ty <= STATS_Y  + STATS_H) {
        draw_button(STATS_X, STATS_Y, STATS_W, STATS_H, "View Stats", COLOR_GREEN);
        HAL_Delay(100);
       // Pond_DrawStatsBG();
        Screen_ViewStats_Draw();
        return;
    }
}

void Screen_Settings_Draw(void) 
{
    ILI9488_FillScreen(POND_DEEP);

	/* Back Button */
	draw_button(BACK_X, BACK_Y, BACK_W, BACK_H, "< Back", POND_SURFACE);

	/* Title */
	ILI9488_DrawStringCentered(0, 0, SCREEN_W, 80, "Settings", COLOR_WHITE, POND_MID, 2);

	/* Divider */
	ILI9488_DrawHLine(40, 90, SCREEN_W - 80, POND_SURFACE);

	// Settings Options
	draw_button(60, 110, 160, 50, "Add User", COLOR_BLUE);
	draw_button(260, 110, 160, 50, "Set Meds Time", COLOR_BLUE);
    draw_button(60, 190, 160, 50, "Change Voice", COLOR_BLUE);
	draw_button(260, 190, 160, 50, "Set Time", COLOR_BLUE);
}

void Screen_Settings_HandleTouch(XPT2046_Touch *touch)
{
    if (!touch->touched) return;

    uint16_t tx = touch->x;
    uint16_t ty = touch->y;

    if (tx >= BACK_X && tx <= BACK_X + BACK_W &&
        ty >= BACK_Y && ty <= BACK_Y + BACK_H) {
        draw_button(BACK_X, BACK_Y, BACK_W, BACK_H, "< Back", COLOR_GREEN);
        HAL_Delay(100);
        Menu_Draw();
        return;
    }

    /* Add User button */
    if (tx >= 60 && tx <= 220 &&
        ty >= 110 && ty <= 160) {
        draw_button(60, 110, 160, 50, "Add User", COLOR_GREEN);
        HAL_Delay(100);
        Screen_VerifyToAdd_Draw();
        return;
    }

    /* Set Meds Time button */
    if (tx >= 260 && tx <= 420 &&
        ty >= 110 && ty <= 160) {
        draw_button(260, 110, 160, 50, "Set Meds Time", COLOR_GREEN);
        HAL_Delay(100);
        buffer_idx = 0;
        memset(password_buffer, 0, sizeof(password_buffer));
        Screen_SetMeds_Draw();
        return;
    }

    /* Change Voice */
    if (tx >= 60 && tx <= 220 &&
        ty >= 190 && ty <= 240) {
        draw_button(60, 190, 160, 50, "Change Voice", COLOR_GREEN);
        HAL_Delay(100);
        Screen_ChangeVoice_Draw();
        return;
    }

    if (tx >= 260 && tx <= 420 &&
        ty >= 190 && ty <= 240) {
    	draw_button(260, 190, 160, 50, "Set Time", COLOR_GREEN);
    	HAL_Delay(100);
    	Screen_SetTime_Draw();
    	return;
    }
}

void Screen_ChangeVoice_Draw(void)
{
	ILI9488_FillScreen(POND_DEEP);

	/* Back Button */
	draw_button(BACK_X, BACK_Y, BACK_W, BACK_H, "< Back", POND_SURFACE);

	/* Title */
	ILI9488_DrawStringCentered(0, 0, SCREEN_W, 80, "Change Voice", COLOR_WHITE, POND_MID, 2);

	/* Divider */
	ILI9488_DrawHLine(40, 90, SCREEN_W - 80, POND_SURFACE);

	// Settings Options
	draw_button(60, 110, 160, 50, "Default", COLOR_BLUE);
	draw_button(260, 110, 160, 50, "Morgan Freeman", COLOR_BLUE);
	draw_button(60, 190, 160, 50, "Spongebob", COLOR_BLUE);
	draw_button(260, 190, 160, 50, "Mickey Mouse", COLOR_BLUE);
}

void Screen_ChangeVoice_HandleTouch(XPT2046_Touch *touch)
{
    if (!touch->touched) return;

    uint16_t tx = touch->x;
    uint16_t ty = touch->y;

    if (tx >= BACK_X && tx <= BACK_X + BACK_W &&
        ty >= BACK_Y && ty <= BACK_Y + BACK_H) {
        draw_button(BACK_X, BACK_Y, BACK_W, BACK_H, "< Back", COLOR_GREEN);
        HAL_Delay(100);
        Menu_Draw();
        return;
    }

    /* Default button */
    if (tx >= 60 && tx <= 220 &&
        ty >= 110 && ty <= 160) {
        draw_button(60, 110, 160, 50, "Default", COLOR_GREEN);
        HAL_Delay(100);
        ILI9488_FillRect(60, 110, 360, 130, POND_DEEP);
        Screen_SetMessage("You selected the default voice!");
        selected_voice = 0;
        HAL_Delay(1000);
        Menu_Draw();
        return;
    }

    /* Morgan Freeman button */
    if (tx >= 260 && tx <= 420 &&
        ty >= 110 && ty <= 160) {
        draw_button(260, 110, 160, 50, "Morgan Freeman", COLOR_GREEN);
        HAL_Delay(100);
        ILI9488_FillRect(60, 110, 360, 130, POND_DEEP);
        Screen_SetMessage("You selected the Morgan Freeman voice!");
        selected_voice = 1;
        HAL_Delay(1000);
        Menu_Draw();
        return;
    }

    /* Change Voice */
    if (tx >= 60 && tx <= 220 &&
    	ty >= 190 && ty <= 240) {
        draw_button(60, 190, 160, 50, "Spongebob", COLOR_GREEN);
        HAL_Delay(100);
        ILI9488_FillRect(60, 110, 360, 130, POND_DEEP);
        Screen_SetMessage("You selected the Spongebob voice!");
        selected_voice = 2;
        HAL_Delay(1000);
        Menu_Draw();
        return;
    }

    if (tx >= 260 && tx <= 420 &&
        ty >= 190 && ty <= 240) {
        draw_button(260, 190, 160, 50, "Mickey Mouse", COLOR_GREEN);
        HAL_Delay(1000);
        ILI9488_FillRect(60, 110, 360, 130, POND_DEEP);
        Screen_SetMessage("You selected the Mickey Mouse voice!");
        selected_voice = 3;
        HAL_Delay(1000);
        Menu_Draw();
        return;
   }
}

/* ── Set Time screen ───────────────────────────────────────── */
void Screen_SetTime_Draw(void)
{
    ILI9488_FillScreen(POND_DEEP);

    draw_button(BACK_X, BACK_Y, BACK_W, BACK_H, "< Back", POND_SURFACE);

    ILI9488_DrawStringCentered(0, 0, SCREEN_W, 80, "Set Current Time", COLOR_WHITE, POND_MID, 2);
    ILI9488_DrawHLine(40, 90, SCREEN_W - 80, POND_SURFACE);

    char buf[48];
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    snprintf(buf, sizeof(buf), "Old time: %02u:%02u", (unsigned)sTime.Hours, (unsigned)sTime.Minutes);
    ILI9488_DrawString(60, 120, buf, COLOR_WHITE, POND_DEEP, 2);

    ILI9488_DrawString(60, 170, "Current time:", COLOR_WHITE, POND_DEEP, 2);
    Screen_SetTime_UpdateEntry();

// # is to save and * is the restart 
    ILI9488_DrawString(60, 232, "Enter HHMM", COLOR_WHITE, POND_DEEP, 2);
    ILI9488_DrawString(60, 260, "# Save    * Clear", COLOR_YELLOW, POND_DEEP, 2);
}

void Screen_SetTime_UpdateEntry(void)
{
    char entry[8];

    format_meds_entry(entry, sizeof(entry));

    ILI9488_FillRect(230, 164, 170, 32, POND_DEEP);
    ILI9488_DrawString(230, 170, entry, COLOR_GREEN, POND_DEEP, 2);
}

void Screen_SetTime_HandleTouch(XPT2046_Touch *touch)
{
    if (!touch->touched) return;

    uint16_t tx = touch->x;
    uint16_t ty = touch->y;

    /* Back button */
    if (tx >= BACK_X && tx <= BACK_X + BACK_W &&
        ty >= BACK_Y && ty <= BACK_Y + BACK_H) {
        draw_button(BACK_X, BACK_Y, BACK_W, BACK_H, "< Back", COLOR_GREEN);
        HAL_Delay(100);
        Screen_Settings_Draw();
        return;
    }
}

// Verify screen 
void Screen_Verify_Draw(void)
{
	ILI9488_FillScreen(POND_DEEP);

	/* Back Button */
	draw_button(BACK_X, BACK_Y, BACK_W, BACK_H, "< Back", POND_SURFACE);

	/* Title */
	ILI9488_DrawStringCentered(0, 0, SCREEN_W, 80, "Verify Access", COLOR_WHITE, POND_MID, 2);

	/* Divider */
	ILI9488_DrawHLine(40, 90, SCREEN_W - 80, POND_SURFACE);

	// Two side-by-side buttons
	draw_button(60,  130, 160, 60, "Fingerprint", COLOR_BLUE);
	draw_button(260, 130, 160, 60, "Keypad",      COLOR_BLUE);
}

void Screen_Verify_HandleTouch(XPT2046_Touch *touch)
{
    if (!touch->touched) return;

    uint16_t tx = touch->x;
    uint16_t ty = touch->y;

    //Back button — return to main menu 
    if (tx >= BACK_X && tx <= BACK_X + BACK_W &&
        ty >= BACK_Y && ty <= BACK_Y + BACK_H) {
        draw_button(BACK_X, BACK_Y, BACK_W, BACK_H, "< Back", COLOR_GREEN);
        HAL_Delay(100);
        Menu_Draw();
        return;
    }

    /* Fingerprint button */
    if (tx >= 60 && tx <= 220 &&
        ty >= 130 && ty <= 190) {
        draw_button(60, 130, 160, 60, "Fingerprint", COLOR_GREEN);
        HAL_Delay(100);
        Screen_Fingerprint_Draw();
        return;
    }

    /* Keypad button */
    if (tx >= 260 && tx <= 420 &&
        ty >= 130 && ty <= 190) {
        draw_button(260, 130, 160, 60, "Keypad", COLOR_GREEN);
        HAL_Delay(100);
        Screen_Keypad_Draw();
        return;
    }
}

void Screen_Fingerprint_Draw(void)
{
	ILI9488_FillScreen(POND_DEEP);

	/* Back Button */
	draw_button(BACK_X, BACK_Y, BACK_W, BACK_H, "< Back", POND_SURFACE);

	/* Title */
	ILI9488_DrawStringCentered(0, 0, SCREEN_W, 80, "Verify by Fingerprint", COLOR_WHITE, POND_MID, 2);

	/* Divider */
	ILI9488_DrawHLine(40, 90, SCREEN_W - 80, POND_SURFACE);

	/* Messages */
	ILI9488_DrawStringCentered(0, 110, SCREEN_W, 40, "Please place your finger...", COLOR_WHITE, POND_MID, 2);
}

void Screen_SetEnrollStep(const char *msg, const unsigned char *bmp)
{
    ILI9488_FillRect(0, 95, SCREEN_W, SCREEN_H - 95, POND_DEEP);
    ILI9488_DrawStringCentered(0, 100, SCREEN_W, 30, msg, COLOR_WHITE, POND_DEEP, 2);
    if (bmp) {
        draw_bmp_scaled(SCREEN_W / 2, 210, bmp, 1);
    }
}

void Screen_Fingerprint_SetStatus(uint8_t success, uint8_t open,
                                  const unsigned char *user_bmp)
{
    ILI9488_FillRect(0, 90, SCREEN_W, SCREEN_H - 90, POND_DEEP);

    if (success) {
        ILI9488_DrawStringCentered(0, 95, SCREEN_W, 30,
            "Access Granted!", COLOR_GREEN, POND_DEEP, 2);
        	draw_bmp_scaled(SCREEN_W / 2, 195, user_bmp, 2);
        if (open == 1) {
            ILI9488_DrawStringCentered(0, 270, SCREEN_W, 30,
                "Opening Lid...", COLOR_GREEN, POND_DEEP, 2);
        } else if (open == 0) {
            ILI9488_DrawStringCentered(0, 270, SCREEN_W, 30,
                "Releasing Medication...", COLOR_GREEN, POND_DEEP, 2);
        } else if (open == 2) {
            ILI9488_DrawStringCentered(0, 270, SCREEN_W, 30,
                "Ready to Add User...", COLOR_GREEN, POND_DEEP, 2);
            HAL_Delay(150);
            Screen_AddUser_Draw();
            return;
        }
    } else {
    	  ILI9488_DrawStringCentered(0, 95, SCREEN_W, 30,
    	            "Access Denied, try again!", COLOR_RED, POND_DEEP, 2);
        draw_bmp_scaled(SCREEN_W / 2, 195, user_bmp, 2);
        ILI9488_FillRect(0, 90, SCREEN_W, SCREEN_H - 90, POND_DEEP);

    }
}


void Screen_FK_HandleTouch(XPT2046_Touch *touch)
{
    if (!touch->touched) return;

    uint16_t tx = touch->x;
    uint16_t ty = touch->y;

    /* Back button — return to verify screen */
    if (tx >= BACK_X && tx <= BACK_X + BACK_W &&
        ty >= BACK_Y && ty <= BACK_Y + BACK_H) {
        draw_button(BACK_X, BACK_Y, BACK_W, BACK_H, "< Back", COLOR_GREEN);
        HAL_Delay(100);
        Screen_Verify_Draw();
        return;
    }
}

void Screen_Keypad_Draw(void)
{
	ILI9488_FillScreen(POND_DEEP);

	/* Back Button */
	draw_button(BACK_X, BACK_Y, BACK_W, BACK_H, "< Back", POND_SURFACE);

	/* Title */
	ILI9488_DrawStringCentered(0, 0, SCREEN_W, 80,
                               adding_passcode ? "Create Passcode" : "Verify by Keypad",
                               COLOR_WHITE, POND_MID, 2);

	/* Divider */
	ILI9488_DrawHLine(40, 90, SCREEN_W - 80, POND_SURFACE);

	/* Messages */
    if (adding_passcode) {
        ILI9488_DrawStringCentered(0, 110, SCREEN_W, 40, "Type 4 digits", COLOR_WHITE, POND_MID, 2);
        ILI9488_DrawStringCentered(0, 155, SCREEN_W, 40, "then press #", COLOR_WHITE, POND_MID, 2);
    } else {
	    ILI9488_DrawStringCentered(0, 110, SCREEN_W, 40, "Please type in", COLOR_WHITE, POND_MID, 2);
	    ILI9488_DrawStringCentered(0, 155, SCREEN_W, 40, "your code on the keypad...", COLOR_WHITE, POND_MID, 2);
    }
    ILI9488_FillRect(110, 210, 260, 36, POND_DEEP);
    ILI9488_DrawHLine(110, 210, 260, POND_SURFACE);
    ILI9488_DrawHLine(110, 246, 260, POND_SURFACE);
    ILI9488_DrawVLine(110, 210, 36, POND_SURFACE);
    ILI9488_DrawVLine(370, 210, 36, POND_SURFACE);
    Screen_Keypad_UpdateEntry(1);
}

void Screen_VerifyToAdd_Draw(void)
{
	ILI9488_FillScreen(POND_DEEP);

	/* Back Button */
	draw_button(BACK_X, BACK_Y, BACK_W, BACK_H, "< Back", POND_SURFACE);

	/* Title */
	ILI9488_DrawStringCentered(0, 0, SCREEN_W, 80, "Verify by Keypad", COLOR_WHITE, POND_MID, 2);

	/* Divider */
	ILI9488_DrawHLine(40, 90, SCREEN_W - 80, POND_SURFACE);

	/* Messages */
	ILI9488_DrawStringCentered(0, 110, SCREEN_W, 40, "Please type in administration", COLOR_WHITE, POND_MID, 2);
	ILI9488_DrawStringCentered(0, 155, SCREEN_W, 40, "code on the keypad...", COLOR_WHITE, POND_MID, 2);
    ILI9488_FillRect(110, 210, 260, 36, POND_DEEP);
    ILI9488_DrawHLine(110, 210, 260, POND_SURFACE);
    ILI9488_DrawHLine(110, 246, 260, POND_SURFACE);
    ILI9488_DrawVLine(110, 210, 36, POND_SURFACE);
    ILI9488_DrawVLine(370, 210, 36, POND_SURFACE);
    Screen_Keypad_UpdateEntry(1);
}

void Screen_Keypad_UpdateEntry(uint8_t masked)
{
    char entry[17];

    format_keypad_entry(entry, sizeof(entry), masked);

    ILI9488_FillRect(118, 218, 244, 20, POND_DEEP);
    if (buffer_idx > 0) {
        ILI9488_DrawStringCentered(110, 214, 260, 28, entry, COLOR_GREEN, POND_MID, 2);
    } else {
        ILI9488_DrawStringCentered(110, 214, 260, 28, "----", POND_SURFACE, POND_MID, 2);
    }
}

void Screen_AddUser_Draw(void)
{
	ILI9488_FillScreen(POND_DEEP);

    /* Back Button */
    draw_button(BACK_X, BACK_Y, BACK_W, BACK_H, "< Back", POND_SURFACE);

	/* Title */
	ILI9488_DrawStringCentered(0, 0, SCREEN_W, 80, "New User", COLOR_WHITE, POND_MID, 2);

	/* Divider */
	ILI9488_DrawHLine(40, 90, SCREEN_W - 80, POND_SURFACE);
}

// Set med times
void Screen_SetMeds_Draw(void)
{
    ILI9488_FillScreen(POND_DEEP);

    /* Back Button */
    draw_button(BACK_X, BACK_Y, BACK_W, BACK_H, "< Back", POND_SURFACE);

    /* Title */
    ILI9488_DrawStringCentered(0, 0, SCREEN_W, 80, "Set Medication Time", COLOR_WHITE, POND_MID, 2);
    ILI9488_DrawHLine(40, 90, SCREEN_W - 80, POND_SURFACE);

    /* Current saved time */
    char buf[48];
    snprintf(buf, sizeof(buf), "Current time: %02u:%02u", (unsigned)user_hr, (unsigned)user_min);
    ILI9488_DrawString(60, 120, buf, COLOR_WHITE, POND_DEEP, 2);

    ILI9488_DrawString(60, 170, "New time:", COLOR_WHITE, POND_DEEP, 2);
    Screen_SetMeds_UpdateEntry();

    ILI9488_DrawString(60, 232, "Enter HHMM", COLOR_WHITE, POND_DEEP, 2);
    ILI9488_DrawString(60, 260, "# Save    * Clear", COLOR_YELLOW, POND_DEEP, 2);
}

void Screen_SetMeds_UpdateEntry(void)
{
    char entry[8];

    format_meds_entry(entry, sizeof(entry));

    ILI9488_FillRect(190, 164, 170, 32, POND_DEEP);
    ILI9488_DrawString(190, 170, entry, COLOR_GREEN, POND_DEEP, 2);
}

void Screen_SetMeds_HandleTouch(XPT2046_Touch *touch)
{
    if (!touch->touched) return;

    uint16_t tx = touch->x;
    uint16_t ty = touch->y;

    /* Back button */
    if (tx >= BACK_X && tx <= BACK_X + BACK_W &&
        ty >= BACK_Y && ty <= BACK_Y + BACK_H) {
        draw_button(BACK_X, BACK_Y, BACK_W, BACK_H, "< Back", COLOR_GREEN);
        HAL_Delay(100);
        Screen_Settings_Draw();
        return;
    }
}

void Screen_SetMessage(const char *msg) {
	ILI9488_FillRect(0, 110, SCREEN_W, 120, POND_DEEP);
	ILI9488_DrawStringCentered(0, 145, SCREEN_W, 40, msg, COLOR_WHITE, POND_MID, 2);
}

void Screen_AddVerify_HandleTouch(XPT2046_Touch *touch)
{
    if (!touch->touched) return;

    uint16_t tx = touch->x;
    uint16_t ty = touch->y;

    if (tx >= BACK_X && tx <= BACK_X + BACK_W &&
        ty >= BACK_Y && ty <= BACK_Y + BACK_H) {
        draw_button(BACK_X, BACK_Y, BACK_W, BACK_H, "< Back", COLOR_GREEN);
        HAL_Delay(150);
        Screen_Settings_Draw();
    }
}

void Screen_KeypadAdd_HandleTouch(XPT2046_Touch *touch)
{
    if (!touch->touched) return;

    uint16_t tx = touch->x;
    uint16_t ty = touch->y;

    if (tx >= BACK_X && tx <= BACK_X + BACK_W &&
        ty >= BACK_Y && ty <= BACK_Y + BACK_H) {
        draw_button(BACK_X, BACK_Y, BACK_W, BACK_H, "< Back", COLOR_GREEN);
        adding_passcode = 0;
        pending_profile_idx = -1;
        buffer_idx = 0;
        memset(password_buffer, 0, sizeof(password_buffer));
        HAL_Delay(100);
        Screen_Settings_Draw();
    }
}

void Screen_ViewStats_Draw(void)
{
    ILI9488_FillScreen(POND_DEEP);

    /* Back Button */
    draw_button(BACK_X, BACK_Y, BACK_W, BACK_H, "< Back", POND_SURFACE);

    /* Title */
    ILI9488_DrawStringCentered(0, 0, SCREEN_W, 80, "Access Stats", COLOR_WHITE, POND_MID, 2);

    /* Divider */
    ILI9488_DrawHLine(40, 90, SCREEN_W - 80, POND_SURFACE);

    draw_stats_tabs();
    if (stats_page == 0) {
        draw_system_stats();
    } else if (stats_page == 1) {
    	draw_history_stats();
    } else {
        draw_user_stats();
    }
}

void Screen_ViewStats_HandleTouch(XPT2046_Touch *touch)
{
    if (!touch->touched) return;

    uint16_t tx = touch->x;
    uint16_t ty = touch->y;

    if (tx >= BACK_X && tx <= BACK_X + BACK_W &&
        ty >= BACK_Y && ty <= BACK_Y + BACK_H) {
        draw_button(BACK_X, BACK_Y, BACK_W, BACK_H, "< Back", COLOR_GREEN);
        HAL_Delay(100);
        Menu_Draw();
        return;
    }

    if (tx >= 80 && tx <= 180 && ty >= 98 && ty <= 132) {
        stats_page = 0;
        users_page = 0;
        Screen_ViewStats_Draw();
        return;
    }

    if (tx >= 190 && tx <= 290 && ty >= 98 && ty <= 132) {
        stats_page = 1;
        users_page = 0;
        Screen_ViewStats_Draw();
        return;
    }

    if (tx >= 300 && tx <= 400 && ty >= 98 && ty <= 132) {
        stats_page = 2;
        Screen_ViewStats_Draw();
        return;
    }

    if (stats_page == 2) {
        int total_pages = (num_profiles + USERS_PER_PAGE - 1) / USERS_PER_PAGE;

        if (tx >= 50 && tx <= 130 && ty >= 284 && ty <= 310) {
            if (users_page > 0) {
                users_page--;
                Screen_ViewStats_Draw();
            }
            return;
        }
        if (tx >= 350 && tx <= 430 && ty >= 284 && ty <= 310) {
            if (users_page + 1 < total_pages) {
                users_page++;
                Screen_ViewStats_Draw();
            }
            return;
        }
    }
}

void Screen_RefillPrompt_Draw(void)
{
    ILI9488_FillScreen(POND_DEEP);

    ILI9488_DrawStringCentered(0, 55, SCREEN_W, 40, "Did you refill medication?", COLOR_WHITE, POND_MID, 2);

    {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d pills remaining", pills_remaining);
        ILI9488_DrawStringCentered(0, 100, SCREEN_W, 32, buf,
                                   (pills_remaining <= 3) ? COLOR_YELLOW : COLOR_WHITE,
                                   POND_DEEP, 2);
    }

    draw_button(70, 170, 140, 60, "Yes", COLOR_GREEN);
    draw_button(270, 170, 140, 60, "No", COLOR_BLUE);
    ILI9488_DrawStringCentered(0, 250, SCREEN_W, 28, "Tap Yes to reset to 13 pills", POND_SURFACE, POND_MID, 1);
}

void Screen_RefillPrompt_HandleTouch(XPT2046_Touch *touch)
{
    if (!touch->touched) return;

    uint16_t tx = touch->x;
    uint16_t ty = touch->y;

    if (tx >= 70 && tx <= 210 && ty >= 170 && ty <= 230) {
        draw_button(70, 170, 140, 60, "Yes", COLOR_GREEN);
        pills_remaining = 13;
        dispenses_since_refill = 0;
        HAL_Delay(100);
        return;
    }

    if (tx >= 270 && tx <= 410 && ty >= 170 && ty <= 230) {
        draw_button(270, 170, 140, 60, "No", COLOR_GREEN);
        HAL_Delay(100);
        return;
    }
}

void Screen_NameEntry_Draw(void)
{
    ILI9488_FillScreen(POND_DEEP);

    /* Back Button */
    draw_button(BACK_X, BACK_Y, BACK_W, BACK_H, "< Back", POND_SURFACE);

    /* Title */
    ILI9488_DrawStringCentered(0, 0, SCREEN_W, 80, "Enter Name for New User", COLOR_WHITE, POND_MID, 2);

    /* Divider */
    ILI9488_DrawHLine(40, 90, SCREEN_W - 80, POND_SURFACE);

    draw_name_status("Tap letters, then Save", COLOR_WHITE);
    ILI9488_FillRect(40, 138, 400, 34, POND_DEEP);
    ILI9488_DrawHLine(40, 138, 400, POND_SURFACE);
    ILI9488_DrawHLine(40, 172, 400, POND_SURFACE);
    ILI9488_DrawVLine(40, 138, 34, POND_SURFACE);
    ILI9488_DrawVLine(440, 138, 34, POND_SURFACE);
    Screen_NameEntry_Update();

    const char *row1[] = {"A","B","C","D","E","F","G"};
    const char *row2[] = {"H","I","J","K","L","M","N"};
    const char *row3[] = {"O","P","Q","R","S","T","U"};
    const char *row4[] = {"V","W","X","Y","Z"};
    uint16_t key_w = 52;
    uint16_t key_h = 28;
    uint16_t gap = 8;
    uint16_t start_x = 32;

    for (int i = 0; i < 7; i++) {
        draw_name_key(start_x + i * (key_w + gap), 184, key_w, key_h, row1[i], COLOR_BLUE);
        draw_name_key(start_x + i * (key_w + gap), 220, key_w, key_h, row2[i], COLOR_BLUE);
        draw_name_key(start_x + i * (key_w + gap), 256, key_w, key_h, row3[i], COLOR_BLUE);
    }

    for (int i = 0; i < 5; i++) {
        draw_name_key(92 + i * (key_w + gap), 292, key_w, key_h, row4[i], COLOR_BLUE);
    }

    draw_name_key(32, 292, 52, key_h, "<", POND_SURFACE);
    draw_name_key(400, 292, 48, key_h, "OK", COLOR_GREEN);
    draw_name_key(328, 292, 64, key_h, "Space", POND_SURFACE);
}

void Screen_NameEntry_Update(void)
{
    char display[32];

    if (name_buffer_idx > 0) {
        snprintf(display, sizeof(display), "%s", name_buffer);
    } else {
        snprintf(display, sizeof(display), "Name...");
    }

    ILI9488_FillRect(46, 144, 388, 22, POND_DEEP);
    ILI9488_DrawString(52, 146, display,
                       (name_buffer_idx > 0) ? COLOR_GREEN : POND_SURFACE,
                       POND_DEEP, 2);
}

void Screen_NameEntry_HandleTouch(XPT2046_Touch *touch)
{
    static uint32_t last_name_touch_tick = 0;

    if (!touch->touched) return;
    if (HAL_GetTick() - last_name_touch_tick < 180) return;

    uint16_t tx = touch->x;
    uint16_t ty = touch->y;

    if (tx >= BACK_X && tx <= BACK_X + BACK_W &&
        ty >= BACK_Y && ty <= BACK_Y + BACK_H) {
        draw_button(BACK_X, BACK_Y, BACK_W, BACK_H, "< Back", COLOR_GREEN);
        HAL_Delay(100);
        adding_name = 0;
        pending_profile_idx = -1;
        name_buffer_idx = 0;
        memset(name_buffer, 0, sizeof(name_buffer));
        Screen_Settings_Draw();
        return;
    }

    if (ty >= 184 && ty <= 212) {
        static const char row1[] = "ABCDEFG";
        int idx = (tx - 32) / 60;
        if (tx >= 32 && idx >= 0 && idx < 7 && (32 + idx * 60 + 52) >= tx) {
            if (name_buffer_idx < (int)sizeof(name_buffer) - 1) {
                name_buffer[name_buffer_idx++] = row1[idx];
                name_buffer[name_buffer_idx] = '\0';
                draw_name_status("Tap letters, then Save", COLOR_WHITE);
                Screen_NameEntry_Update();
                last_name_touch_tick = HAL_GetTick();
            }
            return;
        }
    }

    if (ty >= 220 && ty <= 248) {
        static const char row2[] = "HIJKLMN";
        int idx = (tx - 32) / 60;
        if (tx >= 32 && idx >= 0 && idx < 7 && (32 + idx * 60 + 52) >= tx) {
            if (name_buffer_idx < (int)sizeof(name_buffer) - 1) {
                name_buffer[name_buffer_idx++] = row2[idx];
                name_buffer[name_buffer_idx] = '\0';
                draw_name_status("Tap letters, then Save", COLOR_WHITE);
                Screen_NameEntry_Update();
                last_name_touch_tick = HAL_GetTick();
            }
            return;
        }
    }

    if (ty >= 256 && ty <= 284) {
        static const char row3[] = "OPQRSTU";
        int idx = (tx - 32) / 60;
        if (tx >= 32 && idx >= 0 && idx < 7 && (32 + idx * 60 + 52) >= tx) {
            if (name_buffer_idx < (int)sizeof(name_buffer) - 1) {
                name_buffer[name_buffer_idx++] = row3[idx];
                name_buffer[name_buffer_idx] = '\0';
                draw_name_status("Tap letters, then Save", COLOR_WHITE);
                Screen_NameEntry_Update();
                last_name_touch_tick = HAL_GetTick();
            }
            return;
        }
    }

    if (ty >= 292 && ty <= 320) {
        if (tx >= 32 && tx <= 84) {
            if (name_buffer_idx > 0) {
                name_buffer[--name_buffer_idx] = '\0';
                draw_name_status("Tap letters, then Save", COLOR_WHITE);
                Screen_NameEntry_Update();
                last_name_touch_tick = HAL_GetTick();
            }
            return;
        }

        if (tx >= 328 && tx <= 392) {
            if (name_buffer_idx < (int)sizeof(name_buffer) - 1) {
                name_buffer[name_buffer_idx++] = ' ';
                name_buffer[name_buffer_idx] = '\0';
                draw_name_status("Tap letters, then Save", COLOR_WHITE);
                Screen_NameEntry_Update();
                last_name_touch_tick = HAL_GetTick();
            }
            return;
        }

        if (tx >= 400 && tx <= 448) {
            if (pending_profile_idx >= 0 && pending_profile_idx < MAX_USERS && name_buffer_idx > 0) {
                for (int i = 0; i < num_profiles; i++) {
                    if (strcmp(profiles[i].name, name_buffer) == 0) {
                        draw_name_status("Name unavailable, try again", COLOR_RED);
                        last_name_touch_tick = HAL_GetTick();
                        return;
                    }
                }
                strncpy(profiles[pending_profile_idx].name, name_buffer,
                        sizeof(profiles[pending_profile_idx].name) - 1);
                profiles[pending_profile_idx].name[sizeof(profiles[pending_profile_idx].name) - 1] = '\0';
                profiles[pending_profile_idx].access_count = 0;
                num_profiles++;
                pending_profile_idx = -1;
                adding_name = 0;
                name_buffer_idx = 0;
                memset(name_buffer, 0, sizeof(name_buffer));
                ILI9488_FillScreen(POND_DEEP);
                ILI9488_DrawStringCentered(0, 130, SCREEN_W, 40, "User saved", COLOR_GREEN, POND_MID, 2);
                HAL_Delay(600);
                Screen_Settings_Draw();
            }
            return;
        }

        if (tx >= 92 && tx <= 396) {
            static const char row4[] = "VWXYZ";
            int idx = (tx - 92) / 60;
            if (idx >= 0 && idx < 5 && (92 + idx * 60 + 52) >= tx) {
                if (name_buffer_idx < (int)sizeof(name_buffer) - 1) {
                    name_buffer[name_buffer_idx++] = row4[idx];
                    name_buffer[name_buffer_idx] = '\0';
                    draw_name_status("Tap letters, then Save", COLOR_WHITE);
                    Screen_NameEntry_Update();
                    last_name_touch_tick = HAL_GetTick();
                }
            }
        }
    }
}
