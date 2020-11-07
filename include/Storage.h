#define STORAGE_H 1

template <typename T>
void storeInMemory(char *key, T value)
{
  statsStore.begin(STORE_STATS, /*read-only*/ false);
  if (std::is_same<T, uint16_t>::value)
  {
    statsStore.putUInt(key, value);
  }
  else if (std::is_same<T, unsigned long>::value)
  {
    statsStore.putULong(key, value);
  }
  else
  {
    Serial.printf("WARNING: unhandled type for storeInMemory()\n");
  }
  statsStore.end();
}

template <typename T>
T readFromMemory(char *key)
{
  T result;
  statsStore.begin(STORE_STATS, /*read-only*/ false);
  if (std::is_same<T, uint16_t>::value)
  {
    result = statsStore.getUInt(key, 0);
  }
  else if (std::is_same<T, unsigned long>::value)
  {
    result = statsStore.getULong(key, 0);
  }
  else
  {
    Serial.printf("WARNING: unhandled type for storeInMemory()\n");
  }
  statsStore.end();
  return result;
}