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

} // end namespace