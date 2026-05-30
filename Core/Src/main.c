/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"
#include "pond_bg.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "fingerprint.h"
#include "stdio.h"
#include "string.h"
#include "ili9488.h"
#include "xpt2046.h"
#include "menu.h"
#include <stdlib.h>
#include <stdarg.h> 
#include "sd_functions.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define NUM_POS 14
extern unsigned char user0_bmp[];
extern unsigned int user0_bmp_len;
extern unsigned char turtle_bmp[];
extern unsigned int  turtle_bmp_len;
extern unsigned char User_Kevin_bmp[];
extern unsigned int  kevin_bmp_len;
extern unsigned char User_Elena_bmp[];
extern unsigned int  elena_bmp_len;
extern unsigned char User_Nithya_bmp[];
extern unsigned int  nithya_bmp_len;
extern unsigned char User_George_bmp[];
extern unsigned int  george_bmp_len;
extern unsigned int  fingerprint1_bmp_len;
extern unsigned int  fingerprint2_bmp_len;
extern unsigned int  fingerprint3_bmp_len;
extern unsigned int  fingerprint4_bmp_len;
extern unsigned char fingerprint1_bmp[];
extern unsigned char fingerprint2_bmp[];
extern unsigned char fingerprint3_bmp[];
extern unsigned char fingerprint4_bmp[];
extern unsigned int  happyturtle_bmp_len;
extern unsigned char happyturtle_bmp[];
extern unsigned int  menu_bmp_len;
extern unsigned char menu_bmp[];
extern unsigned char sadturtle_bmp[];
extern unsigned int  sadturtle_bmp_len;

/* USER CODE END PD */

/* USER CODE BEGIN PM */

/* USER CODE END PM */

// Private Variables
DAC_HandleTypeDef hdac1;
DMA_HandleTypeDef hdma_dac1_ch1;

UART_HandleTypeDef hlpuart1;
UART_HandleTypeDef huart4;
UART_HandleTypeDef huart1;

RTC_HandleTypeDef hrtc;

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;
SPI_HandleTypeDef hspi3;
DMA_HandleTypeDef hdma_spi1_tx;
DMA_HandleTypeDef hdma_spi3_rx;
DMA_HandleTypeDef hdma_spi3_tx;

TIM_HandleTypeDef htim4;
TIM_HandleTypeDef htim6;

/* USER CODE BEGIN PV */

uint32_t user_hr = 00;
uint32_t user_min = 00;

int pills_remaining = 13;
int dispenses_since_refill = 0;
int last_dispense_valid = 0;
uint8_t last_dispense_hour = 0;
uint8_t last_dispense_minute = 0;
int dose_overdue = 0;
uint32_t missed_count = 0;
DoseStatus today_dose_status = NONE;
DoseHistoryEntry dose_history[7];
RTC_DateTypeDef last_schedule_date = {0};

uint32_t lock_timer = 0;
int lock_active = 0;
uint32_t last_key_time = 0;
#define DEBOUNCE_MS 50
int adding_passcode = 0;
int adding_name = 0;
int pending_profile_idx = -1;

int turnOffAlarm = 0;
int meds_taken = 0;
const unsigned char *happy = NULL;

// Fingerprint code //
Fingerprint_Handle myFinger;

typedef enum {
    MODE_VERIFY,
    MODE_ENROLL
} SystemMode;

SystemMode currentMode = MODE_VERIFY;
uint16_t nextID = 1;
uint32_t lastHeartbeat = 0;
int button_pressed;

// Screen state — promoted to global so all blocks share it
ScreenState cur_screen = SCREEN_MAIN;

// Keypad code //
char password_buffer[16];
int buffer_idx = 0;
char name_buffer[24];
int name_buffer_idx = 0;


// profiles initializing
UserProfile profiles[MAX_USERS];
int num_profiles = 0;
uint16_t next_fp_id = 1; 

uint32_t entry_start_time = 0;
GPIO_TypeDef* rowPorts[] = {GPIOA,GPIOB,GPIOB,GPIOE};
uint16_t rowPins[] = {GPIO_PIN_1,GPIO_PIN_1,GPIO_PIN_2,GPIO_PIN_4};
GPIO_TypeDef* colPorts[] = {GPIOE,GPIOE,GPIOB,GPIOA};
uint16_t colPins[] = {GPIO_PIN_5,GPIO_PIN_6,GPIO_PIN_6,GPIO_PIN_0};
char keys[4][4] = {
        {'1','2','3','A'},
        {'4','5','6','B'},
        {'7','8','9','C'},
        {'*','0','#','D'},
};

// Motor code //
int steps[NUM_POS] = {
   229,229,229,229,228,228,228,229,229,229,229,228,228,228
};


volatile int steps_remaining = 0;
volatile int move_done = 1;
int current_pos = 0;
uint32_t last_move_time = 0;
uint32_t move_interval = 30000;

// Audio state
static uint8_t  audio_playing    = 0;
static uint32_t audio_start_tick = 0;
#define AUDIO_TIMEOUT_MS 60000  

// Move function
void move_steps_pwm(int num_steps)
{
    steps_remaining = num_steps;
    move_done = 0;

    __HAL_TIM_SET_COUNTER(&htim4, 0);

    HAL_TIM_Base_Start_IT(&htim4);
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
}

int test_auth = 0;

int auth_received(void) {
    if (test_auth == 1) {
        test_auth = 0;
        return 1;
    }
    return 0;
}

static int minutes_of_day(int hour, int minute)
{
    return hour * 60 + minute;
}

static int get_active_schedule_start_minute(int current_minutes)
{
    int scheduled_minutes = minutes_of_day((int)user_hr, (int)user_min);
    return (current_minutes >= scheduled_minutes) ? scheduled_minutes : -1;
}

static int medication_due_now(int current_hour, int current_minute)
{
    int current_minutes = minutes_of_day(current_hour, current_minute);
    return ((meds_taken == 0) && (get_active_schedule_start_minute(current_minutes) >= 0));
}

static int medication_overdue_now(int current_hour, int current_minute)
{
    int current_minutes = minutes_of_day(current_hour, current_minute);
    int active_start = get_active_schedule_start_minute(current_minutes);

    return (meds_taken == 0) && (active_start >= 0) && (current_minutes > active_start);
}

static uint32_t medication_late_minutes(int current_hour, int current_minute)
{
    int current_minutes = minutes_of_day(current_hour, current_minute);
    int active_start = get_active_schedule_start_minute(current_minutes);

    if ((active_start < 0) || (current_minutes <= active_start)) {
        return 0;
    }

    return (uint32_t)(current_minutes - active_start);
}

void Audio_Stop(void)
{
    if (audio_playing) {
        HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1);
        HAL_TIM_Base_Stop(&htim6);
        audio_playing = 0;
    }
}

static void push_dose_history(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, DoseStatus status)
{
    if (status == NONE) {
        return;
    }

    for (int i = 0; i < 6; i++) {
        dose_history[i] = dose_history[i + 1];
    }

    dose_history[6].year = year;
    dose_history[6].month = month;
    dose_history[6].day = day;
    dose_history[6].hour = hour;
    dose_history[6].minute = minute;
    dose_history[6].status = status;
}

static void reset_daily_dose_state(void)
{
    turnOffAlarm = 0;
    meds_taken = 0;
    dose_overdue = 0;
    today_dose_status = NONE;
}

static void rollover_daily_dose_if_needed(const RTC_DateTypeDef *current_date)
{
    if (last_schedule_date.Date == 0) {
        last_schedule_date = *current_date;
        return;
    }

    if (current_date->Year == last_schedule_date.Year &&
        current_date->Month == last_schedule_date.Month &&
        current_date->Date == last_schedule_date.Date) {
        return;
    }

    DoseStatus final_status = today_dose_status;
    if (!meds_taken) {
    	final_status = MISSED;
        missed_count++;
    }

    reset_daily_dose_state();
    last_schedule_date = *current_date;
}

static void record_dispense(uint8_t hour, uint8_t minute)
{
	int was_late = medication_overdue_now((int)hour, (int)minute);
	RTC_DateTypeDef sDate = {0};
	HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    turnOffAlarm = 1;
    meds_taken = 1;
    today_dose_status = was_late ? LATE : ON_TIME;
    push_dose_history(2000u + sDate.Year,
                          sDate.Month,
                          sDate.Date,
						  hour,
						  minute,
                          today_dose_status);
    Audio_Stop();
    if (pills_remaining > 0) {
        pills_remaining--;
    }
    dispenses_since_refill++;
    last_dispense_hour = hour;
    last_dispense_minute = minute;
    last_dispense_valid = 1;
}

// Timer interrupt // 
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM4) {
        if (steps_remaining > 0) {
            steps_remaining--;
        }
        if (steps_remaining == 0) {
            HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_4);
            HAL_TIM_Base_Stop_IT(&htim4);
            move_done = 1;
        }
    }
}

typedef enum {
    MOTOR_IDLE,
    MOTOR_MOVING,
    MOTOR_WAITING
} MotorState;

MotorState motor_state = MOTOR_IDLE;
uint32_t motor_timer = 0;






// Different voices 

typedef enum {
    AUDIO_PILL_REMINDER = 0,
    AUDIO_ACCESS_GRANTED,
    AUDIO_ACCESS_DENIED,
    AUDIO_DISPENSE,
    AUDIO_COUNT  
} AudioEvent;

typedef struct {
    const char *name;
    const char *files[AUDIO_COUNT];  
} VoicePack;

static const VoicePack voice_packs[] = {
    {
        .name = "Default", 
        .files = {
            [AUDIO_PILL_REMINDER]  = "1take_p.wav",
            [AUDIO_ACCESS_GRANTED] = "1acc_gra.wav",
            [AUDIO_ACCESS_DENIED]  = "1acc_den.wav",
            [AUDIO_DISPENSE]       = "Dispense.wav",
        }
    },
    {
        .name = "Morgan Freeman",
        .files = {
            [AUDIO_PILL_REMINDER]  = "2take_p.wav",
            [AUDIO_ACCESS_GRANTED] = "2acc_gra.wav",
            [AUDIO_ACCESS_DENIED]  = "2acc_den.wav",
            [AUDIO_DISPENSE]       = "mf_dis.wav",
        }
    },
    {
        .name = "Spongebob",
        .files = {
            [AUDIO_PILL_REMINDER]  = "4take_p.wav",
            [AUDIO_ACCESS_GRANTED] = "4acc_gra.wav",
            [AUDIO_ACCESS_DENIED]  = "4acc_den.wav",
            [AUDIO_DISPENSE]       = "sb_dis.wav",
        }
    },
	{
	    .name = "Mickey Mouse",
	    .files = {
	        [AUDIO_PILL_REMINDER]  = "3take_p.wav",
	        [AUDIO_ACCESS_GRANTED] = "3acc_gra.wav",
	        [AUDIO_ACCESS_DENIED]  = "3acc_den.wav",
	        [AUDIO_DISPENSE]       = "hotdog.wav",
	    }
	},
	{
	    .name = "Brainrot",
	    .files = {
	        [AUDIO_PILL_REMINDER]  = "alarm.wav",
	        [AUDIO_ACCESS_GRANTED] = "heha.wav",
	        [AUDIO_ACCESS_DENIED]  = "fah.wav",
	        [AUDIO_DISPENSE]       = "pluh.wav",
	    }
	},
};

#define NUM_VOICE_PACKS (sizeof(voice_packs) / sizeof(voice_packs[0]))

int selected_voice = 0;
static uint8_t audio_single_shot = 0;
void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef *hdac)
{
    if (audio_single_shot) {
        Audio_Stop();
        audio_single_shot = 0;
    }
}

// Play audio, just need one for all voices
void Audio_Play(AudioEvent event) {
    const char *filename = voice_packs[selected_voice].files[event];
    audio_single_shot = (event == AUDIO_ACCESS_GRANTED || event == AUDIO_ACCESS_DENIED || event == AUDIO_DISPENSE);
    Audio_PlayFile(filename);  
}

void Audio_PlayFile(const char *filename) {
    // Stop any lingering audio first
    Audio_Stop();
    f_mount(NULL, "0:/", 0);
    HAL_Delay(10);

    extern char sd_path[4];
    strcpy(sd_path, "0:/");

    if (sd_mount() != FR_OK) {
        printf("Mount failed\r\n");
        return;
    }

    FIL wav_file;
    UINT br;
    if (f_open(&wav_file, filename, FA_READ) != FR_OK) {
        sd_unmount();
        printf("Cannot open: %s\r\n", filename);
        return;
    }

    uint8_t hdr[44];
    f_read(&wav_file, hdr, 44, &br);

    uint16_t channels    = hdr[22] | (hdr[23] << 8);
    uint32_t sample_rate = hdr[24] | (hdr[25] << 8) | (hdr[26] << 16) | (hdr[27] << 24);
    uint16_t bps         = hdr[34] | (hdr[35] << 8);
    uint32_t data_size   = hdr[40] | (hdr[41] << 8) | (hdr[42] << 16) | (hdr[43] << 24);

    uint32_t num_samples = data_size / (bps / 8);
    if (channels == 2) num_samples /= 2;

	#define AUDIO_MAX_SAMPLES 40000  // now safe to double

    if (num_samples > AUDIO_MAX_SAMPLES) {
        num_samples = AUDIO_MAX_SAMPLES;
    }

    printf("WAV: ch=%u, sr=%lu, bps=%u, data_size=%lu, num_samples=%lu\r\n",
           channels, sample_rate, bps, data_size, num_samples);

    static uint16_t dac_buf[AUDIO_MAX_SAMPLES];  // only ONE static buffer


    if (bps == 16) {
        int16_t *raw = (int16_t *)dac_buf;
        uint32_t read_bytes = (channels == 2) ? num_samples * 4 : num_samples * 2;
        f_read(&wav_file, raw, read_bytes, &br);

        // MUST walk backwards for mono 16-bit — src and dest overlap, dest is wider
        for (int32_t i = (int32_t)num_samples - 1; i >= 0; i--) {
            int16_t s = (channels == 2) ? raw[i * 2] : raw[i];
            dac_buf[i] = (uint16_t)((s + 32768) >> 4);
        }
    } else if (bps == 8) {
    	uint8_t *raw8 = (uint8_t *)dac_buf;  // alias — no extra RAM
    	f_read(&wav_file, raw8, num_samples * channels, &br);

    // Must walk BACKWARDS here — uint8 src and uint16 dest overlap, dest is wider
    	for (int32_t i = (int32_t)num_samples - 1; i >= 0; i--) {
    		uint8_t s = (channels == 2) ? raw8[i * 2] : raw8[i];
    		dac_buf[i] = (uint16_t)(s) << 4;
    	}
    }


    f_close(&wav_file);
    sd_unmount();

    uint32_t arr = (120000000UL / sample_rate) - 1;
    htim6.Instance->PSC = 0;
    htim6.Instance->ARR = arr;
    HAL_TIM_Base_Start(&htim6);
    HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_1,
                      (uint32_t *)dac_buf, num_samples,
                      DAC_ALIGN_12B_R);

    audio_playing    = 1;
    audio_start_tick = HAL_GetTick();
    printf("Playing: %s (%lu samples @ %lu Hz)\r\n", filename, num_samples, sample_rate);
}

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_UART4_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_LPUART1_UART_Init(void);
static void MX_TIM4_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI2_Init(void);
static void MX_RTC_Init(void);
static void MX_SPI3_Init(void);
static void MX_DAC1_Init(void);
static void MX_TIM6_Init(void);
/* USER CODE BEGIN PFP */
uint8_t xchg_spi(uint8_t dat);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_13) {
        button_pressed = 1;
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_UART4_Init();
  MX_USART1_UART_Init();
  MX_LPUART1_UART_Init();
  MX_TIM4_Init();
  MX_SPI1_Init();
  MX_SPI2_Init();
  MX_RTC_Init();
  MX_SPI3_Init();
  MX_DAC1_Init();
  MX_TIM6_Init();
  /* USER CODE BEGIN 2 */
  ILI9488_Init();
  XPT2046_Init();
  Menu_Draw();
  XPT2046_Touch touch;
  // cur_screen is now a global, initialized to SCREEN_MAIN above

  Fingerprint_Init(&myFinger, &huart4);
  printf("\r\n--- Fingerprint System Online ---\r\n");
  printf("Default Mode: VERIFY. Press Blue Button to ENROLL.\r\n");

  currentMode = MODE_VERIFY;


  // initialize passcode variables

  // initialize admin profile
  memset(profiles, 0, sizeof(profiles));
  strncpy(profiles[0].passcode, "123", sizeof(profiles[0].passcode));
  strncpy(profiles[0].name, "Admin", sizeof(profiles[0].name));
  profiles[0].fp_id = 0;     // admin has no fingerprint by default
  profiles[0].access_count = 0;
  profiles[0].late_count = 0;
  profiles[0].total_late_minutes = 0;
  num_profiles = 1;


  //Clock logic for buzzer
  RTC_TimeTypeDef sTime = {0};
   RTC_DateTypeDef sDate = {0};

   HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
   HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);  // MUST call GetDate after GetTime
   last_schedule_date = sDate;

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  Fingerprint_EmptyDatabase(&myFinger); // clear fingerprints


  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	  HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	  HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
	  rollover_daily_dose_if_needed(&sDate);
	  if (user_hr == sTime.Hours && user_min == sTime.Minutes) {
	      if (turnOffAlarm == 0 && !audio_playing) {
	          turnOffAlarm = 1; 
	          Audio_Play(AUDIO_PILL_REMINDER);
	      }
	  }

	  uint8_t prev_min = (user_min == 0) ? 59 : (uint8_t)(user_min - 1);
	  uint8_t prev_hr  = (user_min == 0 && user_hr > 0) ? (uint8_t)(user_hr - 1) : (uint8_t)user_hr;
	  if (sTime.Hours == prev_hr && sTime.Minutes == prev_min) {
		  turnOffAlarm = 0;
		  meds_taken = 0;
	  }

      dose_overdue = medication_overdue_now((int)sTime.Hours, (int)sTime.Minutes);


	// Clock update
	 static uint32_t last_clock_update = 0;
	 if (cur_screen == SCREEN_MAIN && HAL_GetTick() - last_clock_update >= 1000) {
		 last_clock_update = HAL_GetTick(); Menu_UpdateClock();
	 }

	 // Auto-stop audio after timeout
	 if (audio_playing && !audio_single_shot &&
	     HAL_GetTick() - audio_start_tick >= AUDIO_TIMEOUT_MS) {
	     printf("!!! TIMEOUT KILL at %lu ms\r\n", HAL_GetTick() - audio_start_tick);
	     Audio_Stop();
	 }

    // Display code //
    XPT2046_GetTouch(&touch);
    if (touch.touched) {
        if (cur_screen == SCREEN_MAIN) {
            Menu_HandleTouch(&touch);
            if (touch.x >= 10 && touch.x <= 130 &&
                touch.y >= 40 && touch.y <= 90) {
                cur_screen = SCREEN_OPEN_VERIFY;
            }
            if (touch.x >= 10 && touch.x <= 130 &&
                touch.y >= 110 && touch.y <= 160) {
                if (pills_remaining <= 0) {
                    Screen_SetMessage("No pills remaining!");
                    HAL_Delay(1500);
                    Menu_Draw();
                } else {
                    cur_screen = SCREEN_TAKE_VERIFY;
                }
            }
            if (touch.x >= 10 && touch.x <= 130 &&
            	touch.y >= 180 && touch.y <= 230) {
            	cur_screen = SCREEN_SETTINGS;
            }
            if (touch.x >= 180 && touch.x <= 380 &&
                touch.y >= 220 && touch.y <= 280) {
                cur_screen = SCREEN_VIEW_STATS;
            }
        } else if (cur_screen == SCREEN_OPEN_VERIFY) {
            Screen_Verify_HandleTouch(&touch);
            if (touch.x >= 10 && touch.x <= 90 &&
                touch.y >= 10 && touch.y <= 45) {
                cur_screen = SCREEN_MAIN;
            }
            if (touch.x >= 60  && touch.x <= 220 &&
                touch.y >= 130 && touch.y <= 190) {
                cur_screen = SCREEN_F_OPEN;
            }
            if (touch.x >= 260 && touch.x <= 420 &&
                touch.y >=
0 && touch.y <= 190) {
                cur_screen = SCREEN_K_OPEN;
            }
        } else if (cur_screen == SCREEN_F_OPEN) {
            Screen_FK_HandleTouch(&touch);
            if (touch.x >= 10 && touch.x <= 90 &&
                touch.y >= 10 && touch.y <= 45) {
                cur_screen = SCREEN_OPEN_VERIFY;
            }
        } else if (cur_screen == SCREEN_K_OPEN) {
            Screen_FK_HandleTouch(&touch);
            if (touch.x >= 10 && touch.x <= 90 &&
                touch.y >= 10 && touch.y <= 45) {
                cur_screen = SCREEN_OPEN_VERIFY;
            }
        } else if (cur_screen == SCREEN_K_ADD) {
            Screen_KeypadAdd_HandleTouch(&touch);
            if (touch.x >= 10 && touch.x <= 90 &&
                touch.y >= 10 && touch.y <= 45) {
                cur_screen = SCREEN_SETTINGS;
            }
        } else if (cur_screen == SCREEN_TAKE_VERIFY) {
            Screen_Verify_HandleTouch(&touch);
            if (touch.x >= 10 && touch.x <= 90 &&
                touch.y >= 10 && touch.y <= 45) {
                cur_screen = SCREEN_MAIN;
            }
            if (touch.x >= 60  && touch.x <= 220 &&
                touch.y >= 130 && touch.y <= 190) {
                cur_screen = SCREEN_F_TAKE;
            }
            if (touch.x >= 260 && touch.x <= 420 &&
                touch.y >= 130 && touch.y <= 190) {
                cur_screen = SCREEN_K_TAKE;
            }
        } else if (cur_screen == SCREEN_F_TAKE) {
            Screen_FK_HandleTouch(&touch);
            if (touch.x >= 10 && touch.x <= 90 &&
                touch.y >= 10 && touch.y <= 45) {
                cur_screen = SCREEN_TAKE_VERIFY;
            }
        } else if (cur_screen == SCREEN_K_TAKE) {
            Screen_FK_HandleTouch(&touch);
            if (touch.x >= 10 && touch.x <= 90 &&
                touch.y >= 10 && touch.y <= 45) {
                cur_screen = SCREEN_TAKE_VERIFY;
            }
        } else if (cur_screen == SCREEN_SETTINGS) {
            Screen_Settings_HandleTouch(&touch);
            if (touch.x >= 10 && touch.x <= 90 &&
                touch.y >= 10 && touch.y <= 45) {
                cur_screen = SCREEN_MAIN;
            }
            if (touch.x >= 60 && touch.x <= 220 &&
                touch.y >= 110 && touch.y <= 160) {
                cur_screen = SCREEN_ADD_VERIFY;
            }
            if (touch.x >= 260 && touch.x <= 420 &&
                touch.y >= 110 && touch.y <= 160) {
                cur_screen = SCREEN_SET_MEDS;
            }
            if (touch.x >= 60 && touch.x <= 220 &&
                touch.y >= 190 && touch.y <= 240) {
            	cur_screen = SCREEN_CHANGE_VOICE;
            }
            if (touch.x >= 260 && touch.x <= 420 &&
                touch.y >= 190 && touch.y <= 240) {
            	cur_screen = SCREEN_SET_TIME;
            }
        } else if (cur_screen == SCREEN_ADD_VERIFY) {
            Screen_AddVerify_HandleTouch(&touch);
            if (touch.x >= 10 && touch.x <= 90 &&
                touch.y >= 10 && touch.y <= 45) {
                cur_screen = SCREEN_SETTINGS;
            }
        } else if (cur_screen == SCREEN_ADD_FINGERPRINT) {
            Screen_AddVerify_HandleTouch(&touch);
            if (touch.x >= 10 && touch.x <= 90 &&
                touch.y >= 10 && touch.y <= 45) {
                cur_screen = SCREEN_SETTINGS;
            }
        } else if (cur_screen == SCREEN_SET_MEDS) {
            Screen_SetMeds_HandleTouch(&touch);
            if (touch.x >= 10 && touch.x <= 90 &&
                touch.y >= 10 && touch.y <= 45) {
                cur_screen = SCREEN_SETTINGS;
            }
        } else if (cur_screen == SCREEN_CHANGE_VOICE) {
        	Screen_ChangeVoice_HandleTouch(&touch);
            if (touch.x >= 10 && touch.x <= 90 &&
                touch.y >= 10 && touch.y <= 45) {
                cur_screen = SCREEN_SETTINGS;
            }
            if (touch.x >= 60 && touch.x <= 220 &&
                    touch.y >= 110 && touch.y <= 160) {
            	cur_screen = SCREEN_MAIN;
            }
            if (touch.x >= 260 && touch.x <= 420 &&
                    touch.y >= 110 && touch.y <= 160) {
                cur_screen = SCREEN_MAIN;
            }
            if (touch.x >= 60 && touch.x <= 220 &&
                	touch.y >= 190 && touch.y <= 240) {
                cur_screen = SCREEN_MAIN;
            }

            if (touch.x >= 260 && touch.x <= 420 &&
                    touch.y >= 190 && touch.y <= 240) {
                cur_screen = SCREEN_MAIN;
            }
        } else if (cur_screen == SCREEN_SET_TIME) {
            Screen_SetTime_HandleTouch(&touch);
            if (touch.x >= 10 && touch.x <= 90 &&
                touch.y >= 10 && touch.y <= 45) {
                cur_screen = SCREEN_SETTINGS;
            }
        } else if (cur_screen == SCREEN_VIEW_STATS) {
            Screen_ViewStats_HandleTouch(&touch);
            if (touch.x >= 10 && touch.x <= 90 &&
                touch.y >= 10 && touch.y <= 45) {
                cur_screen = SCREEN_MAIN;
            }
        } else if (cur_screen == SCREEN_REFILL_PROMPT) {
            Screen_RefillPrompt_HandleTouch(&touch);
            if ((touch.x >= 70 && touch.x <= 210 && touch.y >= 170 && touch.y <= 230) ||
                (touch.x >= 270 && touch.x <= 410 && touch.y >= 170 && touch.y <= 230)) {
                cur_screen = SCREEN_MAIN;
                Menu_Draw();
            }
        } else if (cur_screen == SCREEN_NAME_ENTRY) {
            Screen_NameEntry_HandleTouch(&touch);
            if (pending_profile_idx == -1 && !adding_name) {
                cur_screen = SCREEN_SETTINGS;
            }
        }
        HAL_Delay(20);
    }

    // Motor control code //

    if (test_auth && motor_state == MOTOR_IDLE) {
        test_auth = 0;
        move_steps_pwm(steps[current_pos]);
        motor_state = MOTOR_MOVING;

        if (HAL_GetTick() - last_move_time >= move_interval) {
            move_steps_pwm(steps[current_pos]);
            motor_state = MOTOR_MOVING;
        }
    }

    if (motor_state == MOTOR_MOVING && move_done) {
        motor_timer = HAL_GetTick();
        motor_state = MOTOR_WAITING;
    }

    if (motor_state == MOTOR_WAITING &&
        HAL_GetTick() - motor_timer >= 2000) {
        current_pos = (current_pos + 1) % NUM_POS;
        last_move_time = HAL_GetTick();
        motor_state = MOTOR_IDLE;
    }
   
    // Keypad code // 
    for (int r = 0; r < 4; r++) {
        for (int i = 0; i < 4; i++) {
            HAL_GPIO_WritePin(rowPorts[i], rowPins[i], GPIO_PIN_RESET);
        }
        HAL_GPIO_WritePin(rowPorts[r], rowPins[r], GPIO_PIN_SET);

        HAL_Delay(1);

        for (int c = 0; c < 4; c++) {
            if (HAL_GPIO_ReadPin(colPorts[c], colPins[c]) == GPIO_PIN_SET) {

                if (HAL_GetTick() - last_key_time > DEBOUNCE_MS) {
                    last_key_time = HAL_GetTick();

                    char pressedKey = keys[r][c];
                    printf("Key: %c\r\n", pressedKey);

                    if (pressedKey == '*') {
                      buffer_idx = 0;
                      memset(password_buffer, 0, sizeof(password_buffer));
                      if (cur_screen == SCREEN_SET_MEDS) {
                        Screen_SetMeds_UpdateEntry();
                      } else if (cur_screen == SCREEN_SET_TIME) {
                          Screen_SetTime_UpdateEntry();
                        } else if (adding_passcode) {

                        Screen_Keypad_UpdateEntry(0);
                      } else if (cur_screen == SCREEN_K_OPEN || cur_screen == SCREEN_K_TAKE ||
                                 cur_screen == SCREEN_ADD_VERIFY || cur_screen == SCREEN_K_ADD) {
                        Screen_Keypad_UpdateEntry(1);
                      }
                    }
                    else if (pressedKey == '#') {
                      // Setting meds time logic
                      if (cur_screen == SCREEN_SET_MEDS) {
                        if (buffer_idx == 4) {
                          int h = (password_buffer[0]-'0')*10 + (password_buffer[1]-'0');
                          int m = (password_buffer[2]-'0')*10 + (password_buffer[3]-'0');
                          if (h >= 0 && h < 24 && m >= 0 && m < 60) {
                            user_hr = h;
                            user_min = m;
                            reset_daily_dose_state();
                            Screen_SetMessage("Meds time saved");
                            HAL_Delay(1000);
                            buffer_idx = 0;
                            memset(password_buffer, 0, sizeof(password_buffer));
                            cur_screen = SCREEN_SETTINGS;
                            Screen_Settings_Draw();
                          } else {
                            Screen_SetMessage("Invalid time (HHMM)");
                            HAL_Delay(1000);
                            buffer_idx = 0;
                            memset(password_buffer, 0, sizeof(password_buffer));
                            Screen_SetMeds_Draw();
                          }
                        } else {
                          Screen_SetMessage("Enter 4 digits HHMM");
                          HAL_Delay(1000);
                          buffer_idx = 0;
                          memset(password_buffer, 0, sizeof(password_buffer));
                          Screen_SetMeds_Draw();
                        }
                      } else if (cur_screen == SCREEN_SET_TIME) {
                          if (buffer_idx == 4) {
                            int h = (password_buffer[0]-'0')*10 + (password_buffer[1]-'0');
                            int m = (password_buffer[2]-'0')*10 + (password_buffer[3]-'0');
                            if (h >= 0 && h < 24 && m >= 0 && m < 60) {
                                sTime.Hours          = h;
                                sTime.Minutes        = m;
                                sTime.Seconds        = 0;
                                sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
                                sTime.StoreOperation = RTC_STOREOPERATION_RESET;
                                if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) == HAL_OK) {
                                    Screen_SetMessage("Clock time saved!");
                                } else {
                                    Screen_SetMessage("RTC write failed!");
                                }
                                HAL_Delay(1000);
                              buffer_idx = 0;
                              memset(password_buffer, 0, sizeof(password_buffer));
                              cur_screen = SCREEN_SETTINGS;
                              Screen_Settings_Draw();
                            } else {
                              Screen_SetMessage("Invalid time (HHMM)");
                              HAL_Delay(1000);
                              buffer_idx = 0;
                              memset(password_buffer, 0, sizeof(password_buffer));
                              Screen_SetTime_Draw();
                            }
                          } else {
                            Screen_SetMessage("Enter 4 digits HHMM");
                            HAL_Delay(1000);
                            buffer_idx = 0;
                            memset(password_buffer, 0, sizeof(password_buffer));
                            Screen_SetTime_Draw();
                          }
                      }
                      else if (adding_passcode) {
                    	    if (buffer_idx > 0 && pending_profile_idx >= 0 && pending_profile_idx < MAX_USERS) {
                              strncpy(profiles[pending_profile_idx].passcode, password_buffer, sizeof(profiles[pending_profile_idx].passcode)-1);
                              profiles[pending_profile_idx].passcode[sizeof(profiles[pending_profile_idx].passcode)-1] = '\0';
                    	        printf("New passcode saved: %s\r\n", password_buffer);
                    	    }
                    	    adding_passcode = 0;
                          adding_name = 1;
                    	    buffer_idx = 0;
                    	    memset(password_buffer, 0, sizeof(password_buffer));
                          name_buffer_idx = 0;
                          memset(name_buffer, 0, sizeof(name_buffer));
                    	    cur_screen = SCREEN_NAME_ENTRY;
                    	    Screen_NameEntry_Draw();
                    	}
                    	else {
                        int matched_profile = -1;
                        for (int i = 0; i < num_profiles; i++) {
                            if (strcmp(password_buffer, profiles[i].passcode) == 0) {
                                matched_profile = i;
                                break;
                            }
                        }
                        if ((buffer_idx >0) && (matched_profile >= 0)) {
                          int dispense_completed = 0;
                          if (cur_screen == SCREEN_K_OPEN) {
                        	Audio_Play(AUDIO_ACCESS_GRANTED);
                            Screen_Fingerprint_SetStatus(1, 1, happyturtle_bmp);
                            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
                            lock_timer = HAL_GetTick();
                            HAL_Delay(2000);
                            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
                            lock_active = 1;
                            printf("Keypad OK: Deadbolt opened.\r\n");
                            cur_screen = SCREEN_REFILL_PROMPT;
                            Screen_RefillPrompt_Draw();
                          } else if (cur_screen == SCREEN_K_TAKE) {
                          	if (pills_remaining <= 0) {
                          	    Audio_Play(AUDIO_ACCESS_DENIED);
                          	    Screen_Fingerprint_SetStatus(0, 0, sadturtle_bmp);
                          	    Screen_SetMessage("Dispenser is empty!");
                          	    HAL_Delay(2000);
                          	    cur_screen = SCREEN_MAIN;
                          	    Menu_Draw();
                          	}
                          	else if (medication_due_now((int)sTime.Hours, (int)sTime.Minutes)) {
                            	uint32_t late_minutes = medication_late_minutes((int)sTime.Hours, (int)sTime.Minutes);
                              profiles[matched_profile].access_count++;
                              if (dose_overdue) {
                            	  profiles[matched_profile].late_count++;
                            	  profiles[matched_profile].total_late_minutes += late_minutes;
                              }
                              const unsigned char *face = NULL;
                              const char *pname = profiles[matched_profile].name;
                              if (strcmp(pname, "Manasa") == 0 || strcmp(pname, "MANASA") == 0 || strcmp(pname, "manasa") == 0) {
                                  face = user0_bmp;
                              } else if (strcmp(pname, "Kevin") == 0 || strcmp(pname, "KEVIN") == 0 || strcmp(pname, "kevin") == 0) {
                                  face = User_Kevin_bmp;
                              } else if (strcmp(pname, "Elena") == 0 || strcmp(pname, "ELENA") == 0 || strcmp(pname, "elena") == 0) {
                                  face = User_Elena_bmp;
                              } else if (strcmp(pname, "Nithya") == 0 || strcmp(pname, "NITHYA") == 0 || strcmp(pname, "nithya") == 0) {
                                  face = User_Nithya_bmp;
                              } else if (strcmp(pname, "George") == 0 || strcmp(pname, "GEORGE") == 0 || strcmp(pname, "george") == 0) {
                                  face = User_George_bmp;
                              }
                              if(face == NULL){
                            	  Screen_Fingerprint_SetStatus(1, 0, happyturtle_bmp);
                              }else{
                            	  Screen_Fingerprint_SetStatus(1, 0, face);
                              }
                              test_auth = 1;
                              HAL_Delay(1500);
                              printf("Keypad OK: Dispensing medication.\r\n");
                              record_dispense(sTime.Hours, sTime.Minutes);
                              dispense_completed = 1;
                              Audio_Play(AUDIO_DISPENSE);
                            } else {
                              Screen_SetMessage("Medication not due yet");
                              HAL_Delay(1000);
                              Screen_Keypad_Draw();
                            }
                          } else if (cur_screen == SCREEN_ADD_VERIFY) {
                            Screen_Fingerprint_SetStatus(1, 2, NULL);
                            cur_screen = SCREEN_ADD_FINGERPRINT;
                            currentMode = MODE_ENROLL;
                            HAL_Delay(1500);
                            break;
                          }
                          if (cur_screen != SCREEN_REFILL_PROMPT && cur_screen != SCREEN_K_TAKE) {
                              cur_screen = SCREEN_MAIN;
                              Menu_Draw();
                          } else if (cur_screen == SCREEN_K_TAKE && dispense_completed) {
                              cur_screen = SCREEN_MAIN;
                              Menu_Draw();
                          }
                        }
                        buffer_idx = 0;
                        memset(password_buffer, 0, sizeof(password_buffer));
                      }
                    }
                    else if (buffer_idx < 15) {
                      password_buffer[buffer_idx++] = pressedKey;
                      password_buffer[buffer_idx] = '\0';
                      if (cur_screen == SCREEN_SET_MEDS) {
                        Screen_SetMeds_UpdateEntry();
                      } else if (cur_screen == SCREEN_SET_TIME) {
                          Screen_SetTime_UpdateEntry();
                        } else if (adding_passcode) {
                        Screen_Keypad_UpdateEntry(0);
                      } else if (cur_screen == SCREEN_K_OPEN || cur_screen == SCREEN_K_TAKE ||
                                 cur_screen == SCREEN_ADD_VERIFY || cur_screen == SCREEN_K_ADD) {
                        Screen_Keypad_UpdateEntry(1);
                      }
                    }
                }

                // wait for release
                while (HAL_GPIO_ReadPin(colPorts[c], colPins[c]) == GPIO_PIN_SET) {
                }
            }
        }

        HAL_GPIO_WritePin(rowPorts[r], rowPins[r], GPIO_PIN_RESET);
    }

    // fingerprint button // 

    if (button_pressed == 1 && cur_screen == SCREEN_ADD_FINGERPRINT) {
        HAL_Delay(50); // debounce
        if (button_pressed) {
            printf("\r\n>>> SYSTEM: Switching to ENROLL Mode <<<\r\n");
            button_pressed = 0;
            currentMode = MODE_ENROLL;
        }
    }

    // fingerprint enroll mode //

    if (currentMode == MODE_ENROLL) {
         if (num_profiles >= MAX_USERS) {
             Screen_SetMessage("User list full");
             HAL_Delay(1000);
             currentMode = MODE_VERIFY;
             cur_screen = SCREEN_SETTINGS;
             Screen_Settings_Draw();
             continue;
         }

         printf("\r\n[ENROLLING ID #%d]\r\n", nextID);

             // Step 1: Place finger
             Screen_SetEnrollStep("Place finger on sensor", fingerprint1_bmp);
             while (1) {
                 while (Fingerprint_GetImage(&myFinger) != FINGERPRINT_OK);

                 if (Fingerprint_Image2Tz(&myFinger, 1) == FINGERPRINT_OK) {
                     break;
                 }
                 Screen_SetEnrollStep("Scan failed. Try again.", fingerprint1_bmp);
                 HAL_Delay(1000);
             }

             // Step 2: Lift finger
             Screen_SetEnrollStep("Lift finger off sensor", fingerprint2_bmp);
             while (Fingerprint_GetImage(&myFinger) != FINGERPRINT_NOFINGER) { HAL_Delay(100); }
             HAL_Delay(500);

             // Step 3: Place same finger again
             Screen_SetEnrollStep("Place same finger again", fingerprint3_bmp);
             while (Fingerprint_GetImage(&myFinger) != FINGERPRINT_OK);

             if (Fingerprint_Image2Tz(&myFinger, 2) != FINGERPRINT_OK) {
                 Screen_SetEnrollStep("Second scan failed.", fingerprint1_bmp);
                 HAL_Delay(1000);
                 currentMode = MODE_VERIFY;
             }

             if (Fingerprint_RegModel(&myFinger) == FINGERPRINT_OK) {
                 if (Fingerprint_Store(&myFinger, nextID) == FINGERPRINT_OK) {
                   pending_profile_idx = num_profiles;
                   memset(&profiles[pending_profile_idx], 0, sizeof(profiles[pending_profile_idx]));
                   profiles[pending_profile_idx].fp_id = nextID;
                   nextID++;
                   printf("SUCCESS: fp_id #%d saved.\r\n", profiles[pending_profile_idx].fp_id);
                   // Step 4: Success!
                   Screen_SetEnrollStep("Fingerprint saved!", fingerprint4_bmp);
                   HAL_Delay(1000);
                 } else {
                     Screen_SetMessage("Fingerprint save failed");
                     HAL_Delay(1000);
                 }
             } else {
                 Screen_SetMessage("Scans did not match");
                 HAL_Delay(1000);
             }

        currentMode = MODE_VERIFY;
        adding_passcode = 1;
        cur_screen = SCREEN_K_ADD;
        Screen_Keypad_Draw();
    }

    // FINGERPRINT — VERIFY MODE
    else if (cur_screen == SCREEN_F_OPEN) {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        uint8_t imgResult = Fingerprint_GetImage(&myFinger);
        if (imgResult == FINGERPRINT_OK) {
            if (Fingerprint_Image2Tz(&myFinger, 1) == FINGERPRINT_OK) {
                if (Fingerprint_FastSearch(&myFinger) == FINGERPRINT_OK) {
                    printf(">>> VERIFIED: Opening lid...\r\n");
                    Audio_Play(AUDIO_ACCESS_GRANTED);
                    //happy = 1;
                    Screen_Fingerprint_SetStatus(1, 1, happy);
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
                    lock_timer = HAL_GetTick();
                    HAL_Delay(2000);
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
                    lock_active = 1;
                    printf("Lock Opened!\r\n");
                    cur_screen = SCREEN_REFILL_PROMPT;
                    Screen_RefillPrompt_Draw();
                } else {
                    printf("!!! NOT VERIFIED !!!\r\n");
                    Audio_Play(AUDIO_ACCESS_DENIED);
                    Screen_Fingerprint_SetStatus(0, 1, sadturtle_bmp);
                    for (int i = 0; i < 6; i++) {
                        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
                        HAL_Delay(50);
                    }
                }
            }
            HAL_Delay(1000);
            if (cur_screen == SCREEN_F_OPEN) {
            	Screen_Fingerprint_Draw();
            }
        }
    } else if (cur_screen == SCREEN_F_TAKE) {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        uint8_t imgResult = Fingerprint_GetImage(&myFinger);
        if (imgResult == FINGERPRINT_OK) {
            if (Fingerprint_Image2Tz(&myFinger, 1) == FINGERPRINT_OK) {
                if (Fingerprint_FastSearch(&myFinger) == FINGERPRINT_OK) {
                	if (pills_remaining <= 0) {
                	    Audio_Play(AUDIO_ACCESS_DENIED);
                	    Screen_Fingerprint_SetStatus(0, 0, sadturtle_bmp);
                	    Screen_SetMessage("Dispenser is empty!");
                	    HAL_Delay(2000);
                	    cur_screen = SCREEN_MAIN;
                	    Menu_Draw();
                	}
                	else if (medication_due_now((int)sTime.Hours, (int)sTime.Minutes)) {
                        printf(">>> VERIFIED: Dispensing medication...\r\n");
                        test_auth = 1;
                        record_dispense(sTime.Hours, sTime.Minutes);
                        HAL_Delay(1500);
                        const unsigned char *face = NULL;
                        for (int i = 0; i < num_profiles; i++) {
                            if (profiles[i].fp_id == myFinger.fingerID) {
                            	const char *pname = profiles[i].name;
                            	if (strcmp(pname, "Manasa") == 0 || strcmp(pname, "MANASA") == 0 || strcmp(pname, "manasa") == 0) {
                            	    face = user0_bmp;
                            	} else if (strcmp(pname, "Kevin") == 0 || strcmp(pname, "KEVIN") == 0 || strcmp(pname, "kevin") == 0) {
                            	    face = User_Kevin_bmp;
                            	} else if (strcmp(pname, "Elena") == 0 || strcmp(pname, "ELENA") == 0 || strcmp(pname, "elena") == 0) {
                            	    face = User_Elena_bmp;
                            	} else if (strcmp(pname, "Nithya") == 0 || strcmp(pname, "NITHYA") == 0 || strcmp(pname, "nithya") == 0) {
                            	    face = User_Nithya_bmp;
                            	} else if (strcmp(pname, "George") == 0 || strcmp(pname, "GEORGE") == 0 || strcmp(pname, "george") == 0) {
                            	    face = User_George_bmp;
                            	}

                                profiles[i].access_count++;
                                if (dose_overdue) {
                                	profiles[i].late_count++;
                                	profiles[i].total_late_minutes += medication_late_minutes((int)sTime.Hours, (int)sTime.Minutes);
                                }
                                break;
                            }
                        }
                        if(face != NULL){
                        	Screen_Fingerprint_SetStatus(1,0,face);
                        }else{
                        	Screen_Fingerprint_SetStatus(1,0,happyturtle_bmp);
                        }
                        cur_screen = SCREEN_MAIN;
                        Menu_Draw();
                    } else {
                        Screen_SetMessage("Medication not due yet");

                    }
                } else {
                    printf("!!! NOT VERIFIED !!!\r\n");
                    HAL_Delay(50);
                    Screen_Fingerprint_SetStatus(0, 1, sadturtle_bmp);
                    for (int i = 0; i < 6; i++) {
                        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
                        HAL_Delay(50);
                    }
                }
            }
            HAL_Delay(1000);
            if (cur_screen == SCREEN_F_TAKE) {
                ILI9488_DrawStringCentered(0, 110, 480, 40, "Please place your finger...", COLOR_WHITE, POND_DEEP, 2);
            }
        }
    }

    // LOCK AUTO-CLOSE (non-blocking)
    if (lock_active && (HAL_GetTick() - lock_timer >= 500)) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
        lock_active = 0;
    }

  } 
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 60;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief DAC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_DAC1_Init(void)
{

  /* USER CODE BEGIN DAC1_Init 0 */

  /* USER CODE END DAC1_Init 0 */

  DAC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN DAC1_Init 1 */

  /* USER CODE END DAC1_Init 1 */

  /** DAC Initialization
  */
  hdac1.Instance = DAC1;
  if (HAL_DAC_Init(&hdac1) != HAL_OK)
  {
    Error_Handler();
  }

  /** DAC channel OUT1 config
  */
  sConfig.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE;
  sConfig.DAC_Trigger = DAC_TRIGGER_T6_TRGO;
  sConfig.DAC_HighFrequency = DAC_HIGH_FREQUENCY_INTERFACE_MODE_ABOVE_80MHZ;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  sConfig.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_DISABLE;
  sConfig.DAC_UserTrimming = DAC_TRIMMING_FACTORY;
  if (HAL_DAC_ConfigChannel(&hdac1, &sConfig, DAC_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DAC1_Init 2 */

  /* USER CODE END DAC1_Init 2 */

}

/**
  * @brief LPUART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_LPUART1_UART_Init(void)
{

  /* USER CODE BEGIN LPUART1_Init 0 */

  /* USER CODE END LPUART1_Init 0 */

  /* USER CODE BEGIN LPUART1_Init 1 */

  /* USER CODE END LPUART1_Init 1 */
  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 115200;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits = UART_STOPBITS_1;
  hlpuart1.Init.Parity = UART_PARITY_NONE;
  hlpuart1.Init.Mode = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  hlpuart1.FifoMode = UART_FIFOMODE_DISABLE;
  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&hlpuart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&hlpuart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LPUART1_Init 2 */

  /* USER CODE END LPUART1_Init 2 */

}

/**
  * @brief UART4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART4_Init(void)
{

  /* USER CODE BEGIN UART4_Init 0 */

  /* USER CODE END UART4_Init 0 */

  /* USER CODE BEGIN UART4_Init 1 */

  /* USER CODE END UART4_Init 1 */
  huart4.Instance = UART4;
  huart4.Init.BaudRate = 57600;
  huart4.Init.WordLength = UART_WORDLENGTH_8B;
  huart4.Init.StopBits = UART_STOPBITS_1;
  huart4.Init.Parity = UART_PARITY_NONE;
  huart4.Init.Mode = UART_MODE_TX_RX;
  huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart4.Init.OverSampling = UART_OVERSAMPLING_16;
  huart4.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart4.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart4.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart4) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart4, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart4, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART4_Init 2 */

  /* USER CODE END UART4_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0x0;
  sTime.Minutes = 0x0;
  sTime.Seconds = 0x0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 0x1;
  sDate.Year = 0x0;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 7;
  hspi2.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief SPI3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI3_Init(void)
{

  /* USER CODE BEGIN SPI3_Init 0 */

  /* USER CODE END SPI3_Init 0 */

  /* USER CODE BEGIN SPI3_Init 1 */

  /* USER CODE END SPI3_Init 1 */
  /* SPI3 parameter configuration*/
  hspi3.Instance = SPI3;
  hspi3.Init.Mode = SPI_MODE_MASTER;
  hspi3.Init.Direction = SPI_DIRECTION_2LINES;
  hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi3.Init.NSS = SPI_NSS_SOFT;
  hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
  hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi3.Init.CRCPolynomial = 7;
  hspi3.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi3.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI3_Init 2 */

  /* USER CODE END SPI3_Init 2 */

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 19999;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 21;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 11;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */
  HAL_TIM_MspPostInit(&htim4);

}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 79;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 62;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMAMUX1_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  /* DMA1_Channel2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);
  /* DMA1_Channel3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);
  /* DMA1_Channel4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  HAL_PWREx_EnableVddIO2();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOF, LCD_CS_Pin|SD_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, LCD_LED_Pin|LCD_DC_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1|GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1|GPIO_PIN_2|T_CS_Pin|GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pins : PE2 PE3 */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF13_SAI1;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : PE4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PE5 PE6 */
  GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PF0 PF1 PF2 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C2;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pin : LCD_CS_Pin */
  GPIO_InitStruct.Pin = LCD_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LCD_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PF7 */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF13_SAI1;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pin : SD_CS_Pin */
  GPIO_InitStruct.Pin = SD_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(SD_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_LED_Pin LCD_DC_Pin */
  GPIO_InitStruct.Pin = LCD_LED_Pin|LCD_DC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PC2 PC3 PC4 PC5 */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PA1 PA9 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA3 */
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_RST_Pin PB1 PB2 T_CS_Pin
                           PB7 */
  GPIO_InitStruct.Pin = LCD_RST_Pin|GPIO_PIN_1|GPIO_PIN_2|T_CS_Pin
                          |GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PE7 PE8 PE9 PE10
                           PE11 PE12 PE13 */
  GPIO_InitStruct.Pin = GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10
                          |GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PE14 PE15 */
  GPIO_InitStruct.Pin = GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF3_TIM1_COMP1;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : PB10 */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PB13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF13_SAI2;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PD8 PD9 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : T_IRQ_Pin */
  GPIO_InitStruct.Pin = T_IRQ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(T_IRQ_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PC6 */
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF13_SAI2;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PC7 */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PC8 PC9 PC12 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_SDMMC1;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA8 PA10 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG_FS;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PD0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : PD2 */
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_SDMMC1;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : PD3 PD4 PD5 PD6 */
  GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : PB6 */
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB8 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
PUTCHAR_PROTOTYPE
{
  HAL_UART_Transmit(&hlpuart1, (uint8_t *)&ch, 1, 0xFFFF);
  return ch;
}


void Buzzer_Buzz(int duration_ms){
	int cycles = duration_ms * 2;

	for (int i = 0; i < cycles; i++){
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);
		//HAL_Delay(1);
		HAL_GetTick();
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
		HAL_GetTick();
		//HAL_Delay(1);
	}
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
