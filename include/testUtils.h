#include <QueueManager1.h>

#define ASSERT(condition, message)                                                        \
  do                                                                                      \
  {                                                                                       \
    if (!(condition))                                                                     \
    {                                                                                     \
      Serial.printf("Assertion: %s | file: %s line: %s \n", message, __FILE__, __LINE__); \
    }                                                                                     \
  } while (false)

namespace Test
{
  typedef void (*ResponseCallback)(unsigned long id, unsigned long latency);

  template <typename T>
  bool waitForNewResponse(Queue1::Manager<T> *queue,
                          bool &reponseOut,
                          bool &timedoutOut,
                          uint16_t timeout,
                          ResponseCallback gotResponse_cb = nullptr)
  {
    reponseOut = false;
    timedoutOut = false;
    elapsedMillis since_started_listening = 0;
    do
    {
      if (queue->hasValue())
      {
        reponseOut = true;
        since_started_listening = 0;
        if (gotResponse_cb != nullptr)
          gotResponse_cb(queue->payload.event_id, queue->payload.latency);
      }
      timedoutOut = since_started_listening > timeout;
      vTaskDelay(1);
    } while (!reponseOut && !timedoutOut);
    return reponseOut && !timedoutOut;
  }

  enum WaitResp
  {
    OK = 0,
    TIMEOUT,
  };

  template <typename T>
  WaitResp waitForNewResp(Queue1::Manager<T> *queue,
                          uint16_t timeout,
                          ResponseCallback gotResponse_cb = nullptr)
  {
    elapsedMillis since_started_listening = 0;
    do
    {
      if (queue->hasValue())
      {
        if (gotResponse_cb != nullptr)
          gotResponse_cb(queue->payload.event_id, queue->payload.latency);
        return OK;
      }
      vTaskDelay(1);
    } while (since_started_listening < timeout);
    return TIMEOUT;
  }

} // end namespace