#include <ChunkyDigit.h>
#include <math.h>

#define LINE_1 1
#define LINE_2 2
#define LINE_3 3

// prototypes
void screen_with_stats();

//-----------------------------------------------------

void screen_searching()
{
  u8g2.clearBuffer();
  lcd_message("Searching...", MC_DATUM);
  u8g2.sendBuffer();
}
//-----------------------------------------------------

void screen_disconnected()
{
  u8g2.clearBuffer();
  lcd_message("ِDisconnected :(", MC_DATUM);
  u8g2.sendBuffer();
  // screen_with_stats();
}
//-----------------------------------------------------

void screen_with_stats()
{
  u8g2.clearBuffer();
  // line 1
  char buff1[20];
  float retry_rate = retry_log.get();
  sprintf(buff1, "rate: %.1f%%", isnan(retry_rate) ? 0.0 : retry_rate);
  lcd_message(1, buff1, Aligned::ALIGNED_LEFT);
  // line 2
  char buff2[20];
  sprintf(buff2, "total: %lu", stats.total_failed);
  lcd_message(2, buff2, Aligned::ALIGNED_LEFT);
  // line 3
  char buff3[20];
  sprintf(buff3, "w/rt: %lu", stats.num_packets_with_retries);
  lcd_message(3, buff3, Aligned::ALIGNED_LEFT);
  // battery
  draw_small_battery(remote_battery_percent, 128 - SM_BATT_WIDTH, 0);
  // deadman
  draw_trigger_state(trigger.get_current_state(), BR_DATUM);
  u8g2.sendBuffer();
}
//-----------------------------------------------------

void screen_moving()
{
  char buff[10];

  ChunkyDigit chunky_digit(&u8g2, 6, 3);

  u8g2.clearBuffer();
  sprintf(buff, "%.1f%%", retry_log.get());
  chunky_digit.draw_float(TR_DATUM, buff, NULL);
  draw_trigger_state(trigger.get_current_state(), BR_DATUM);
  u8g2.sendBuffer();
}
//-----------------------------------------------------