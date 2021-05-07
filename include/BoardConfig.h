
#define DIR_CLOCKWISE 0
#define DIR_ANTI_CLOCKWISE 1
#define IS_ESP32
#define LONGCLICK_MS 1000
#define PRINT_THIS 1

//-----------------------------------------
#if REMOTE_USED == RED_REMOTE
#define IS_ESP32
#define THROTTLE_PIN 27
#define PRIMARY_BUTTON_PIN 21
#define THROTTLE_RAW_MAX 2587
#define THROTTLE_RAW_CENTRE 1280
#define THROTTLE_RAW_MIN 0
#define THROTTLE_RAW_DEADBAND 50
#define BATTERY_MEASURE_PIN 34

#define SOFT_SPI_MOSI_PIN 13 // Blue
#define SOFT_SPI_MISO_PIN 12 // Orange
#define SOFT_SPI_SCK_PIN 15  // Yellow
#define NRF_CE 17
#define NRF_CS 2
#define SOFTSPI

#endif
//-----------------------------------------
#if REMOTE_USED == NINTENDO_REMOTE

// #define IS_ESP32
// #define LIMIT_DELTA_MAX 50.0
// #define LIMIT_DELTA_MIN 3.0
// #define THROTTLE_DIRECTION DIR_CLOCKWISE
// #define BATTERY_MEASURE_PIN 34

// #define SOFT_SPI_MOSI_PIN 13 //Blue
// #define SOFT_SPI_MISO_PIN 25 //Orange
// #define SOFT_SPI_SCK_PIN 15  //Yellow
// #define NRF_CE 17
// #define NRF_CS 2
// #define SOFTSPI

// #define USING_DISPLAY 1

#endif