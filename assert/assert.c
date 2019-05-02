#include "common.h"
#include "lazurite.h"
#include <ML620504F.h>
#include "assert.h"

void __assert_print(const char *assertion, const char *file, unsigned int line)
{
    Serial.print(assertion);
    Serial.print(" at ");
    Serial.print(file);
    Serial.print("::");
    Serial.println_long((long)line, DEC);
}

void __assert_dhalt(const char *assertion, const char *file, unsigned int line)
{
    __assert_print(assertion, file, line);
    DHLT = 1;
}

void __assert_stop(const char *assertion, const char *file, unsigned int line)
{
    __assert_print(assertion, file, line);
    STPACP = 0x50;
    STPACP = 0xA0;
    STP = 1;
}

void __assert_brk(const char *assertion, const char *file, unsigned int line)
{
    __assert_print(assertion, file, line);
    __asm("brk");
}
