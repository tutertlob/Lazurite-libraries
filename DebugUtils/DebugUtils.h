#ifndef DEBUGUTILS_H
#define DEBUGUTILS_H
#include <lazurite.h>

#ifndef NDEBUG
#define DEBUG_PRINT(str)                    \
    Serial.print_long(millis(), DEC);       \
    Serial.print(": ");                     \
    Serial.print(__FILE__);                 \
    Serial.print(":");                      \
    Serial.print_long((long)__LINE__, DEC); \
    Serial.print(": ");                     \
    Serial.println(str);                    \
    Serial.flush()

#define DEBUG_PRINT_LONG(data, format)      \
    Serial.print_long(millis(), DEC);       \
    Serial.print(": ");                     \
    Serial.print(__FILE__);                 \
    Serial.print(":");                      \
    Serial.print_long((long)__LINE__, DEC); \
    Serial.print(": ");                     \
    Serial.println_long(data, format);      \
    Serial.flush()

#define DEBUG_PRINT_BYTE(data)      \
    Serial.print_long(millis(), DEC);       \
    Serial.print(": ");                     \
    Serial.print(__FILE__);                 \
    Serial.print(":");                      \
    Serial.print_long((long)__LINE__, DEC); \
    Serial.print(": ");                     \
    Serial.write_byte(data);                \
    Serial.flush()

#define DEBUG_WRITE(data, length)           \
    Serial.print_long(millis(), DEC);       \
    Serial.print(": ");                     \
    Serial.print(__FILE__);                 \
    Serial.print(":");                      \
    Serial.print_long((long)__LINE__, DEC); \
    Serial.print(": ");                     \
    Serial.write(data, length);             \
    Serial.println("");                     \
    Serial.flush()
#else
#ifndef DEBUG_PRINT
  #define DEBUG_PRINT(str)
  #define DEBUG_PRINT_LONG(data, format)
  #define DEBUG_PRINT_BYTE(data)
  #define DEBUG_WRITE(data, length)
#endif
#endif /* NDEBUG */

#endif //DEBUGUTILS_H
