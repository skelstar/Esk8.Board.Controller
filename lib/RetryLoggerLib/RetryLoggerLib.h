#ifndef RetryLoggerLib_h
#define RetryLoggerLib_h

#include <Arduino.h>

class RetryLoggerLib
{
  #define RETRIES_REG_LEN 100

  public:

    RetryLoggerLib(uint8_t register_len)
    {
      if (register_len > RETRIES_REG_LEN)
      {
        Serial.printf("**** ERROR: RETRIES_REG_LEN = %d ****", RETRIES_REG_LEN);
        return;
      }
      _register_len = register_len;
    }

    void log(uint8_t retries)
    {
      retries_reg[idx] = retries > 0;
      idx = idx < _register_len - 1 
        ? idx + 1 
        : 0;
      if (log_count < _register_len)
      {
        log_count++;
      }
    }

    void reset()
    {
      for (int i=0; i<_register_len; i++)
      {
        log_count = 0;
        retries_reg[i] = false;
        idx = 0;
      }
    }

    float get_retry_rate()
    {
      uint8_t sum = get_sum_retries();
      return sum / (log_count < _register_len ? log_count * 1.0 : _register_len * 1.0);
    }

    uint8_t get_sum_retries()
    {
      uint8_t sum_retries = 0;
      for (int i = 0; i < _register_len; i++)
      {
        if (retries_reg[i] == true)
        {
          sum_retries++;
        }
      }
      return sum_retries;
    }

  private:
    bool retries_reg[RETRIES_REG_LEN];
    uint8_t idx = 0;
    unsigned long log_count;
    uint8_t _register_len;
};

#endif