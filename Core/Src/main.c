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
#include "string.h"
#include "cmsis_os.h"
#include "fatfs.h"
#include "usb_host.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32746g_discovery_lcd.h"
#include "stm32746g_discovery_ts.h"

#include <stdio.h>
#include <string.h>

#include "fatfs.h"
#include "ff.h"
#include "diskio.h"
#include "bsp_driver_sd.h"
#include "lwip/tcpip.h"
#include "lwip/dhcp.h"
#include "lwip/netifapi.h"
#include "lwip/apps/sntp.h"
#include "ethernetif.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */


#define APP_SEPARATOR_X              280U

#define APP_MILK_BUTTON_WIDTH         84U
#define APP_MILK_BUTTON_HEIGHT        52U

#define APP_MILK_BUTTON_X1           290U
#define APP_MILK_BUTTON_X2           386U

#define APP_MILK_BUTTON_Y1            30U
#define APP_MILK_BUTTON_Y2            95U
#define APP_MILK_BUTTON_Y3           160U

#define APP_RECORD_X                  10U
#define APP_RECORD_Y                  88U
#define APP_RECORD_WIDTH             260U
#define APP_RECORD_HEIGHT            185U
#define APP_RECORD_LINE_HEIGHT        22U

#define APP_HISTORY_MAX_RECORDS      256U
#define APP_RECORDS_PER_PAGE           8U
#define APP_RECORD_TEXT_LENGTH        32U
#define APP_TOUCH_DEBOUNCE_MS        200U
#define APP_HISTORY_SWIPE_PIXELS       35U
#define APP_VARIABLE_SWIPE_PIXELS      25U
#define APP_VARIABLE_TAP_MOVE_PIXELS   12U
#define APP_VARIABLE_MIN_ML            10U
#define APP_VARIABLE_MAX_ML           990U
#define APP_VARIABLE_STEP_ML           10U
#define APP_VARIABLE_X                290U
#define APP_VARIABLE_Y                216U
#define APP_VARIABLE_WIDTH             84U
#define APP_VARIABLE_HEIGHT            52U
#define APP_DELETE_X                  386U
#define APP_DELETE_Y                  216U
#define APP_DELETE_WIDTH               84U
#define APP_DELETE_HEIGHT              52U
#define APP_MAIN_MENU_X               205U
#define APP_MAIN_MENU_Y                 3U
#define APP_MAIN_MENU_WIDTH            65U
#define APP_MAIN_MENU_HEIGHT           24U
#define APP_MENU_BUTTON_WIDTH         190U
#define APP_MENU_BUTTON_HEIGHT         65U
#define APP_MENU_LEFT_X                30U
#define APP_MENU_RIGHT_X              260U
#define APP_MENU_TOP_Y                 75U
#define APP_MENU_BOTTOM_Y             165U
#define APP_SYNC_CANCEL_X             170U
#define APP_SYNC_CANCEL_Y             185U
#define APP_SYNC_CANCEL_WIDTH         140U
#define APP_SYNC_CANCEL_HEIGHT         55U
#define APP_PENDING_MAX_RECORDS        32U
#define APP_SD_RETRY_MS               500U
#define APP_SD_STEP_DELAY_MS            20U
#define APP_SD_INTERWRITE_MS           500U
#define APP_SD_REINSERT_STABLE_MS      500U
#define APP_SD_POST_RECOVERY_MS        200U
#define APP_SD_DIRECT_FAILURE_LIMIT      3U
#define APP_COLOR_PENDING      0xFFFF8C00U

#define RTC_INIT_MAGIC             0x32F3U

#define SET_MINUS_X          10U
#define SET_PLUS_X           125U
#define SET_NEXT_X           240U
#define SET_SAVE_X           355U

#define SET_BUTTON_Y         180U
#define SET_BUTTON_WIDTH     110U
#define SET_BUTTON_HEIGHT     60U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc3;

CRC_HandleTypeDef hcrc;

DCMI_HandleTypeDef hdcmi;

DMA2D_HandleTypeDef hdma2d;

ETH_HandleTypeDef heth;

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c3;

LTDC_HandleTypeDef hltdc;

QSPI_HandleTypeDef hqspi;

RTC_HandleTypeDef hrtc;

SAI_HandleTypeDef hsai_BlockA2;
SAI_HandleTypeDef hsai_BlockB2;

SD_HandleTypeDef hsd1;

SPDIFRX_HandleTypeDef hspdif;

SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim5;
TIM_HandleTypeDef htim8;
TIM_HandleTypeDef htim12;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart6;

SDRAM_HandleTypeDef hsdram1;

osThreadId defaultTaskHandle;
/* USER CODE BEGIN PV */

static osThreadId appStorageTaskHandle;
static osMailQId appStorageQueue;
static osMessageQId appStorageDoneQueue;

static TS_StateTypeDef appTouchState;

typedef struct
{
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
    uint16_t amountMl;
    const char *label;
} APP_MilkButtonTypeDef;


typedef struct
{
    uint16_t year;

    uint8_t month;
    uint8_t day;

    uint8_t hour;
    uint8_t minute;
    uint8_t second;

    uint16_t amountMl;
    uint32_t sourceOrdinal;
    uint32_t recordId;
    uint8_t pending;
} APP_MilkRecordTypeDef;

osMailQDef(AppStorageMail, APP_PENDING_MAX_RECORDS,
           APP_MilkRecordTypeDef);
osMessageQDef(AppStorageDone, APP_PENDING_MAX_RECORDS, uint32_t);


/*
 * 右侧六个按钮。
 */
static const APP_MilkButtonTypeDef appMilkButtons[] =
{
    {
        APP_MILK_BUTTON_X1,
        APP_MILK_BUTTON_Y1,
        APP_MILK_BUTTON_WIDTH,
        APP_MILK_BUTTON_HEIGHT,
        30U,
        "30ml"
    },
    {
        APP_MILK_BUTTON_X2,
        APP_MILK_BUTTON_Y1,
        APP_MILK_BUTTON_WIDTH,
        APP_MILK_BUTTON_HEIGHT,
        50U,
        "50ml"
    },
    {
        APP_MILK_BUTTON_X1,
        APP_MILK_BUTTON_Y2,
        APP_MILK_BUTTON_WIDTH,
        APP_MILK_BUTTON_HEIGHT,
        90U,
        "90ml"
    },
    {
        APP_MILK_BUTTON_X2,
        APP_MILK_BUTTON_Y2,
        APP_MILK_BUTTON_WIDTH,
        APP_MILK_BUTTON_HEIGHT,
        100U,
        "100ml"
    },
    {
        APP_MILK_BUTTON_X1,
        APP_MILK_BUTTON_Y3,
        APP_MILK_BUTTON_WIDTH,
        APP_MILK_BUTTON_HEIGHT,
        110U,
        "110ml"
    },
    {
        APP_MILK_BUTTON_X2,
        APP_MILK_BUTTON_Y3,
        APP_MILK_BUTTON_WIDTH,
        APP_MILK_BUTTON_HEIGHT,
        120U,
        "120ml"
    }
};

#define APP_MILK_BUTTON_COUNT \
    ((uint8_t)(sizeof(appMilkButtons) / sizeof(appMilkButtons[0])))


static APP_MilkRecordTypeDef appMilkRecords[APP_HISTORY_MAX_RECORDS];

static uint16_t appRecordCount = 0U;
static uint16_t appHistoryPage = 0U;
static int16_t appSelectedRecordIndex = -1;
static uint32_t appCsvRecordCount = 0U;
static uint32_t appNextRecordId = 0U;
static uint8_t appPendingCount = 0U;
static volatile uint8_t appStorageFault = 0U;
static volatile uint8_t appStorageQueueFull = 0U;
static volatile FRESULT appStorageLastResult = FR_OK;
static volatile uint32_t appStorageStatusVersion = 0U;
static volatile uint8_t appStorageWriteStage = 0U;
static volatile uint8_t appSdRemovalSeen = 0U;
static uint32_t appTodayTotalMl = 0U;
static uint16_t appVariableAmountMl = 150U;

static uint16_t appTodayYear = 0U;
static uint8_t appTodayMonth = 0U;
static uint8_t appTodayDay = 0U;

static uint32_t appLastPressTick = 0U;

static DSTATUS appSdDiskStatus = STA_NOINIT;
static uint32_t appSdHalError = 0U;
static uint32_t appSdHalState = 0U;
static uint8_t appBspSdInitResult = 0xFFU;
static uint8_t appBspSdCardState = 0xFFU;
static BYTE appSdPhysicalDrive = 0xFFU;
static uint8_t appSdFailStage = 0U;
static volatile uint8_t appSdReady = 0U;
static struct netif appNetif;
static volatile uint8_t appTimeSynchronized = 0U;
static uint8_t appNetworkInitialized = 0U;


typedef enum
{
    APP_FIELD_YEAR = 0,
    APP_FIELD_MONTH,
    APP_FIELD_DAY,
    APP_FIELD_HOUR,
    APP_FIELD_MINUTE,
    APP_FIELD_SECOND,
    APP_FIELD_COUNT
} APP_TimeFieldTypeDef;

static uint16_t setupYear = 2026U;
static uint8_t setupMonth = 1U;
static uint8_t setupDay = 1U;
static uint8_t setupHour = 0U;
static uint8_t setupMinute = 0U;
static uint8_t setupSecond = 0U;

static APP_TimeFieldTypeDef setupField = APP_FIELD_YEAR;

static FIL appSdFile;
static char appSdFilePath[40];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC3_Init(void);
static void MX_CRC_Init(void);
static void MX_DCMI_Init(void);
static void MX_DMA2D_Init(void);
static void MX_FMC_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C3_Init(void);
static void MX_LTDC_Init(void);
static void MX_QUADSPI_Init(void);
static void MX_RTC_Init(void);
static void MX_SAI2_Init(void);
static void MX_SDMMC1_SD_Init(void);
static void MX_SPDIFRX_Init(void);
static void MX_SPI2_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM5_Init(void);
static void MX_TIM8_Init(void);
static void MX_TIM12_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART6_UART_Init(void);
void StartDefaultTask(void const * argument);
static void APP_StorageTask(void const * argument);

/* USER CODE BEGIN PFP */

static void APP_Init(void);
static void APP_Run(void);
static void APP_BootMenu(void);
static FRESULT APP_ClearAllHistory(void);
static uint8_t APP_ConfirmClearHistory(void);
static void APP_NetworkInit(void);
static uint8_t APP_SyncNetworkTime(void);
static uint8_t APP_SyncCancelRequested(void);

static void APP_DrawInterface(void);
static void APP_DrawLeftPanel(void);
static void APP_DrawCurrentTime(uint8_t force);
static void APP_DrawSdStatus(uint8_t force);
static void APP_DrawMainMenuButton(void);
static void APP_DrawVariableMilkButton(uint8_t pressed);

static void APP_DrawMilkButton(
    uint8_t buttonIndex,
    uint8_t pressed);

static void APP_DrawAllMilkButtons(void);

static void APP_InitTodayState(void);
static void APP_AddMilkRecord(uint16_t amountMl);
static FRESULT APP_AppendMilkRecord(const APP_MilkRecordTypeDef *record);
static FRESULT APP_RecoverCsvUpgrade(void);
static FRESULT APP_RecoverSd(void);
static void APP_ProcessStorageEvents(void);
static FRESULT APP_LoadTodayRecords(void);
static void APP_InsertTodayRecord(const APP_MilkRecordTypeDef *record);
static FRESULT APP_DeleteRecord(uint32_t sourceOrdinal);
static void APP_DrawDeleteButton(void);
static void APP_GetHistoryPageRange(uint16_t *firstIndex,
                                    uint16_t *endIndex,
                                    uint16_t *totalPages);
static void APP_ProcessTouch(void);

static uint8_t APP_PointInRect(
    uint16_t touchX,
    uint16_t touchY,
    uint16_t rectX,
    uint16_t rectY,
    uint16_t rectWidth,
    uint16_t rectHeight);

static uint8_t APP_DaysInMonth(
    uint16_t year,
    uint8_t month);

static uint8_t APP_CalculateWeekDay(
    uint16_t year,
    uint8_t month,
    uint8_t day);

static uint8_t APP_RtcIsValid(void);

static FRESULT APP_SD_Test(void);
static void APP_ShowSdTestResult(FRESULT result);
static void APP_SD_CaptureHalState(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static uint8_t APP_PointInRect(
    uint16_t touchX,
    uint16_t touchY,
    uint16_t rectX,
    uint16_t rectY,
    uint16_t rectWidth,
    uint16_t rectHeight)
{
    if ((touchX >= rectX) &&
        (touchX < (rectX + rectWidth)) &&
        (touchY >= rectY) &&
        (touchY < (rectY + rectHeight)))
    {
        return 1U;
    }

    return 0U;
}


static uint8_t APP_DaysInMonth(
    uint16_t year,
    uint8_t month)
{
    static const uint8_t days[] =
    {
        31U, 28U, 31U, 30U, 31U, 30U,
        31U, 31U, 30U, 31U, 30U, 31U
    };

    uint8_t result;

    if ((month < 1U) || (month > 12U))
    {
        return 31U;
    }

    result = days[month - 1U];

    if (month == 2U)
    {
        if (((year % 400U) == 0U) ||
            (((year % 4U) == 0U) &&
             ((year % 100U) != 0U)))
        {
            result = 29U;
        }
    }

    return result;
}


/*
 * 返回值符合 STM32 RTC：
 * Monday = 1 ... Sunday = 7
 */
static uint8_t APP_CalculateWeekDay(
    uint16_t year,
    uint8_t month,
    uint8_t day)
{
    static const uint8_t table[] =
    {
        0U, 3U, 2U, 5U, 0U, 3U,
        5U, 1U, 4U, 6U, 2U, 4U
    };

    uint32_t weekDay;

    if (month < 3U)
    {
        year--;
    }

    weekDay =
        (year +
         (year / 4U) -
         (year / 100U) +
         (year / 400U) +
         table[month - 1U] +
         day) % 7U;

    if (weekDay == 0U)
    {
        return RTC_WEEKDAY_SUNDAY;
    }

    return (uint8_t)weekDay;
}


static uint8_t APP_RtcIsValid(void)
{
    RTC_TimeTypeDef rtcTime = {0};
    RTC_DateTypeDef rtcDate = {0};

    if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) != RTC_INIT_MAGIC)
    {
        return 0U;
    }
    if (HAL_RTC_GetTime(&hrtc, &rtcTime, RTC_FORMAT_BIN) != HAL_OK)
    {
        return 0U;
    }
    if (HAL_RTC_GetDate(&hrtc, &rtcDate, RTC_FORMAT_BIN) != HAL_OK)
    {
        return 0U;
    }

    return ((rtcDate.Month >= 1U) && (rtcDate.Month <= 12U) &&
            (rtcDate.Date >= 1U) && (rtcDate.Date <= 31U) &&
            (rtcTime.Hours <= 23U) && (rtcTime.Minutes <= 59U) &&
            (rtcTime.Seconds <= 59U)) ? 1U : 0U;
}

static void APP_LoadRtcTime(void)
{
    RTC_TimeTypeDef rtcTime = {0};
    RTC_DateTypeDef rtcDate = {0};

    /*
     * RTC 已经设置过时，使用当前 RTC 值作为界面初始值。
     */
    if (HAL_RTCEx_BKUPRead(
            &hrtc,
            RTC_BKP_DR0) == RTC_INIT_MAGIC)
    {
        if (HAL_RTC_GetTime(
                &hrtc,
                &rtcTime,
                RTC_FORMAT_BIN) == HAL_OK)
        {
            if (HAL_RTC_GetDate(
                    &hrtc,
                    &rtcDate,
                    RTC_FORMAT_BIN) == HAL_OK)
            {
                setupYear = 2000U + rtcDate.Year;
                setupMonth = rtcDate.Month;
                setupDay = rtcDate.Date;
                setupHour = rtcTime.Hours;
                setupMinute = rtcTime.Minutes;
                setupSecond = rtcTime.Seconds;

                return;
            }
        }
    }

    /*
     * RTC 尚未设置时使用默认值。
     */
    setupYear = 2026U;
    setupMonth = 1U;
    setupDay = 1U;
    setupHour = 0U;
    setupMinute = 0U;
    setupSecond = 0U;
}

static void APP_InitTodayState(void)
{
    RTC_TimeTypeDef rtcTime = {0};
    RTC_DateTypeDef rtcDate = {0};

    /*
     * STM32 RTC 必须先读时间，再读日期。
     */
    if (HAL_RTC_GetTime(
            &hrtc,
            &rtcTime,
            RTC_FORMAT_BIN) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_RTC_GetDate(
            &hrtc,
            &rtcDate,
            RTC_FORMAT_BIN) != HAL_OK)
    {
        Error_Handler();
    }

    appTodayYear = 2000U + rtcDate.Year;
    appTodayMonth = rtcDate.Month;
    appTodayDay = rtcDate.Date;

    appTodayTotalMl = 0U;
    appRecordCount = 0U;
    appHistoryPage = 0U;
    appSelectedRecordIndex = -1;
    appCsvRecordCount = 0U;
    appNextRecordId = 0U;
    appPendingCount = 0U;
}


static void APP_DrawSetupButton(
    uint16_t x,
    uint16_t y,
    uint16_t width,
    uint16_t height,
    const char *text)
{
    uint16_t textWidth;
    uint16_t textX;
    uint16_t textY;

    BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
    BSP_LCD_FillRect(x, y, width, height);

    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_DrawRect(x, y, width, height);

    BSP_LCD_SetFont(&Font16);
    BSP_LCD_SetBackColor(LCD_COLOR_BLUE);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);

    textWidth = (uint16_t)(strlen(text) * Font16.Width);

    if (textWidth < width)
    {
        textX = x + ((width - textWidth) / 2U);
    }
    else
    {
        textX = x + 2U;
    }

    textY = y + ((height - Font16.Height) / 2U);

    BSP_LCD_DisplayStringAt(
        textX,
        textY,
        (uint8_t *)text,
        LEFT_MODE
    );
}


static void APP_DrawTimeSetupScreen(void)
{
    static const char *fieldNames[] =
    {
        "YEAR",
        "MONTH",
        "DAY",
        "HOUR",
        "MINUTE",
        "SECOND"
    };

    char text[50];

    BSP_LCD_Clear(LCD_COLOR_WHITE);

    BSP_LCD_SetFont(&Font20);
    BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);

    BSP_LCD_DisplayStringAt(
        0U,
        15U,
        (uint8_t *)"Set date and time",
        CENTER_MODE
    );

    snprintf(
        text,
        sizeof(text),
        "%04u-%02u-%02u  %02u:%02u:%02u",
        setupYear,
        setupMonth,
        setupDay,
        setupHour,
        setupMinute,
        setupSecond
    );

    BSP_LCD_SetFont(&Font20);

    BSP_LCD_DisplayStringAt(
        0U,
        65U,
        (uint8_t *)text,
        CENTER_MODE
    );

    snprintf(
        text,
        sizeof(text),
        "Editing: %s",
        fieldNames[setupField]
    );

    BSP_LCD_SetFont(&Font16);

    BSP_LCD_DisplayStringAt(
        0U,
        115U,
        (uint8_t *)text,
        CENTER_MODE
    );

    APP_DrawSetupButton(
        SET_MINUS_X,
        SET_BUTTON_Y,
        SET_BUTTON_WIDTH,
        SET_BUTTON_HEIGHT,
        "-"
    );

    APP_DrawSetupButton(
        SET_PLUS_X,
        SET_BUTTON_Y,
        SET_BUTTON_WIDTH,
        SET_BUTTON_HEIGHT,
        "+"
    );

    APP_DrawSetupButton(
        SET_NEXT_X,
        SET_BUTTON_Y,
        SET_BUTTON_WIDTH,
        SET_BUTTON_HEIGHT,
        "NEXT"
    );

    APP_DrawSetupButton(
        SET_SAVE_X,
        SET_BUTTON_Y,
        SET_BUTTON_WIDTH,
        SET_BUTTON_HEIGHT,
        "SAVE"
    );
}


static void APP_AdjustTime(int8_t direction)
{
    uint8_t maximumDay;

    switch (setupField)
    {
        case APP_FIELD_YEAR:
            if (direction > 0)
            {
                setupYear++;

                if (setupYear > 2099U)
                {
                    setupYear = 2000U;
                }
            }
            else
            {
                if (setupYear <= 2000U)
                {
                    setupYear = 2099U;
                }
                else
                {
                    setupYear--;
                }
            }
            break;

        case APP_FIELD_MONTH:
            if (direction > 0)
            {
                setupMonth++;

                if (setupMonth > 12U)
                {
                    setupMonth = 1U;
                }
            }
            else
            {
                if (setupMonth <= 1U)
                {
                    setupMonth = 12U;
                }
                else
                {
                    setupMonth--;
                }
            }
            break;

        case APP_FIELD_DAY:
            maximumDay =
                APP_DaysInMonth(setupYear, setupMonth);

            if (direction > 0)
            {
                setupDay++;

                if (setupDay > maximumDay)
                {
                    setupDay = 1U;
                }
            }
            else
            {
                if (setupDay <= 1U)
                {
                    setupDay = maximumDay;
                }
                else
                {
                    setupDay--;
                }
            }
            break;

        case APP_FIELD_HOUR:
            if (direction > 0)
            {
                setupHour =
                    (uint8_t)((setupHour + 1U) % 24U);
            }
            else
            {
                setupHour =
                    (setupHour == 0U) ? 23U :
                                       setupHour - 1U;
            }
            break;

        case APP_FIELD_MINUTE:
            if (direction > 0)
            {
                setupMinute =
                    (uint8_t)((setupMinute + 1U) % 60U);
            }
            else
            {
                setupMinute =
                    (setupMinute == 0U) ? 59U :
                                         setupMinute - 1U;
            }
            break;

        case APP_FIELD_SECOND:
            if (direction > 0)
            {
                setupSecond =
                    (uint8_t)((setupSecond + 1U) % 60U);
            }
            else
            {
                setupSecond =
                    (setupSecond == 0U) ? 59U :
                                         setupSecond - 1U;
            }
            break;

        default:
            break;
    }

    /*
     * 调整年份或月份后，避免出现 2 月 31 日。
     */
    maximumDay = APP_DaysInMonth(
        setupYear,
        setupMonth
    );

    if (setupDay > maximumDay)
    {
        setupDay = maximumDay;
    }
}


static HAL_StatusTypeDef APP_SaveRtcTime(void)
{
    RTC_TimeTypeDef rtcTime = {0};
    RTC_DateTypeDef rtcDate = {0};

    rtcTime.Hours = setupHour;
    rtcTime.Minutes = setupMinute;
    rtcTime.Seconds = setupSecond;
    rtcTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    rtcTime.StoreOperation = RTC_STOREOPERATION_RESET;

    if (HAL_RTC_SetTime(
            &hrtc,
            &rtcTime,
            RTC_FORMAT_BIN) != HAL_OK)
    {
        return HAL_ERROR;
    }

    rtcDate.WeekDay = APP_CalculateWeekDay(
        setupYear,
        setupMonth,
        setupDay
    );

    rtcDate.Month = setupMonth;
    rtcDate.Date = setupDay;
    rtcDate.Year = (uint8_t)(setupYear - 2000U);

    if (HAL_RTC_SetDate(
            &hrtc,
            &rtcDate,
            RTC_FORMAT_BIN) != HAL_OK)
    {
        return HAL_ERROR;
    }

    HAL_RTCEx_BKUPWrite(
        &hrtc,
        RTC_BKP_DR0,
        RTC_INIT_MAGIC
    );

    return HAL_OK;
}


static void APP_TimeSetupScreen(void)
{
    TS_StateTypeDef touchState = {0};

    uint8_t touching;
    uint8_t previousTouching = 0U;
    uint8_t setupFinished = 0U;

    uint16_t touchX;
    uint16_t touchY;

    APP_LoadRtcTime();
    setupField = APP_FIELD_YEAR;

    APP_DrawTimeSetupScreen();

    while (setupFinished == 0U)
    {
        if (BSP_TS_GetState(&touchState) == TS_OK)
        {
            touching =
                (touchState.touchDetected > 0U) ?
                1U : 0U;

            /*
             * 只处理刚按下的瞬间。
             */
            if ((touching != 0U) &&
                (previousTouching == 0U))
            {
                touchX = touchState.touchX[0];
                touchY = touchState.touchY[0];

                if (APP_PointInRect(
                        touchX,
                        touchY,
                        SET_MINUS_X,
                        SET_BUTTON_Y,
                        SET_BUTTON_WIDTH,
                        SET_BUTTON_HEIGHT) != 0U)
                {
                    APP_AdjustTime(-1);
                    APP_DrawTimeSetupScreen();
                }
                else if (APP_PointInRect(
                             touchX,
                             touchY,
                             SET_PLUS_X,
                             SET_BUTTON_Y,
                             SET_BUTTON_WIDTH,
                             SET_BUTTON_HEIGHT) != 0U)
                {
                    APP_AdjustTime(1);
                    APP_DrawTimeSetupScreen();
                }
                else if (APP_PointInRect(
                             touchX,
                             touchY,
                             SET_NEXT_X,
                             SET_BUTTON_Y,
                             SET_BUTTON_WIDTH,
                             SET_BUTTON_HEIGHT) != 0U)
                {
                    setupField =
                        (APP_TimeFieldTypeDef)
                        ((setupField + 1U) %
                         APP_FIELD_COUNT);

                    APP_DrawTimeSetupScreen();
                }
                else if (APP_PointInRect(
                             touchX,
                             touchY,
                             SET_SAVE_X,
                             SET_BUTTON_Y,
                             SET_BUTTON_WIDTH,
                             SET_BUTTON_HEIGHT) != 0U)
                {
                    if (APP_SaveRtcTime() == HAL_OK)
                    {
                        BSP_LCD_Clear(LCD_COLOR_WHITE);

                        BSP_LCD_SetFont(&Font20);
                        BSP_LCD_SetBackColor(
                            LCD_COLOR_WHITE);
                        BSP_LCD_SetTextColor(
                            LCD_COLOR_GREEN);

                        BSP_LCD_DisplayStringAt(
                            0U,
                            110U,
                            (uint8_t *)"RTC SAVED",
                            CENTER_MODE
                        );

                        HAL_Delay(500);
                        setupFinished = 1U;
                    }
                }
            }

            previousTouching = touching;
        }

        HAL_Delay(20);
    }
}

static FRESULT APP_ClearAllHistory(void)
{
    FIL file;
    FRESULT result;
    UINT written = 0U;
    static const char header[] = "date,time,amount_ml,record_id\r\n";

    if (appSdReady == 0U)
    {
        return FR_NOT_READY;
    }

    result = f_open(&file, appSdFilePath, FA_CREATE_ALWAYS | FA_WRITE);
    if (result != FR_OK)
    {
        return result;
    }
    if (result == FR_OK)
    {
        result = f_write(&file, header, sizeof(header) - 1U, &written);
        if ((result == FR_OK) && (written != (sizeof(header) - 1U)))
        {
            result = FR_DISK_ERR;
        }
    }
    if (result == FR_OK)
    {
        result = f_sync(&file);
    }
    if (result == FR_OK)
    {
        result = f_close(&file);
    }
    else
    {
        (void)f_close(&file);
    }

    if (result == FR_OK)
    {
        APP_InitTodayState();
    }
    return result;
}

static uint8_t APP_ConfirmClearHistory(void)
{
    TS_StateTypeDef state = {0};
    uint8_t previous = 0U;

    BSP_LCD_Clear(LCD_COLOR_WHITE);
    BSP_LCD_SetFont(&Font20);
    BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
    BSP_LCD_SetTextColor(LCD_COLOR_RED);
    BSP_LCD_DisplayStringAt(0U, 55U,
        (uint8_t *)"DELETE ALL HISTORY?", CENTER_MODE);
    BSP_LCD_SetFont(&Font16);
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_DisplayStringAt(0U, 105U,
        (uint8_t *)"This cannot be undone", CENTER_MODE);
    APP_DrawSetupButton(80U, 175U, 130U, 60U, "CANCEL");
    APP_DrawSetupButton(270U, 175U, 130U, 60U, "DELETE");

    do
    {
        (void)BSP_TS_GetState(&state);
        osDelay(20U);
    } while (state.touchDetected > 0U);

    for (;;)
    {
        if (BSP_TS_GetState(&state) == TS_OK)
        {
            uint8_t touching = (state.touchDetected > 0U) ? 1U : 0U;
            if ((touching != 0U) && (previous == 0U))
            {
                uint16_t x = state.touchX[0];
                uint16_t y = state.touchY[0];
                if (APP_PointInRect(x, y, 80U, 175U, 130U, 60U) != 0U)
                {
                    return 0U;
                }
                if (APP_PointInRect(x, y, 270U, 175U, 130U, 60U) != 0U)
                {
                    return 1U;
                }
            }
            previous = touching;
        }
        osDelay(20U);
    }
}

void APP_SNTP_SetUnixTime(unsigned long seconds)
{
    int64_t localSeconds = (int64_t)seconds + (8LL * 60LL * 60LL);
    int64_t days = localSeconds / 86400LL;
    uint32_t daySeconds = (uint32_t)(localSeconds % 86400LL);
    int64_t z = days + 719468LL;
    int64_t era = (z >= 0LL ? z : z - 146096LL) / 146097LL;
    uint32_t doe = (uint32_t)(z - era * 146097LL);
    uint32_t yoe = (doe - doe / 1460U + doe / 36524U - doe / 146096U) / 365U;
    int32_t year = (int32_t)yoe + (int32_t)(era * 400LL);
    uint32_t doy = doe - (365U * yoe + yoe / 4U - yoe / 100U);
    uint32_t mp = (5U * doy + 2U) / 153U;
    uint32_t day = doy - (153U * mp + 2U) / 5U + 1U;
    uint32_t month = (uint32_t)((int32_t)mp + ((mp < 10U) ? 3 : -9));
    RTC_TimeTypeDef rtcTime = {0};
    RTC_DateTypeDef rtcDate = {0};

    year += (month <= 2U) ? 1 : 0;
    if ((year < 2000) || (year > 2099))
    {
        return;
    }

    rtcTime.Hours = (uint8_t)(daySeconds / 3600U);
    rtcTime.Minutes = (uint8_t)((daySeconds % 3600U) / 60U);
    rtcTime.Seconds = (uint8_t)(daySeconds % 60U);
    rtcTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    rtcTime.StoreOperation = RTC_STOREOPERATION_RESET;
    rtcDate.Year = (uint8_t)(year - 2000);
    rtcDate.Month = (uint8_t)month;
    rtcDate.Date = (uint8_t)day;
    rtcDate.WeekDay = APP_CalculateWeekDay((uint16_t)year,
                                           (uint8_t)month,
                                           (uint8_t)day);

    if ((HAL_RTC_SetTime(&hrtc, &rtcTime, RTC_FORMAT_BIN) == HAL_OK) &&
        (HAL_RTC_SetDate(&hrtc, &rtcDate, RTC_FORMAT_BIN) == HAL_OK))
    {
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, RTC_INIT_MAGIC);
        appTimeSynchronized = 1U;
    }
}

static void APP_NetworkInit(void)
{
    ip_addr_t ipaddr;
    ip_addr_t netmask;
    ip_addr_t gateway;

    if (appNetworkInitialized != 0U)
    {
        return;
    }

    ip_addr_set_zero_ip4(&ipaddr);
    ip_addr_set_zero_ip4(&netmask);
    ip_addr_set_zero_ip4(&gateway);
    tcpip_init(NULL, NULL);
    netif_add(&appNetif, &ipaddr, &netmask, &gateway, NULL,
              ethernetif_init, tcpip_input);
    netif_set_default(&appNetif);
    netif_set_up(&appNetif);

    osThreadDef(EthLink, ethernet_link_thread, osPriorityNormal, 0,
                configMINIMAL_STACK_SIZE * 2U);
    (void)osThreadCreate(osThread(EthLink), &appNetif);
    (void)netifapi_dhcp_start(&appNetif);
    appNetworkInitialized = 1U;
}

static uint8_t APP_SyncCancelRequested(void)
{
    TS_StateTypeDef state = {0};

    if ((BSP_TS_GetState(&state) == TS_OK) &&
        (state.touchDetected > 0U) &&
        (APP_PointInRect(state.touchX[0], state.touchY[0],
                         APP_SYNC_CANCEL_X, APP_SYNC_CANCEL_Y,
                         APP_SYNC_CANCEL_WIDTH,
                         APP_SYNC_CANCEL_HEIGHT) != 0U))
    {
        return 1U;
    }
    return 0U;
}

static uint8_t APP_SyncNetworkTime(void)
{
    uint32_t started;
    TS_StateTypeDef state = {0};

    APP_NetworkInit();

    /* Do not treat the menu tap that opened this page as CANCEL. */
    do
    {
        (void)BSP_TS_GetState(&state);
        osDelay(20U);
    } while (state.touchDetected > 0U);

    BSP_LCD_SetFont(&Font16);
    BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_DisplayStringAt(0U, 145U,
        (uint8_t *)"Waiting for cable / DHCP...", CENTER_MODE);

    started = HAL_GetTick();
    while ((HAL_GetTick() - started) < 20000U)
    {
        if (APP_SyncCancelRequested() != 0U)
        {
            return 2U;
        }
        if (netif_is_link_up(&appNetif) &&
            !ip4_addr_isany_val(*netif_ip4_addr(&appNetif)))
        {
            break;
        }
        osDelay(100U);
    }

    if (!netif_is_link_up(&appNetif) ||
        ip4_addr_isany_val(*netif_ip4_addr(&appNetif)))
    {
        return 0U;
    }

    {
        char ipText[40];
        BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
        BSP_LCD_FillRect(0U, 140U, BSP_LCD_GetXSize(), 25U);
        snprintf(ipText, sizeof(ipText), "IP %s - contacting NTP",
                 ip4addr_ntoa(netif_ip4_addr(&appNetif)));
        BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
        BSP_LCD_DisplayStringAt(0U, 145U,
                                (uint8_t *)ipText, CENTER_MODE);
    }

    appTimeSynchronized = 0U;
    sntp_stop();
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0U, "pool.ntp.org");
    sntp_init();

    started = HAL_GetTick();
    while (((HAL_GetTick() - started) < 15000U) &&
           (appTimeSynchronized == 0U))
    {
        if (APP_SyncCancelRequested() != 0U)
        {
            sntp_stop();
            return 2U;
        }
        osDelay(100U);
    }
    return appTimeSynchronized;
}

static void APP_BootMenu(void)
{
    TS_StateTypeDef state = {0};
    uint8_t previous = 0U;
    char connectionStatus[64];

    for (;;)
    {
        BSP_LCD_Clear(LCD_COLOR_WHITE);
        BSP_LCD_SetFont(&Font24);
        BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
        BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
        BSP_LCD_DisplayStringAt(0U, 20U,
            (uint8_t *)"MILK LOG", CENTER_MODE);

        APP_DrawSetupButton(APP_MENU_LEFT_X, APP_MENU_TOP_Y,
            APP_MENU_BUTTON_WIDTH, APP_MENU_BUTTON_HEIGHT, "START");
        APP_DrawSetupButton(APP_MENU_RIGHT_X, APP_MENU_TOP_Y,
            APP_MENU_BUTTON_WIDTH, APP_MENU_BUTTON_HEIGHT, "SET TIME");
        APP_DrawSetupButton(APP_MENU_LEFT_X, APP_MENU_BOTTOM_Y,
            APP_MENU_BUTTON_WIDTH, APP_MENU_BUTTON_HEIGHT, "SYNC TIME");
        APP_DrawSetupButton(APP_MENU_RIGHT_X, APP_MENU_BOTTOM_Y,
            APP_MENU_BUTTON_WIDTH, APP_MENU_BUTTON_HEIGHT, "CLEAR HISTORY");

        BSP_LCD_SetFont(&Font16);
        BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
        if (appNetworkInitialized == 0U)
        {
            snprintf(connectionStatus, sizeof(connectionStatus),
                     "%s   NET: OFF",
                     (appSdReady != 0U) ? "SD: OK" : "SD: ERROR");
            BSP_LCD_SetTextColor(LCD_COLOR_DARKYELLOW);
        }
        else if (!netif_is_link_up(&appNetif))
        {
            snprintf(connectionStatus, sizeof(connectionStatus),
                     "%s   NET: NO CABLE",
                     (appSdReady != 0U) ? "SD: OK" : "SD: ERROR");
            BSP_LCD_SetTextColor(LCD_COLOR_RED);
        }
        else if (ip4_addr_isany_val(*netif_ip4_addr(&appNetif)))
        {
            snprintf(connectionStatus, sizeof(connectionStatus),
                     "%s   NET: DHCP...",
                     (appSdReady != 0U) ? "SD: OK" : "SD: ERROR");
            BSP_LCD_SetTextColor(LCD_COLOR_ORANGE);
        }
        else
        {
            snprintf(connectionStatus, sizeof(connectionStatus),
                     "%s   NET: %s",
                     (appSdReady != 0U) ? "SD: OK" : "SD: ERROR",
                     ip4addr_ntoa(netif_ip4_addr(&appNetif)));
            BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
        }
        BSP_LCD_DisplayStringAt(0U, 245U,
            (uint8_t *)connectionStatus, CENTER_MODE);

        do
        {
            (void)BSP_TS_GetState(&state);
            osDelay(20U);
        } while (state.touchDetected > 0U);
        previous = 0U;
        for (;;)
        {
            if (BSP_TS_GetState(&state) == TS_OK)
            {
                uint8_t touching = (state.touchDetected > 0U) ? 1U : 0U;
                if ((touching != 0U) && (previous == 0U))
                {
                    uint16_t x = state.touchX[0];
                    uint16_t y = state.touchY[0];
                    if (APP_PointInRect(x, y, APP_MENU_LEFT_X,
                            APP_MENU_TOP_Y, APP_MENU_BUTTON_WIDTH,
                            APP_MENU_BUTTON_HEIGHT) != 0U)
                    {
                        return;
                    }
                    if (APP_PointInRect(x, y, APP_MENU_RIGHT_X,
                            APP_MENU_TOP_Y, APP_MENU_BUTTON_WIDTH,
                            APP_MENU_BUTTON_HEIGHT) != 0U)
                    {
                        APP_TimeSetupScreen();
                        break;
                    }
                    if (APP_PointInRect(x, y, APP_MENU_LEFT_X,
                            APP_MENU_BOTTOM_Y, APP_MENU_BUTTON_WIDTH,
                            APP_MENU_BUTTON_HEIGHT) != 0U)
                    {
                        uint8_t synced;
                        BSP_LCD_Clear(LCD_COLOR_WHITE);
                        BSP_LCD_SetFont(&Font20);
                        BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
                        BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
                        BSP_LCD_DisplayStringAt(0U, 110U,
                            (uint8_t *)"CONNECTING / SYNCING...",
                            CENTER_MODE);
                        APP_DrawSetupButton(APP_SYNC_CANCEL_X,
                            APP_SYNC_CANCEL_Y, APP_SYNC_CANCEL_WIDTH,
                            APP_SYNC_CANCEL_HEIGHT, "CANCEL");
                        synced = APP_SyncNetworkTime();
                        BSP_LCD_Clear(LCD_COLOR_WHITE);
                        BSP_LCD_SetTextColor((synced == 1U) ?
                                             LCD_COLOR_GREEN :
                                             ((synced == 2U) ?
                                              LCD_COLOR_ORANGE :
                                              LCD_COLOR_RED));
                        BSP_LCD_DisplayStringAt(0U, 110U,
                            (uint8_t *)((synced == 1U) ?
                                "TIME SYNCED" :
                                ((synced == 2U) ?
                                 "SYNC CANCELLED" : "SYNC FAILED")),
                            CENTER_MODE);
                        osDelay(1200U);
                        break;
                    }
                    if (APP_PointInRect(x, y, APP_MENU_RIGHT_X,
                            APP_MENU_BOTTOM_Y, APP_MENU_BUTTON_WIDTH,
                            APP_MENU_BUTTON_HEIGHT) != 0U)
                    {
                        if ((APP_ConfirmClearHistory() != 0U) &&
                            (APP_ClearAllHistory() == FR_OK))
                        {
                            BSP_LCD_Clear(LCD_COLOR_WHITE);
                            BSP_LCD_SetFont(&Font20);
                            BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
                            BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
                            BSP_LCD_DisplayStringAt(0U, 110U,
                                (uint8_t *)"HISTORY CLEARED", CENTER_MODE);
                            osDelay(800U);
                        }
                        break;
                    }
                }
                previous = touching;
            }
            osDelay(20U);
        }
    }
}

static void APP_Init(void)
{
    FRESULT sdResult;
    /*
     * 初始化 LCD。
     */
    if (BSP_LCD_Init() != LCD_OK)
    {
        Error_Handler();
    }

    /*
     * LCD framebuffer 位于外部 SDRAM。
     */
    BSP_LCD_LayerDefaultInit(
        0U,
        LCD_FB_START_ADDRESS
    );

    BSP_LCD_SelectLayer(0U);
    BSP_LCD_SetLayerVisible(0U, ENABLE);
    BSP_LCD_DisplayOn();

    /*
     * 初始化触摸屏。
     */
    if (BSP_TS_Init(
            BSP_LCD_GetXSize(),
            BSP_LCD_GetYSize()) != TS_OK)
    {
        BSP_LCD_Clear(LCD_COLOR_WHITE);

        BSP_LCD_SetFont(&Font20);
        BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
        BSP_LCD_SetTextColor(LCD_COLOR_RED);

        BSP_LCD_DisplayStringAt(
            0U,
            110U,
            (uint8_t *)"Touch init failed",
            CENTER_MODE
        );

        Error_Handler();
    }

if (APP_RtcIsValid() == 0U)
{
    APP_TimeSetupScreen();
}

/*
 * 初始化、挂载并创建 CSV 文件。
 */
sdResult = APP_SD_Test();
appSdReady = (sdResult == FR_OK) ? 1U : 0U;

/*
 * 显示测试结果。
 */
if (sdResult != FR_OK)
{
    APP_ShowSdTestResult(sdResult);
}

APP_BootMenu();

/*
 * SD 测试完成后，进入六个奶量按钮界面。
 */
APP_InitTodayState();
(void)APP_LoadTodayRecords();
APP_DrawInterface();

appLastPressTick =
    HAL_GetTick() - APP_TOUCH_DEBOUNCE_MS;
}


static void APP_Run(void)
{
    while (1)
    {
        if (BSP_SD_IsDetected() != SD_PRESENT)
        {
            appSdRemovalSeen = 1U;
            if (appSdReady != 0U)
            {
                appSdReady = 0U;
                appStorageStatusVersion++;
            }
        }
        APP_ProcessStorageEvents();
        APP_ProcessTouch();
        APP_DrawCurrentTime(0U);
        APP_DrawSdStatus(0U);
        osDelay(10U);
    }
}

static void APP_DrawInterface(void)
{
    BSP_LCD_Clear(LCD_COLOR_WHITE);

    /*
     * 左右分隔线。
     */
    BSP_LCD_SetTextColor(LCD_COLOR_GRAY);

    BSP_LCD_DrawVLine(
        APP_SEPARATOR_X,
        5U,
        BSP_LCD_GetYSize() - 10U
    );

    APP_DrawLeftPanel();
    APP_DrawAllMilkButtons();
    APP_DrawVariableMilkButton(0U);
    APP_DrawDeleteButton();

    APP_DrawCurrentTime(1U);

    APP_DrawSdStatus(1U);
}

static void APP_GetHistoryPageRange(uint16_t *firstIndex,
                                    uint16_t *endIndex,
                                    uint16_t *totalPages)
{
    uint16_t todayCount = 0U;
    uint16_t todayShown;
    uint16_t historyCount;

    while (todayCount < appRecordCount)
    {
        const APP_MilkRecordTypeDef *record =
            &appMilkRecords[appRecordCount - 1U - todayCount];
        if ((record->year != appTodayYear) ||
            (record->month != appTodayMonth) ||
            (record->day != appTodayDay))
        {
            break;
        }
        todayCount++;
    }

    todayShown = (todayCount > APP_RECORDS_PER_PAGE) ?
        APP_RECORDS_PER_PAGE : todayCount;
    historyCount = appRecordCount - todayShown;
    *totalPages = (uint16_t)(1U +
        ((historyCount + APP_RECORDS_PER_PAGE - 1U) /
         APP_RECORDS_PER_PAGE));

    if (appHistoryPage == 0U)
    {
        *endIndex = appRecordCount;
        *firstIndex = appRecordCount - todayShown;
    }
    else
    {
        uint16_t offset = (uint16_t)
            ((appHistoryPage - 1U) * APP_RECORDS_PER_PAGE);
        *endIndex = (historyCount > offset) ?
            (uint16_t)(historyCount - offset) : 0U;
        *firstIndex = (*endIndex > APP_RECORDS_PER_PAGE) ?
            (uint16_t)(*endIndex - APP_RECORDS_PER_PAGE) : 0U;
    }
}

static void APP_DrawCurrentTime(uint8_t force)
{
    static uint8_t lastSecond = 0xFFU;
    RTC_TimeTypeDef rtcTime = {0};
    RTC_DateTypeDef rtcDate = {0};
    char text[12];

    if (HAL_RTC_GetTime(&hrtc, &rtcTime, RTC_FORMAT_BIN) != HAL_OK)
    {
        return;
    }
    /* Reading the date unlocks the RTC shadow registers for the next read. */
    if (HAL_RTC_GetDate(&hrtc, &rtcDate, RTC_FORMAT_BIN) != HAL_OK)
    {
        return;
    }
    if ((force == 0U) && (rtcTime.Seconds == lastSecond))
    {
        return;
    }
    lastSecond = rtcTime.Seconds;

    snprintf(text, sizeof(text), "%02u:%02u:%02u",
             rtcTime.Hours, rtcTime.Minutes, rtcTime.Seconds);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_FillRect(292U, 2U, 105U, 22U);
    BSP_LCD_SetFont(&Font16);
    BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_DisplayStringAt(300U, 5U, (uint8_t *)text, LEFT_MODE);
}

static void APP_DrawSdStatus(uint8_t force)
{
    static uint32_t lastVersion = 0xFFFFFFFFU;
    static uint8_t lastPending = 0xFFU;
    char text[11];
    uint32_t color;

    if ((force == 0U) &&
        (lastVersion == appStorageStatusVersion) &&
        (lastPending == appPendingCount))
    {
        return;
    }

    lastVersion = appStorageStatusVersion;
    lastPending = appPendingCount;

    if (appStorageQueueFull != 0U)
    {
        snprintf(text, sizeof(text), "QUEUE FULL");
        color = LCD_COLOR_RED;
    }
    else if (appStorageFault != 0U)
    {
        snprintf(text, sizeof(text), "WAIT P:%u", appPendingCount);
        color = APP_COLOR_PENDING;
    }
    else if (appPendingCount != 0U)
    {
        snprintf(text, sizeof(text), "SAVE P:%u", appPendingCount);
        color = APP_COLOR_PENDING;
    }
    else if (appSdReady != 0U)
    {
        snprintf(text, sizeof(text), "SD OK");
        color = LCD_COLOR_GREEN;
    }
    else
    {
        snprintf(text, sizeof(text), "SD ERROR");
        color = LCD_COLOR_RED;
    }

    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_FillRect(400U, 2U, 80U, 22U);
    BSP_LCD_SetFont(&Font16);
    BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
    BSP_LCD_SetTextColor(color);
    BSP_LCD_DisplayStringAt(400U, 5U, (uint8_t *)text, LEFT_MODE);
}

static void APP_DrawLeftPanel(void)
{
    char text[40];
    uint16_t i;
    uint16_t firstIndex;
    uint16_t endIndex;
    uint16_t totalPages;
    uint16_t displayYear = appTodayYear;
    uint8_t displayMonth = appTodayMonth;
    uint8_t displayDay = appTodayDay;
    uint32_t displayTotal = 0U;
    uint16_t displayFeeds = 0U;
    const APP_MilkRecordTypeDef *displayLast = NULL;

    APP_GetHistoryPageRange(&firstIndex, &endIndex, &totalPages);
    if (appHistoryPage >= totalPages)
    {
        appHistoryPage = totalPages - 1U;
        APP_GetHistoryPageRange(&firstIndex, &endIndex, &totalPages);
    }

    if ((appHistoryPage > 0U) && (endIndex > firstIndex))
    {
        const APP_MilkRecordTypeDef *latest = &appMilkRecords[endIndex - 1U];
        displayYear = latest->year;
        displayMonth = latest->month;
        displayDay = latest->day;
    }

    {
        uint16_t j;
        for (j = 0U; j < appRecordCount; j++)
        {
            if ((appMilkRecords[j].year == displayYear) &&
                (appMilkRecords[j].month == displayMonth) &&
                (appMilkRecords[j].day == displayDay))
            {
                displayTotal += appMilkRecords[j].amountMl;
                displayFeeds++;
                displayLast = &appMilkRecords[j];
            }
        }
    }

    /*
     * 清除整个左侧区域。
     */
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);

    BSP_LCD_FillRect(
        0U,
        0U,
        APP_SEPARATOR_X - 2U,
        BSP_LCD_GetYSize()
    );

    BSP_LCD_SetFont(&Font16);
    BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);

    snprintf(
        text,
        sizeof(text),
        "Date: %04u-%02u-%02u",
        displayYear,
        displayMonth,
        displayDay
    );

    BSP_LCD_DisplayStringAt(
        APP_RECORD_X,
        8U,
        (uint8_t *)text,
        LEFT_MODE
    );

    snprintf(
        text,
        sizeof(text),
        "Total:%lu ml Feed:%u",
        (unsigned long)displayTotal,
        (unsigned int)displayFeeds
    );

    BSP_LCD_DisplayStringAt(
        APP_RECORD_X,
        29U,
        (uint8_t *)text,
        LEFT_MODE
    );

    if (displayLast != NULL)
    {
        snprintf(text, sizeof(text), "Last: %02u:%02u  %u ml",
                 displayLast->hour, displayLast->minute,
                 displayLast->amountMl);
    }
    else
    {
        snprintf(text, sizeof(text), "Last: --:--  -- ml");
    }
    BSP_LCD_DisplayStringAt(APP_RECORD_X, 50U,
                            (uint8_t *)text, LEFT_MODE);

    if (appHistoryPage == 0U)
    {
        snprintf(text, sizeof(text), "TODAY RECORDS");
    }
    else
    {
        snprintf(text, sizeof(text), "HISTORY  Page %u/%u",
                 (unsigned int)appHistoryPage,
                 (unsigned int)(totalPages - 1U));
    }
    BSP_LCD_DisplayStringAt(APP_RECORD_X, 69U, (uint8_t *)text, LEFT_MODE);

    for (i = 0U; i < (endIndex - firstIndex); i++)
    {
        uint16_t recordIndex = (uint16_t)(endIndex - 1U - i);

        BSP_LCD_SetBackColor(
            ((int16_t)recordIndex == appSelectedRecordIndex) ?
            LCD_COLOR_YELLOW : LCD_COLOR_WHITE);
        BSP_LCD_SetTextColor(
            (appMilkRecords[recordIndex].pending != 0U) ?
            APP_COLOR_PENDING : LCD_COLOR_BLACK);
        snprintf(
            text,
            sizeof(text),
            "%02u-%02u %02u:%02u  %3u ml%s",
            appMilkRecords[recordIndex].month,
            appMilkRecords[recordIndex].day,
            appMilkRecords[recordIndex].hour,
            appMilkRecords[recordIndex].minute,
            appMilkRecords[recordIndex].amountMl,
            (appMilkRecords[recordIndex].pending != 0U) ? " *" : ""
        );

        BSP_LCD_DisplayStringAt(
            APP_RECORD_X,
            APP_RECORD_Y +
                (i *
                 APP_RECORD_LINE_HEIGHT),
            (uint8_t *)text,
            LEFT_MODE
        );
    }

    BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
    APP_DrawMainMenuButton();
}

static void APP_DrawMainMenuButton(void)
{
    uint8_t canOpen =
        ((appPendingCount == 0U) && (appStorageFault == 0U)) ? 1U : 0U;
    uint32_t color = (canOpen != 0U) ? LCD_COLOR_BLUE : LCD_COLOR_GRAY;

    BSP_LCD_SetTextColor(color);
    BSP_LCD_FillRect(APP_MAIN_MENU_X, APP_MAIN_MENU_Y,
                     APP_MAIN_MENU_WIDTH, APP_MAIN_MENU_HEIGHT);
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_DrawRect(APP_MAIN_MENU_X, APP_MAIN_MENU_Y,
                     APP_MAIN_MENU_WIDTH, APP_MAIN_MENU_HEIGHT);
    BSP_LCD_SetFont(&Font16);
    BSP_LCD_SetBackColor(color);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_DisplayStringAt(APP_MAIN_MENU_X + 10U,
                            APP_MAIN_MENU_Y + 4U,
                            (uint8_t *)"MENU", LEFT_MODE);
}

static void APP_DrawDeleteButton(void)
{
    uint8_t canDelete =
        ((appSelectedRecordIndex >= 0) &&
         (appMilkRecords[appSelectedRecordIndex].pending == 0U) &&
         (appPendingCount == 0U) &&
         (appStorageFault == 0U)) ? 1U : 0U;
    uint32_t color = (canDelete != 0U) ? LCD_COLOR_RED : LCD_COLOR_GRAY;

    BSP_LCD_SetTextColor(color);
    BSP_LCD_FillRect(APP_DELETE_X, APP_DELETE_Y,
                     APP_DELETE_WIDTH, APP_DELETE_HEIGHT);
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_DrawRect(APP_DELETE_X, APP_DELETE_Y,
                     APP_DELETE_WIDTH, APP_DELETE_HEIGHT);
    BSP_LCD_SetFont(&Font16);
    BSP_LCD_SetBackColor(color);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_DisplayStringAt(APP_DELETE_X + 9U, APP_DELETE_Y + 18U,
                            (uint8_t *)"DELETE", LEFT_MODE);
}

static void APP_DrawVariableMilkButton(uint8_t pressed)
{
    char label[8];
    uint16_t textWidth;
    uint16_t textX;
    uint32_t color = (pressed != 0U) ?
        LCD_COLOR_DARKBLUE : LCD_COLOR_BLUE;

    snprintf(label, sizeof(label), "%uml",
             (unsigned int)appVariableAmountMl);
    textWidth = (uint16_t)(strlen(label) * Font16.Width);
    textX = APP_VARIABLE_X +
        ((APP_VARIABLE_WIDTH - textWidth) / 2U);

    BSP_LCD_SetTextColor(color);
    BSP_LCD_FillRect(APP_VARIABLE_X, APP_VARIABLE_Y,
                     APP_VARIABLE_WIDTH, APP_VARIABLE_HEIGHT);
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_DrawRect(APP_VARIABLE_X, APP_VARIABLE_Y,
                     APP_VARIABLE_WIDTH, APP_VARIABLE_HEIGHT);
    BSP_LCD_SetFont(&Font16);
    BSP_LCD_SetBackColor(color);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_DisplayStringAt(textX, APP_VARIABLE_Y + 18U,
                            (uint8_t *)label, LEFT_MODE);
}

static void APP_DrawMilkButton(
    uint8_t buttonIndex,
    uint8_t pressed)
{
    const APP_MilkButtonTypeDef *button;
    uint32_t backgroundColor;

    uint16_t textWidth;
    uint16_t textX;
    uint16_t textY;

    if (buttonIndex >= APP_MILK_BUTTON_COUNT)
    {
        return;
    }

    button = &appMilkButtons[buttonIndex];

    backgroundColor =
        (pressed != 0U) ?
        LCD_COLOR_DARKBLUE :
        LCD_COLOR_BLUE;

    BSP_LCD_SetTextColor(backgroundColor);

    BSP_LCD_FillRect(
        button->x,
        button->y,
        button->width,
        button->height
    );

    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);

    BSP_LCD_DrawRect(
        button->x,
        button->y,
        button->width,
        button->height
    );

    BSP_LCD_SetFont(&Font16);
    BSP_LCD_SetBackColor(backgroundColor);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);

    textWidth =
        (uint16_t)(strlen(button->label) *
                   Font16.Width);

    textX =
        button->x +
        ((button->width - textWidth) / 2U);

    textY =
        button->y +
        ((button->height - Font16.Height) / 2U);

    BSP_LCD_DisplayStringAt(
        textX,
        textY,
        (uint8_t *)button->label,
        LEFT_MODE
    );
}


static void APP_DrawAllMilkButtons(void)
{
    uint8_t i;

    for (i = 0U;
         i < APP_MILK_BUTTON_COUNT;
         i++)
    {
        APP_DrawMilkButton(i, 0U);
    }
}




static void APP_AddMilkRecord(uint16_t amountMl)
{
    RTC_TimeTypeDef rtcTime = {0};
    RTC_DateTypeDef rtcDate = {0};

    uint16_t currentYear;
    APP_MilkRecordTypeDef record;
    APP_MilkRecordTypeDef *queuedRecord;

    if (HAL_RTC_GetTime(
            &hrtc,
            &rtcTime,
            RTC_FORMAT_BIN) != HAL_OK)
    {
        return;
    }

    if (HAL_RTC_GetDate(
            &hrtc,
            &rtcDate,
            RTC_FORMAT_BIN) != HAL_OK)
    {
        return;
    }

    currentYear = 2000U + rtcDate.Year;

    /*
     * 检测日期是否已经改变。
     */
    if ((currentYear != appTodayYear) ||
        (rtcDate.Month != appTodayMonth) ||
        (rtcDate.Date != appTodayDay))
    {
        appTodayYear = currentYear;
        appTodayMonth = rtcDate.Month;
        appTodayDay = rtcDate.Date;

        appTodayTotalMl = 0U;
        appHistoryPage = 0U;
    }

    record.year = currentYear;
    record.month = rtcDate.Month;
    record.day = rtcDate.Date;
    record.hour = rtcTime.Hours;
    record.minute = rtcTime.Minutes;
    record.second = rtcTime.Seconds;
    record.amountMl = amountMl;
    record.sourceOrdinal = 0U;
    record.pending = 1U;

    if (appPendingCount >= APP_PENDING_MAX_RECORDS)
    {
        appStorageQueueFull = 1U;
        appStorageStatusVersion++;
        return;
    }

    appNextRecordId++;
    if (appNextRecordId == 0U)
    {
        appNextRecordId = 1U;
    }
    record.recordId = appNextRecordId;

    queuedRecord = osMailAlloc(appStorageQueue, 0U);
    if (queuedRecord == NULL)
    {
        appStorageQueueFull = 1U;
        appStorageStatusVersion++;
        return;
    }
    *queuedRecord = record;
    if (osMailPut(appStorageQueue, queuedRecord) != osOK)
    {
        (void)osMailFree(appStorageQueue, queuedRecord);
        appStorageQueueFull = 1U;
        appStorageStatusVersion++;
        return;
    }

    appPendingCount++;
    appStorageQueueFull = 0U;
    appStorageStatusVersion++;
    APP_InsertTodayRecord(&record);
    appHistoryPage = 0U;

    APP_DrawLeftPanel();
    APP_DrawDeleteButton();
}

static void APP_InsertTodayRecord(const APP_MilkRecordTypeDef *record)
{
    uint16_t recordIndex;

    if ((record->year == appTodayYear) &&
        (record->month == appTodayMonth) &&
        (record->day == appTodayDay))
    {
        appTodayTotalMl += record->amountMl;
    }

    if (appRecordCount < APP_HISTORY_MAX_RECORDS)
    {
        recordIndex = appRecordCount++;
    }
    else
    {
        memmove(&appMilkRecords[0], &appMilkRecords[1],
                sizeof(appMilkRecords[0]) *
                    (APP_HISTORY_MAX_RECORDS - 1U));
        recordIndex = APP_HISTORY_MAX_RECORDS - 1U;
    }

    appMilkRecords[recordIndex] = *record;
}

static FRESULT APP_AppendMilkRecord(const APP_MilkRecordTypeDef *record)
{
    FIL file;
    FRESULT result;
    UINT bytesWritten = 0U;
    char line[64];
    int length;
    uint8_t fileOpen = 0U;

    length = snprintf(line, sizeof(line),
                      "%04u-%02u-%02u,%02u:%02u:%02u,%u,%lu\r\n",
                      record->year, record->month, record->day,
                      record->hour, record->minute, record->second,
                      record->amountMl,
                      (unsigned long)record->recordId);
    if ((length <= 0) || ((size_t)length >= sizeof(line)))
    {
        return FR_INVALID_PARAMETER;
    }

    appStorageWriteStage = 1U;
    result = f_open(&file, appSdFilePath, FA_OPEN_ALWAYS | FA_WRITE);
    if (result == FR_OK)
    {
        fileOpen = 1U;
        osDelay(APP_SD_STEP_DELAY_MS);
    }
    if (result == FR_OK)
    {
        appStorageWriteStage = 2U;
        result = f_lseek(&file, f_size(&file));
        if (result == FR_OK)
        {
            osDelay(APP_SD_STEP_DELAY_MS);
        }
    }
    if (result == FR_OK)
    {
        appStorageWriteStage = 3U;
        result = f_write(&file, line, (UINT)length, &bytesWritten);
        if ((result == FR_OK) && (bytesWritten != (UINT)length))
        {
            result = FR_DISK_ERR;
        }
        if (result == FR_OK)
        {
            osDelay(APP_SD_STEP_DELAY_MS);
        }
    }
    if (fileOpen != 0U)
    {
        appStorageWriteStage = 4U;
        FRESULT closeResult = f_close(&file);
        if (result == FR_OK)
        {
            result = closeResult;
        }
        if (result == FR_OK)
        {
            osDelay(APP_SD_STEP_DELAY_MS);
        }
    }
    if (result == FR_OK)
    {
        appStorageWriteStage = 0U;
    }
    return result;
}

static FRESULT APP_RecoverCsvUpgrade(void)
{
    FILINFO info;
    FRESULT mainStatus;
    FRESULT tempStatus;
    FRESULT backupStatus;
    char tempPath[24];
    char backupPath[24];

    snprintf(tempPath, sizeof(tempPath), "%sMILKUPG.CSV", SDPath);
    snprintf(backupPath, sizeof(backupPath), "%sMILKOLD.CSV", SDPath);
    mainStatus = f_stat(appSdFilePath, &info);
    tempStatus = f_stat(tempPath, &info);
    backupStatus = f_stat(backupPath, &info);

    if (mainStatus == FR_OK)
    {
        if (tempStatus == FR_OK)
        {
            (void)f_unlink(tempPath);
        }
        if (backupStatus == FR_OK)
        {
            (void)f_unlink(backupPath);
        }
        return FR_OK;
    }
    if (mainStatus != FR_NO_FILE)
    {
        return mainStatus;
    }

    if (tempStatus == FR_OK)
    {
        FRESULT result = f_rename(tempPath, appSdFilePath);
        if (result == FR_OK)
        {
            (void)f_unlink(backupPath);
        }
        return result;
    }
    if (backupStatus == FR_OK)
    {
        return f_rename(backupPath, appSdFilePath);
    }
    return FR_OK;
}

static FRESULT APP_RecoverSd(void)
{
    FRESULT result;

    /*
     * A newly inserted card starts from its own power-on protocol state while
     * SDMMC may still contain the state of the removed card.  A FatFs remount
     * alone cannot repair that mismatch, so reset the host peripheral before
     * asking disk_initialize() to negotiate with the card again.
     */
    (void)f_mount(NULL, (TCHAR const *)SDPath, 1U);
    if (BSP_SD_IsDetected() != SD_PRESENT)
    {
        appBspSdCardState = 0xFFU;
        appSdHalState = (uint32_t)HAL_SD_GetState(&hsd1);
        appSdHalError = HAL_SD_GetError(&hsd1);
        return FR_NOT_READY;
    }

    /* Debounce the socket and allow all card contacts/power to settle. */
    osDelay(APP_SD_REINSERT_STABLE_MS);
    if (BSP_SD_IsDetected() != SD_PRESENT)
    {
        return FR_NOT_READY;
    }

    if (HAL_SD_DeInit(&hsd1) != HAL_OK)
    {
        APP_SD_CaptureHalState();
        return FR_DISK_ERR;
    }
    osDelay(50U);

    appBspSdInitResult = BSP_SD_Init();
    if ((appBspSdInitResult != MSD_OK) ||
        (HAL_SD_GetState(&hsd1) != HAL_SD_STATE_READY))
    {
        APP_SD_CaptureHalState();
        return FR_NOT_READY;
    }

    appSdDiskStatus = disk_status(appSdPhysicalDrive);
    if ((appSdDiskStatus & STA_NOINIT) != 0U)
    {
        APP_SD_CaptureHalState();
        return FR_NOT_READY;
    }

    result = f_mount(&SDFatFS, (TCHAR const *)SDPath, 1U);
    APP_SD_CaptureHalState();
    return result;
}

static void APP_ProcessStorageEvents(void)
{
    osEvent event;
    uint8_t redraw = 0U;

    do
    {
        uint32_t recordId;
        uint16_t i;

        event = osMessageGet(appStorageDoneQueue, 0U);
        if (event.status != osEventMessage)
        {
            break;
        }

        recordId = event.value.v;
        for (i = 0U; i < appRecordCount; i++)
        {
            if ((appMilkRecords[i].recordId == recordId) &&
                (appMilkRecords[i].pending != 0U))
            {
                appMilkRecords[i].pending = 0U;
                appMilkRecords[i].sourceOrdinal = ++appCsvRecordCount;
                redraw = 1U;
                break;
            }
        }
        if (appPendingCount > 0U)
        {
            appPendingCount--;
        }
        appStorageQueueFull = 0U;
        appStorageStatusVersion++;
    } while (1);

    if (redraw != 0U)
    {
        APP_DrawLeftPanel();
        APP_DrawDeleteButton();
    }
}

static void APP_StorageTask(void const *argument)
{
    (void)argument;

    for (;;)
    {
        osEvent event = osMailGet(appStorageQueue, osWaitForever);

        if (event.status == osEventMail)
        {
            APP_MilkRecordTypeDef record =
                *(APP_MilkRecordTypeDef *)event.value.p;
            FRESULT result;
            uint8_t directFailureCount = 0U;

            (void)osMailFree(appStorageQueue, event.value.p);

            for (;;)
            {
                result = APP_AppendMilkRecord(&record);
                if (result == FR_OK)
                {
                    appStorageLastResult = FR_OK;
                    appStorageFault = 0U;
                    appSdReady = 1U;
                    appSdRemovalSeen = 0U;
                    appStorageStatusVersion++;
                    (void)osMessagePut(appStorageDoneQueue,
                                       record.recordId,
                                       osWaitForever);
                    /* Give the card a short quiet period before the FIFO's
                       next open/read/write transaction. */
                    osDelay(APP_SD_INTERWRITE_MS);
                    break;
                }

                APP_SD_CaptureHalState();
                appStorageLastResult = result;
                appStorageFault = 1U;
                appSdReady = 0U;
                appStorageStatusVersion++;

                if (BSP_SD_IsDetected() != SD_PRESENT)
                {
                    appSdRemovalSeen = 1U;
                    directFailureCount = 0U;
                }

                osDelay(APP_SD_RETRY_MS);
                if (BSP_SD_IsDetected() != SD_PRESENT)
                {
                    appStorageLastResult = FR_NOT_READY;
                    appStorageStatusVersion++;
                    continue;
                }

                directFailureCount++;
                if ((appSdRemovalSeen != 0U) ||
                    (directFailureCount >= APP_SD_DIRECT_FAILURE_LIMIT))
                {

                    result = APP_RecoverSd();
                    if (result != FR_OK)
                    {
                        appStorageLastResult = result;
                        appStorageStatusVersion++;
                        continue;
                    }

                    appSdRemovalSeen = 0U;
                    directFailureCount = 0U;
                    /* Let the reinitialized card settle before f_open(). */
                    osDelay(APP_SD_POST_RECOVERY_MS);
                }
            }
        }
    }
}

static FRESULT APP_LoadTodayRecords(void)
{
    FIL file;
    FRESULT result;
    char line[96];

    result = f_open(&file, appSdFilePath, FA_READ);
    if (result != FR_OK)
    {
        return result;
    }

    appCsvRecordCount = 0U;

    while (f_gets(line, sizeof(line), &file) != NULL)
    {
        APP_MilkRecordTypeDef record;
        unsigned int year, month, day, hour, minute, second, amount;
        unsigned long recordId = 0UL;
        int fields;

        fields = sscanf(line, "%u-%u-%u,%u:%u:%u,%u,%lu",
                        &year, &month, &day, &hour, &minute, &second,
                        &amount, &recordId);
        if (fields < 7)
        {
            continue;
        }

        if ((month < 1U) || (month > 12U) || (day < 1U) ||
            (day > 31U) || (hour > 23U) || (minute > 59U) ||
            (second > 59U) || (amount > 65535U))
        {
            continue;
        }

        record.year = (uint16_t)year;
        record.month = (uint8_t)month;
        record.day = (uint8_t)day;
        record.hour = (uint8_t)hour;
        record.minute = (uint8_t)minute;
        record.second = (uint8_t)second;
        record.amountMl = (uint16_t)amount;
        record.recordId = (fields >= 8) ? (uint32_t)recordId : 0U;
        record.pending = 0U;
        record.sourceOrdinal = ++appCsvRecordCount;
        if (record.recordId > appNextRecordId)
        {
            appNextRecordId = record.recordId;
        }
        APP_InsertTodayRecord(&record);
    }

    result = f_error(&file);
    {
        FRESULT closeResult = f_close(&file);
        if (result == FR_OK)
        {
            result = closeResult;
        }
    }
    return result;
}

static FRESULT APP_DeleteRecord(uint32_t sourceOrdinal)
{
    FIL source;
    FIL temp;
    FRESULT result;
    char line[64];
    char tempPath[24];
    char backupPath[24];
    uint32_t ordinal = 0U;

    snprintf(tempPath, sizeof(tempPath), "%sMILKTMP.CSV", SDPath);
    snprintf(backupPath, sizeof(backupPath), "%sMILKBAK.CSV", SDPath);
    (void)f_unlink(tempPath);
    (void)f_unlink(backupPath);

    result = f_open(&source, appSdFilePath, FA_READ);
    if (result != FR_OK)
    {
        return result;
    }
    result = f_open(&temp, tempPath, FA_CREATE_ALWAYS | FA_WRITE);
    if (result != FR_OK)
    {
        (void)f_close(&source);
        return result;
    }

    while ((result == FR_OK) && (f_gets(line, sizeof(line), &source) != NULL))
    {
        unsigned int y, mo, d, h, mi, s, amount;
        UINT written = 0U;
        UINT length = (UINT)strlen(line);

        if (sscanf(line, "%u-%u-%u,%u:%u:%u,%u",
                   &y, &mo, &d, &h, &mi, &s, &amount) == 7)
        {
            ordinal++;
            if (ordinal == sourceOrdinal)
            {
                continue;
            }
        }

        result = f_write(&temp, line, length, &written);
        if ((result == FR_OK) && (written != length))
        {
            result = FR_DISK_ERR;
        }
    }

    if (result == FR_OK)
    {
        result = f_error(&source);
    }
    if (result == FR_OK)
    {
        result = f_sync(&temp);
    }
    (void)f_close(&source);
    {
        FRESULT closeResult = f_close(&temp);
        if (result == FR_OK)
        {
            result = closeResult;
        }
    }

    if (result != FR_OK)
    {
        (void)f_unlink(tempPath);
        return result;
    }

    result = f_rename(appSdFilePath, backupPath);
    if (result == FR_OK)
    {
        result = f_rename(tempPath, appSdFilePath);
        if (result == FR_OK)
        {
            (void)f_unlink(backupPath);
        }
        else
        {
            (void)f_rename(backupPath, appSdFilePath);
        }
    }
    return result;
}


static void APP_ProcessTouch(void)
{
    static uint8_t previousTouching = 0U;
    static int8_t activeButtonIndex = -1;
    static uint8_t historyTouchActive = 0U;
    static uint16_t historyTouchStartY = 0U;
    static uint16_t historyTouchLastY = 0U;
    static uint8_t variableTouchActive = 0U;
    static uint16_t variableTouchStartX = 0U;
    static uint16_t variableTouchStartY = 0U;
    static uint16_t variableTouchLastX = 0U;
    static uint16_t variableTouchLastY = 0U;

    uint8_t touching;
    uint8_t i;

    uint16_t touchX;
    uint16_t touchY;

    uint32_t currentTick;

    if (BSP_TS_GetState(&appTouchState) != TS_OK)
    {
        return;
    }

    touching =
        (appTouchState.touchDetected > 0U) ?
        1U : 0U;

    /*
     * 刚刚按下。
     */
    if ((touching != 0U) &&
        (previousTouching == 0U))
    {
        touchX = appTouchState.touchX[0];
        touchY = appTouchState.touchY[0];

        if ((touchX < APP_SEPARATOR_X) &&
            (APP_PointInRect(touchX, touchY,
                             APP_MAIN_MENU_X, APP_MAIN_MENU_Y,
                             APP_MAIN_MENU_WIDTH,
                             APP_MAIN_MENU_HEIGHT) == 0U))
        {
            historyTouchActive = 1U;
            historyTouchStartY = touchY;
            historyTouchLastY = touchY;
        }

        currentTick = HAL_GetTick();

        if ((currentTick - appLastPressTick) >=
            APP_TOUCH_DEBOUNCE_MS)
        {
            if ((appPendingCount == 0U) &&
                (appStorageFault == 0U) &&
                (APP_PointInRect(touchX, touchY,
                                 APP_MAIN_MENU_X, APP_MAIN_MENU_Y,
                                 APP_MAIN_MENU_WIDTH,
                                 APP_MAIN_MENU_HEIGHT) != 0U))
            {
                TS_StateTypeDef releaseState = {0};

                appLastPressTick = currentTick;
                historyTouchActive = 0U;
                activeButtonIndex = -1;
                APP_BootMenu();

                /* START returns on touch-down; consume that touch so it
                   cannot select a history row on the redrawn main screen. */
                do
                {
                    (void)BSP_TS_GetState(&releaseState);
                    osDelay(20U);
                } while (releaseState.touchDetected > 0U);

                APP_InitTodayState();
                (void)APP_LoadTodayRecords();
                APP_DrawInterface();
                appLastPressTick = HAL_GetTick();
                previousTouching = 0U;
                return;
            }

            if ((appSelectedRecordIndex >= 0) &&
                (appMilkRecords[appSelectedRecordIndex].pending == 0U) &&
                (appPendingCount == 0U) &&
                (appStorageFault == 0U) &&
                (APP_PointInRect(touchX, touchY, APP_DELETE_X,
                                 APP_DELETE_Y, APP_DELETE_WIDTH,
                                 APP_DELETE_HEIGHT) != 0U))
            {
                appLastPressTick = currentTick;
                if (APP_DeleteRecord(
                        appMilkRecords[appSelectedRecordIndex].sourceOrdinal)
                    == FR_OK)
                {
                    APP_InitTodayState();
                    (void)APP_LoadTodayRecords();
                    APP_DrawInterface();
                }
            }

            if (APP_PointInRect(touchX, touchY,
                                APP_VARIABLE_X, APP_VARIABLE_Y,
                                APP_VARIABLE_WIDTH,
                                APP_VARIABLE_HEIGHT) != 0U)
            {
                appLastPressTick = currentTick;
                variableTouchActive = 1U;
                variableTouchStartX = touchX;
                variableTouchStartY = touchY;
                variableTouchLastX = touchX;
                variableTouchLastY = touchY;
                APP_DrawVariableMilkButton(1U);
            }

            for (i = 0U;
                 i < APP_MILK_BUTTON_COUNT;
                 i++)
            {
                if (APP_PointInRect(
                        touchX,
                        touchY,
                        appMilkButtons[i].x,
                        appMilkButtons[i].y,
                        appMilkButtons[i].width,
                        appMilkButtons[i].height) != 0U)
                {
                    appLastPressTick = currentTick;
                    activeButtonIndex = (int8_t)i;

                    APP_DrawMilkButton(i, 1U);

                    APP_AddMilkRecord(
                        appMilkButtons[i].amountMl
                    );

                    break;
                }
            }
        }
    }

    if ((touching != 0U) && (historyTouchActive != 0U))
    {
        historyTouchLastY = appTouchState.touchY[0];
    }
    if ((touching != 0U) && (variableTouchActive != 0U))
    {
        variableTouchLastX = appTouchState.touchX[0];
        variableTouchLastY = appTouchState.touchY[0];
    }

    /*
     * 手指松开。
     */
    if ((touching == 0U) &&
        (previousTouching != 0U))
    {
        if (variableTouchActive != 0U)
        {
            int32_t deltaX = (int32_t)variableTouchLastX -
                             (int32_t)variableTouchStartX;
            int32_t deltaY = (int32_t)variableTouchLastY -
                             (int32_t)variableTouchStartY;
            int32_t absoluteX = (deltaX < 0) ? -deltaX : deltaX;
            int32_t absoluteY = (deltaY < 0) ? -deltaY : deltaY;

            if ((deltaX >= (int32_t)APP_VARIABLE_SWIPE_PIXELS) &&
                (absoluteX > absoluteY))
            {
                if (appVariableAmountMl <=
                    (APP_VARIABLE_MAX_ML - APP_VARIABLE_STEP_ML))
                {
                    appVariableAmountMl += APP_VARIABLE_STEP_ML;
                }
            }
            else if ((deltaX <= -(int32_t)APP_VARIABLE_SWIPE_PIXELS) &&
                     (absoluteX > absoluteY))
            {
                if (appVariableAmountMl >=
                    (APP_VARIABLE_MIN_ML + APP_VARIABLE_STEP_ML))
                {
                    appVariableAmountMl -= APP_VARIABLE_STEP_ML;
                }
            }
            else if ((absoluteX <=
                      (int32_t)APP_VARIABLE_TAP_MOVE_PIXELS) &&
                     (absoluteY <=
                      (int32_t)APP_VARIABLE_TAP_MOVE_PIXELS))
            {
                APP_AddMilkRecord(appVariableAmountMl);
            }

            APP_DrawVariableMilkButton(0U);
            variableTouchActive = 0U;
        }

        if (historyTouchActive != 0U)
        {
            int32_t swipe = (int32_t)historyTouchStartY -
                            (int32_t)historyTouchLastY;
            uint16_t firstIndex;
            uint16_t endIndex;
            uint16_t totalPages;

            APP_GetHistoryPageRange(&firstIndex, &endIndex, &totalPages);

            if ((swipe >= (int32_t)APP_HISTORY_SWIPE_PIXELS) &&
                ((appHistoryPage + 1U) < totalPages))
            {
                appHistoryPage++;
                appSelectedRecordIndex = -1;
                APP_DrawLeftPanel();
                APP_DrawDeleteButton();
            }
            else if ((swipe <= -(int32_t)APP_HISTORY_SWIPE_PIXELS) &&
                     (appHistoryPage > 0U))
            {
                appHistoryPage--;
                appSelectedRecordIndex = -1;
                APP_DrawLeftPanel();
                APP_DrawDeleteButton();
            }
            else if ((swipe < (int32_t)APP_HISTORY_SWIPE_PIXELS) &&
                     (swipe > -(int32_t)APP_HISTORY_SWIPE_PIXELS) &&
                     (historyTouchLastY >= APP_RECORD_Y))
            {
                uint16_t row = (uint16_t)
                    ((historyTouchLastY - APP_RECORD_Y) /
                     APP_RECORD_LINE_HEIGHT);
                uint16_t firstIndex;
                uint16_t endIndex;
                uint16_t totalPages;
                uint16_t pageCount;

                APP_GetHistoryPageRange(&firstIndex, &endIndex, &totalPages);
                pageCount = endIndex - firstIndex;

                if (row < pageCount)
                {
                    appSelectedRecordIndex =
                        (int16_t)(endIndex - 1U - row);
                    APP_DrawLeftPanel();
                    APP_DrawDeleteButton();
                }
            }

            historyTouchActive = 0U;
        }

        if (activeButtonIndex >= 0)
        {
            APP_DrawMilkButton(
                (uint8_t)activeButtonIndex,
                0U
            );

            activeButtonIndex = -1;
        }
    }

    previousTouching = touching;
}

static FRESULT APP_SD_Test(void)
{
    FRESULT result = FR_OK;
    FRESULT closeResult = FR_OK;
    UINT bytesWritten = 0U;

    static const char csvHeader[] =
        "date,time,amount_ml,record_id\r\n";

    appBspSdInitResult = 0xFFU;
    appBspSdCardState = 0xFFU;
    appSdDiskStatus = STA_NOINIT;
    appSdHalError = 0U;
    appSdHalState = 0U;
    appSdPhysicalDrive = 0xFFU;
    appSdFailStage = 0U;

    /*
     * SDPath 应为 "0:/"。
     */
    if ((SDPath[0] < '0') ||
        (SDPath[0] > '9'))
    {
        return FR_INVALID_DRIVE;
    }

    appSdPhysicalDrive =
        (BYTE)(SDPath[0] - '0');

    /*
     * 此处由 sd_diskio.c 内部调用 BSP_SD_Init()。
     */
    appSdDiskStatus =
        disk_initialize(appSdPhysicalDrive);

    osDelay(50U);

    appBspSdCardState =
        BSP_SD_GetCardState();

    appSdHalError =
        HAL_SD_GetError(&hsd1);

    appSdHalState =
        (uint32_t)HAL_SD_GetState(&hsd1);

    /*
     * BSP 显示值根据磁盘初始化结果记录。
     */
    if ((appSdDiskStatus & STA_NOINIT) != 0U)
    {
        appBspSdInitResult = MSD_ERROR;
        appSdFailStage = 1U;
        APP_SD_CaptureHalState();
        return FR_NOT_READY;
    }

    appBspSdInitResult = MSD_OK;

    /*
     * 挂载文件系统。
     */
    appSdFailStage = 2U;
    result = f_mount(
        &SDFatFS,
        (TCHAR const *)SDPath,
        1U
    );

    if (result != FR_OK)
    {
        APP_SD_CaptureHalState();
        return result;
    }

    snprintf(
        appSdFilePath,
        sizeof(appSdFilePath),
        "%sMILKLOG.CSV",
        SDPath
    );

    result = APP_RecoverCsvUpgrade();
    if (result != FR_OK)
    {
        APP_SD_CaptureHalState();
        return result;
    }

    appSdFailStage = 3U;
    result = f_open(
        &appSdFile,
        appSdFilePath,
        FA_OPEN_ALWAYS | FA_WRITE
    );

    if (result != FR_OK)
    {
        APP_SD_CaptureHalState();
        (void)f_mount(
            NULL,
            (TCHAR const *)SDPath,
            1U
        );

        return result;
    }

    if (f_size(&appSdFile) == 0U)
    {
        appSdFailStage = 4U;
        result = f_write(
            &appSdFile,
            csvHeader,
            sizeof(csvHeader) - 1U,
            &bytesWritten
        );

        if ((result == FR_OK) &&
            (bytesWritten !=
             (sizeof(csvHeader) - 1U)))
        {
            result = FR_DISK_ERR;
        }

        if (result == FR_OK)
        {
            appSdFailStage = 5U;
            result = f_sync(&appSdFile);
        }

        if (result != FR_OK)
        {
            APP_SD_CaptureHalState();
        }
    }

    if (result == FR_OK)
    {
        appSdFailStage = 6U;
    }
    closeResult =
        f_close(&appSdFile);

    if (result == FR_OK)
    {
        result = closeResult;
    }

    APP_SD_CaptureHalState();
    if (result == FR_OK)
    {
        appSdFailStage = 0U;
    }
    return result;
}

static void APP_SD_CaptureHalState(void)
{
    appSdHalState = (uint32_t)HAL_SD_GetState(&hsd1);
    appSdHalError = HAL_SD_GetError(&hsd1);

    /*
     * Do not send CMD13 merely to collect diagnostics after an I/O failure.
     * The extra command can overwrite the original HAL error and disturb the
     * retry path.  Explicit disk status checks update appBspSdCardState where
     * they are actually required.
     */
    if ((BSP_SD_IsDetected() != SD_PRESENT) ||
        (appSdHalState != (uint32_t)HAL_SD_STATE_READY))
    {
        appBspSdCardState = 0xFFU;
    }
}

static void APP_ShowSdTestResult(FRESULT result)
{
    char text[48];

    BSP_LCD_Clear(LCD_COLOR_WHITE);

    /*
     * 标题。
     */
    BSP_LCD_SetFont(&Font20);
    BSP_LCD_SetBackColor(LCD_COLOR_WHITE);

    if (result == FR_OK)
    {
        BSP_LCD_SetTextColor(LCD_COLOR_GREEN);

        BSP_LCD_DisplayStringAt(
            0U,
            45U,
            (uint8_t *)"SD CARD OK",
            CENTER_MODE
        );
    }
    else
    {
        BSP_LCD_SetTextColor(LCD_COLOR_RED);

        BSP_LCD_DisplayStringAt(
            0U,
            45U,
            (uint8_t *)"SD CARD ERROR",
            CENTER_MODE
        );
    }

    /*
     * 诊断信息。
     */
    BSP_LCD_SetFont(&Font16);
    BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);

    /*
     * FatFs 和 BSP 初始化结果。
     */
    snprintf(
        text,
        sizeof(text),
        "FatFs: %u  BSP: %u",
        (unsigned int)result,
        (unsigned int)appBspSdInitResult
    );

    BSP_LCD_DisplayStringAt(
        0U,
        90U,
        (uint8_t *)text,
        CENTER_MODE
    );

    /*
     * 磁盘状态和物理盘号。
     */
    snprintf(
        text,
        sizeof(text),
        "Disk: %02X Drive:%u Stage:%u",
        (unsigned int)appSdDiskStatus,
        (unsigned int)appSdPhysicalDrive,
        (unsigned int)appSdFailStage
    );

    BSP_LCD_DisplayStringAt(
        0U,
        115U,
        (uint8_t *)text,
        CENTER_MODE
    );

    /*
     * SD 卡传输状态。
     */
    snprintf(
        text,
        sizeof(text),
        "Card state: %u",
        (unsigned int)appBspSdCardState
    );

    BSP_LCD_DisplayStringAt(
        0U,
        140U,
        (uint8_t *)text,
        CENTER_MODE
    );

    /*
     * HAL SD 状态。
     */
    snprintf(
        text,
        sizeof(text),
        "HAL state: %lu",
        (unsigned long)appSdHalState
    );

    BSP_LCD_DisplayStringAt(
        0U,
        165U,
        (uint8_t *)text,
        CENTER_MODE
    );

    /*
     * HAL SD 错误码。
     */
    snprintf(
        text,
        sizeof(text),
        "HAL error: 0x%08lX",
        (unsigned long)appSdHalError
    );

    BSP_LCD_DisplayStringAt(
        0U,
        190U,
        (uint8_t *)text,
        CENTER_MODE
    );

    /*
     * 显示 SDPath。
     */
    snprintf(
        text,
        sizeof(text),
        "Path: %s",
        SDPath
    );

    BSP_LCD_DisplayStringAt(
        0U,
        215U,
        (uint8_t *)text,
        CENTER_MODE
    );

    /* Success only needs a brief confirmation; keep errors visible for debug. */
    osDelay((result == FR_OK) ? 1500U : 10000U);
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

  /* MPU Configuration--------------------------------------------------------*/
  // MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC3_Init();
  MX_CRC_Init();
  MX_DCMI_Init();
  MX_DMA2D_Init();
  MX_FMC_Init();
  MX_I2C1_Init();
  MX_I2C3_Init();
  MX_LTDC_Init();
  MX_QUADSPI_Init();
  MX_RTC_Init();
  MX_SAI2_Init();
  MX_SDMMC1_SD_Init();
  MX_SPDIFRX_Init();
  MX_SPI2_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM5_Init();
  MX_TIM8_Init();
  MX_TIM12_Init();
  MX_USART1_UART_Init();
  MX_USART6_UART_Init();
  MX_FATFS_Init();
  /* USER CODE BEGIN 2 */


  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  appStorageQueue = osMailCreate(osMailQ(AppStorageMail), NULL);
  appStorageDoneQueue =
      osMessageCreate(osMessageQ(AppStorageDone), NULL);
  if ((appStorageQueue == NULL) || (appStorageDoneQueue == NULL))
  {
    Error_Handler();
  }
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 4096);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  osThreadDef(storageTask, APP_StorageTask, osPriorityAboveNormal, 0, 3072);
  appStorageTaskHandle =
      osThreadCreate(osThread(storageTask), NULL);
  if (appStorageTaskHandle == NULL)
  {
    Error_Handler();
  }
  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 400;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_6) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC|RCC_PERIPHCLK_SAI2
                              |RCC_PERIPHCLK_SDMMC1|RCC_PERIPHCLK_CLK48;
  PeriphClkInitStruct.PLLSAI.PLLSAIN = 384;
  PeriphClkInitStruct.PLLSAI.PLLSAIR = 5;
  PeriphClkInitStruct.PLLSAI.PLLSAIQ = 2;
  PeriphClkInitStruct.PLLSAI.PLLSAIP = RCC_PLLSAIP_DIV8;
  PeriphClkInitStruct.PLLSAIDivQ = 1;
  PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_8;
  PeriphClkInitStruct.Sai2ClockSelection = RCC_SAI2CLKSOURCE_PLLSAI;
  PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48SOURCE_PLLSAIP;
  PeriphClkInitStruct.Sdmmc1ClockSelection = RCC_SDMMC1CLKSOURCE_CLK48;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC3_Init(void)
{

  /* USER CODE BEGIN ADC3_Init 0 */

  /* USER CODE END ADC3_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC3_Init 1 */

  /* USER CODE END ADC3_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc3.Instance = ADC3;
  hadc3.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc3.Init.Resolution = ADC_RESOLUTION_12B;
  hadc3.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc3.Init.ContinuousConvMode = DISABLE;
  hadc3.Init.DiscontinuousConvMode = DISABLE;
  hadc3.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc3.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc3.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc3.Init.NbrOfConversion = 1;
  hadc3.Init.DMAContinuousRequests = DISABLE;
  hadc3.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc3) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC3_Init 2 */

  /* USER CODE END ADC3_Init 2 */

}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
  hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
  hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
  hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
  hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief DCMI Initialization Function
  * @param None
  * @retval None
  */
static void MX_DCMI_Init(void)
{

  /* USER CODE BEGIN DCMI_Init 0 */

  /* USER CODE END DCMI_Init 0 */

  /* USER CODE BEGIN DCMI_Init 1 */

  /* USER CODE END DCMI_Init 1 */
  hdcmi.Instance = DCMI;
  hdcmi.Init.SynchroMode = DCMI_SYNCHRO_HARDWARE;
  hdcmi.Init.PCKPolarity = DCMI_PCKPOLARITY_FALLING;
  hdcmi.Init.VSPolarity = DCMI_VSPOLARITY_LOW;
  hdcmi.Init.HSPolarity = DCMI_HSPOLARITY_LOW;
  hdcmi.Init.CaptureRate = DCMI_CR_ALL_FRAME;
  hdcmi.Init.ExtendedDataMode = DCMI_EXTEND_DATA_8B;
  hdcmi.Init.JPEGMode = DCMI_JPEG_DISABLE;
  hdcmi.Init.ByteSelectMode = DCMI_BSM_ALL;
  hdcmi.Init.ByteSelectStart = DCMI_OEBS_ODD;
  hdcmi.Init.LineSelectMode = DCMI_LSM_ALL;
  hdcmi.Init.LineSelectStart = DCMI_OELS_ODD;
  if (HAL_DCMI_Init(&hdcmi) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DCMI_Init 2 */

  /* USER CODE END DCMI_Init 2 */

}

/**
  * @brief DMA2D Initialization Function
  * @param None
  * @retval None
  */
static void MX_DMA2D_Init(void)
{

  /* USER CODE BEGIN DMA2D_Init 0 */

  /* USER CODE END DMA2D_Init 0 */

  /* USER CODE BEGIN DMA2D_Init 1 */

  /* USER CODE END DMA2D_Init 1 */
  hdma2d.Instance = DMA2D;
  hdma2d.Init.Mode = DMA2D_M2M;
  hdma2d.Init.ColorMode = DMA2D_OUTPUT_ARGB8888;
  hdma2d.Init.OutputOffset = 0;
  hdma2d.LayerCfg[1].InputOffset = 0;
  hdma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_ARGB8888;
  hdma2d.LayerCfg[1].AlphaMode = DMA2D_NO_MODIF_ALPHA;
  hdma2d.LayerCfg[1].InputAlpha = 0;
  if (HAL_DMA2D_Init(&hdma2d) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DMA2D_ConfigLayer(&hdma2d, 1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DMA2D_Init 2 */

  /* USER CODE END DMA2D_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00C0EAFF;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2C3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C3_Init(void)
{

  /* USER CODE BEGIN I2C3_Init 0 */

  /* USER CODE END I2C3_Init 0 */

  /* USER CODE BEGIN I2C3_Init 1 */

  /* USER CODE END I2C3_Init 1 */
  hi2c3.Instance = I2C3;
  hi2c3.Init.Timing = 0x00C0EAFF;
  hi2c3.Init.OwnAddress1 = 0;
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c3.Init.OwnAddress2 = 0;
  hi2c3.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c3) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c3, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c3, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C3_Init 2 */

  /* USER CODE END I2C3_Init 2 */

}

/**
  * @brief LTDC Initialization Function
  * @param None
  * @retval None
  */
static void MX_LTDC_Init(void)
{

  /* USER CODE BEGIN LTDC_Init 0 */

  /* USER CODE END LTDC_Init 0 */

  LTDC_LayerCfgTypeDef pLayerCfg = {0};

  /* USER CODE BEGIN LTDC_Init 1 */

  /* USER CODE END LTDC_Init 1 */
  hltdc.Instance = LTDC;
  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
  hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;
  hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
  hltdc.Init.HorizontalSync = 40;
  hltdc.Init.VerticalSync = 9;
  hltdc.Init.AccumulatedHBP = 53;
  hltdc.Init.AccumulatedVBP = 11;
  hltdc.Init.AccumulatedActiveW = 533;
  hltdc.Init.AccumulatedActiveH = 283;
  hltdc.Init.TotalWidth = 565;
  hltdc.Init.TotalHeigh = 285;
  hltdc.Init.Backcolor.Blue = 0;
  hltdc.Init.Backcolor.Green = 0;
  hltdc.Init.Backcolor.Red = 0;
  if (HAL_LTDC_Init(&hltdc) != HAL_OK)
  {
    Error_Handler();
  }
  pLayerCfg.WindowX0 = 0;
  pLayerCfg.WindowX1 = 480;
  pLayerCfg.WindowY0 = 0;
  pLayerCfg.WindowY1 = 272;
  pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
  pLayerCfg.Alpha = 255;
  pLayerCfg.Alpha0 = 0;
  pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
  pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
  pLayerCfg.FBStartAdress = 0xC0000000;
  pLayerCfg.ImageWidth = 480;
  pLayerCfg.ImageHeight = 272;
  pLayerCfg.Backcolor.Blue = 0;
  pLayerCfg.Backcolor.Green = 0;
  pLayerCfg.Backcolor.Red = 0;
  if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LTDC_Init 2 */

  /* USER CODE END LTDC_Init 2 */

}

/**
  * @brief QUADSPI Initialization Function
  * @param None
  * @retval None
  */
static void MX_QUADSPI_Init(void)
{

  /* USER CODE BEGIN QUADSPI_Init 0 */

  /* USER CODE END QUADSPI_Init 0 */

  /* USER CODE BEGIN QUADSPI_Init 1 */

  /* USER CODE END QUADSPI_Init 1 */
  /* QUADSPI parameter configuration*/
  hqspi.Instance = QUADSPI;
  hqspi.Init.ClockPrescaler = 1;
  hqspi.Init.FifoThreshold = 4;
  hqspi.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
  hqspi.Init.FlashSize = 24;
  hqspi.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_6_CYCLE;
  hqspi.Init.ClockMode = QSPI_CLOCK_MODE_0;
  hqspi.Init.FlashID = QSPI_FLASH_ID_1;
  hqspi.Init.DualFlash = QSPI_DUALFLASH_DISABLE;
  if (HAL_QSPI_Init(&hqspi) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN QUADSPI_Init 2 */

  /* USER CODE END QUADSPI_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{
  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* 时间由开机触摸界面设置 */
}

/**
  * @brief SAI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SAI2_Init(void)
{

  /* USER CODE BEGIN SAI2_Init 0 */

  /* USER CODE END SAI2_Init 0 */

  /* USER CODE BEGIN SAI2_Init 1 */

  /* USER CODE END SAI2_Init 1 */
  hsai_BlockA2.Instance = SAI2_Block_A;
  hsai_BlockA2.Init.Protocol = SAI_FREE_PROTOCOL;
  hsai_BlockA2.Init.AudioMode = SAI_MODEMASTER_TX;
  hsai_BlockA2.Init.DataSize = SAI_DATASIZE_8;
  hsai_BlockA2.Init.FirstBit = SAI_FIRSTBIT_MSB;
  hsai_BlockA2.Init.ClockStrobing = SAI_CLOCKSTROBING_FALLINGEDGE;
  hsai_BlockA2.Init.Synchro = SAI_ASYNCHRONOUS;
  hsai_BlockA2.Init.OutputDrive = SAI_OUTPUTDRIVE_DISABLE;
  hsai_BlockA2.Init.NoDivider = SAI_MASTERDIVIDER_ENABLE;
  hsai_BlockA2.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_EMPTY;
  hsai_BlockA2.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_192K;
  hsai_BlockA2.Init.SynchroExt = SAI_SYNCEXT_DISABLE;
  hsai_BlockA2.Init.MonoStereoMode = SAI_STEREOMODE;
  hsai_BlockA2.Init.CompandingMode = SAI_NOCOMPANDING;
  hsai_BlockA2.Init.TriState = SAI_OUTPUT_NOTRELEASED;
  hsai_BlockA2.FrameInit.FrameLength = 8;
  hsai_BlockA2.FrameInit.ActiveFrameLength = 1;
  hsai_BlockA2.FrameInit.FSDefinition = SAI_FS_STARTFRAME;
  hsai_BlockA2.FrameInit.FSPolarity = SAI_FS_ACTIVE_LOW;
  hsai_BlockA2.FrameInit.FSOffset = SAI_FS_FIRSTBIT;
  hsai_BlockA2.SlotInit.FirstBitOffset = 0;
  hsai_BlockA2.SlotInit.SlotSize = SAI_SLOTSIZE_DATASIZE;
  hsai_BlockA2.SlotInit.SlotNumber = 1;
  hsai_BlockA2.SlotInit.SlotActive = 0x00000000;
  if (HAL_SAI_Init(&hsai_BlockA2) != HAL_OK)
  {
    Error_Handler();
  }
  hsai_BlockB2.Instance = SAI2_Block_B;
  hsai_BlockB2.Init.Protocol = SAI_FREE_PROTOCOL;
  hsai_BlockB2.Init.AudioMode = SAI_MODESLAVE_RX;
  hsai_BlockB2.Init.DataSize = SAI_DATASIZE_8;
  hsai_BlockB2.Init.FirstBit = SAI_FIRSTBIT_MSB;
  hsai_BlockB2.Init.ClockStrobing = SAI_CLOCKSTROBING_FALLINGEDGE;
  hsai_BlockB2.Init.Synchro = SAI_SYNCHRONOUS;
  hsai_BlockB2.Init.OutputDrive = SAI_OUTPUTDRIVE_DISABLE;
  hsai_BlockB2.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_EMPTY;
  hsai_BlockB2.Init.SynchroExt = SAI_SYNCEXT_DISABLE;
  hsai_BlockB2.Init.MonoStereoMode = SAI_STEREOMODE;
  hsai_BlockB2.Init.CompandingMode = SAI_NOCOMPANDING;
  hsai_BlockB2.Init.TriState = SAI_OUTPUT_NOTRELEASED;
  hsai_BlockB2.FrameInit.FrameLength = 8;
  hsai_BlockB2.FrameInit.ActiveFrameLength = 1;
  hsai_BlockB2.FrameInit.FSDefinition = SAI_FS_STARTFRAME;
  hsai_BlockB2.FrameInit.FSPolarity = SAI_FS_ACTIVE_LOW;
  hsai_BlockB2.FrameInit.FSOffset = SAI_FS_FIRSTBIT;
  hsai_BlockB2.SlotInit.FirstBitOffset = 0;
  hsai_BlockB2.SlotInit.SlotSize = SAI_SLOTSIZE_DATASIZE;
  hsai_BlockB2.SlotInit.SlotNumber = 1;
  hsai_BlockB2.SlotInit.SlotActive = 0x00000000;
  if (HAL_SAI_Init(&hsai_BlockB2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SAI2_Init 2 */

  /* USER CODE END SAI2_Init 2 */

}

/**
  * @brief SDMMC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SDMMC1_SD_Init(void)
{

  /* USER CODE BEGIN SDMMC1_Init 0 */

  /* USER CODE END SDMMC1_Init 0 */

  /* USER CODE BEGIN SDMMC1_Init 1 */

  /* USER CODE END SDMMC1_Init 1 */
  hsd1.Instance = SDMMC1;
  hsd1.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
  hsd1.Init.ClockBypass = SDMMC_CLOCK_BYPASS_DISABLE;
  hsd1.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
  hsd1.Init.BusWide = SDMMC_BUS_WIDE_1B;
  hsd1.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
  /* Conservative 8 MHz SD clock for reliable writes on the Discovery socket. */
  hsd1.Init.ClockDiv = 4;
  /* USER CODE BEGIN SDMMC1_Init 2 */

  /* USER CODE END SDMMC1_Init 2 */

}

/**
  * @brief SPDIFRX Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPDIFRX_Init(void)
{

  /* USER CODE BEGIN SPDIFRX_Init 0 */

  /* USER CODE END SPDIFRX_Init 0 */

  /* USER CODE BEGIN SPDIFRX_Init 1 */

  /* USER CODE END SPDIFRX_Init 1 */
  hspdif.Instance = SPDIFRX;
  hspdif.Init.InputSelection = SPDIFRX_INPUT_IN0;
  hspdif.Init.Retries = SPDIFRX_MAXRETRIES_NONE;
  hspdif.Init.WaitForActivity = SPDIFRX_WAITFORACTIVITY_OFF;
  hspdif.Init.ChannelSelection = SPDIFRX_CHANNEL_A;
  hspdif.Init.DataFormat = SPDIFRX_DATAFORMAT_LSB;
  hspdif.Init.StereoMode = SPDIFRX_STEREOMODE_DISABLE;
  hspdif.Init.PreambleTypeMask = SPDIFRX_PREAMBLETYPEMASK_OFF;
  hspdif.Init.ChannelStatusMask = SPDIFRX_CHANNELSTATUS_OFF;
  hspdif.Init.ValidityBitMask = SPDIFRX_VALIDITYMASK_OFF;
  hspdif.Init.ParityErrorMask = SPDIFRX_PARITYERRORMASK_OFF;
  if (HAL_SPDIFRX_Init(&hspdif) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPDIFRX_Init 2 */

  /* USER CODE END SPDIFRX_Init 2 */

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
  hspi2.Init.DataSize = SPI_DATASIZE_4BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
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
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.BreakFilter = 0;
  sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
  sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
  sBreakDeadTimeConfig.Break2Filter = 0;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief TIM5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM5_Init(void)
{

  /* USER CODE BEGIN TIM5_Init 0 */

  /* USER CODE END TIM5_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM5_Init 1 */

  /* USER CODE END TIM5_Init 1 */
  htim5.Instance = TIM5;
  htim5.Init.Prescaler = 0;
  htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim5.Init.Period = 4294967295;
  htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim5) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim5, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim5) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim5, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM5_Init 2 */

  /* USER CODE END TIM5_Init 2 */
  HAL_TIM_MspPostInit(&htim5);

}

/**
  * @brief TIM8 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM8_Init(void)
{

  /* USER CODE BEGIN TIM8_Init 0 */

  /* USER CODE END TIM8_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM8_Init 1 */

  /* USER CODE END TIM8_Init 1 */
  htim8.Instance = TIM8;
  htim8.Init.Prescaler = 0;
  htim8.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim8.Init.Period = 65535;
  htim8.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim8.Init.RepetitionCounter = 0;
  htim8.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim8) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim8, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim8, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM8_Init 2 */

  /* USER CODE END TIM8_Init 2 */

}

/**
  * @brief TIM12 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM12_Init(void)
{

  /* USER CODE BEGIN TIM12_Init 0 */

  /* USER CODE END TIM12_Init 0 */

  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM12_Init 1 */

  /* USER CODE END TIM12_Init 1 */
  htim12.Instance = TIM12;
  htim12.Init.Prescaler = 0;
  htim12.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim12.Init.Period = 65535;
  htim12.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim12.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim12) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim12, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM12_Init 2 */

  /* USER CODE END TIM12_Init 2 */
  HAL_TIM_MspPostInit(&htim12);

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
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 115200;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  huart6.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart6.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

}

/* FMC initialization function */
static void MX_FMC_Init(void)
{

  /* USER CODE BEGIN FMC_Init 0 */

  /* USER CODE END FMC_Init 0 */

  FMC_SDRAM_TimingTypeDef SdramTiming = {0};

  /* USER CODE BEGIN FMC_Init 1 */

  /* USER CODE END FMC_Init 1 */

  /** Perform the SDRAM1 memory initialization sequence
  */
  hsdram1.Instance = FMC_SDRAM_DEVICE;
  /* hsdram1.Init */
  hsdram1.Init.SDBank = FMC_SDRAM_BANK1;
  hsdram1.Init.ColumnBitsNumber = FMC_SDRAM_COLUMN_BITS_NUM_8;
  hsdram1.Init.RowBitsNumber = FMC_SDRAM_ROW_BITS_NUM_12;
  hsdram1.Init.MemoryDataWidth = FMC_SDRAM_MEM_BUS_WIDTH_16;
  hsdram1.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
  hsdram1.Init.CASLatency = FMC_SDRAM_CAS_LATENCY_3;
  hsdram1.Init.WriteProtection = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
  hsdram1.Init.SDClockPeriod = FMC_SDRAM_CLOCK_PERIOD_2;
  hsdram1.Init.ReadBurst = FMC_SDRAM_RBURST_ENABLE;
  hsdram1.Init.ReadPipeDelay = FMC_SDRAM_RPIPE_DELAY_0;
  /* SdramTiming */
  SdramTiming.LoadToActiveDelay = 2;
  SdramTiming.ExitSelfRefreshDelay = 7;
  SdramTiming.SelfRefreshTime = 4;
  SdramTiming.RowCycleDelay = 7;
  SdramTiming.WriteRecoveryTime = 3;
  SdramTiming.RPDelay = 2;
  SdramTiming.RCDDelay = 2;

  if (HAL_SDRAM_Init(&hsdram1, &SdramTiming) != HAL_OK)
  {
    Error_Handler( );
  }

  /* USER CODE BEGIN FMC_Init 2 */

  /* USER CODE END FMC_Init 2 */
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
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOJ_CLK_ENABLE();
  __HAL_RCC_GPIOI_CLK_ENABLE();
  __HAL_RCC_GPIOK_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(OTG_FS_PowerSwitchOn_GPIO_Port, OTG_FS_PowerSwitchOn_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOI, ARDUINO_D7_Pin|ARDUINO_D8_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_Port, LCD_BL_CTRL_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_DISP_GPIO_Port, LCD_DISP_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(DCMI_PWR_EN_GPIO_Port, DCMI_PWR_EN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOG, ARDUINO_D4_Pin|ARDUINO_D2_Pin|EXT_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : OTG_HS_OverCurrent_Pin */
  GPIO_InitStruct.Pin = OTG_HS_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(OTG_HS_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ULPI_D7_Pin ULPI_D6_Pin ULPI_D5_Pin ULPI_D3_Pin
                           ULPI_D2_Pin ULPI_D1_Pin ULPI_D4_Pin */
  GPIO_InitStruct.Pin = ULPI_D7_Pin|ULPI_D6_Pin|ULPI_D5_Pin|ULPI_D3_Pin
                          |ULPI_D2_Pin|ULPI_D1_Pin|ULPI_D4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG_HS;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : OTG_FS_VBUS_Pin */
  GPIO_InitStruct.Pin = OTG_FS_VBUS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(OTG_FS_VBUS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : Audio_INT_Pin */
  GPIO_InitStruct.Pin = Audio_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(Audio_INT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : OTG_FS_PowerSwitchOn_Pin */
  GPIO_InitStruct.Pin = OTG_FS_PowerSwitchOn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(OTG_FS_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ARDUINO_D7_Pin ARDUINO_D8_Pin LCD_DISP_Pin */
  GPIO_InitStruct.Pin = ARDUINO_D7_Pin|ARDUINO_D8_Pin|LCD_DISP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

  /*Configure GPIO pin : uSD_Detect_Pin */
  GPIO_InitStruct.Pin = uSD_Detect_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(uSD_Detect_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LCD_BL_CTRL_Pin */
  GPIO_InitStruct.Pin = LCD_BL_CTRL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LCD_BL_CTRL_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : OTG_FS_OverCurrent_Pin */
  GPIO_InitStruct.Pin = OTG_FS_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(OTG_FS_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : TP3_Pin NC2_Pin */
  GPIO_InitStruct.Pin = TP3_Pin|NC2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

  /*Configure GPIO pin : DCMI_PWR_EN_Pin */
  GPIO_InitStruct.Pin = DCMI_PWR_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DCMI_PWR_EN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LCD_INT_Pin */
  GPIO_InitStruct.Pin = LCD_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(LCD_INT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : ULPI_NXT_Pin */
  GPIO_InitStruct.Pin = ULPI_NXT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG_HS;
  HAL_GPIO_Init(ULPI_NXT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ARDUINO_D4_Pin ARDUINO_D2_Pin EXT_RST_Pin */
  GPIO_InitStruct.Pin = ARDUINO_D4_Pin|ARDUINO_D2_Pin|EXT_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pins : ULPI_STP_Pin ULPI_DIR_Pin */
  GPIO_InitStruct.Pin = ULPI_STP_Pin|ULPI_DIR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG_HS;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : RMII_RXER_Pin */
  GPIO_InitStruct.Pin = RMII_RXER_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(RMII_RXER_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ULPI_CLK_Pin ULPI_D0_Pin */
  GPIO_InitStruct.Pin = ULPI_CLK_Pin|ULPI_D0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG_HS;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
    /* USER CODE BEGIN 5 */

    APP_Init();
    APP_Run();

    for (;;)
    {
        osDelay(10U);
    }

    /* USER CODE END 5 */
}


 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
