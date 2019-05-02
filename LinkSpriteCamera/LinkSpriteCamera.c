#include "LinkSpriteCamera.h"
#include "DebugUtils.h"
#include "assert.h"

#define DEFAULT_CAMERA_SERIAL_BAUD_RATE	(38400)

#ifndef CAMERA_SERIAL
#define CAMERA_SERIAL	Serial3
#endif /* CAMERA_SERIAL */	

static void LinkSpriteCamera_begin(uint32_t baud_rate);
static void LinkSpriteCamera_end();
static void LinkSpriteCamera_reset();
static int LinkSpriteCamera_getSize();
static void LinkSpriteCamera_takePicture();
static void LinkSpriteCamera_stopPicture();
static size_t LinkSpriteCamera_readData(uint8_t* data, size_t read_size);
static bool LinkSpriteCamera_isEOF();
static void LinkSpriteCamera_enterPowerSaving();
static void LinkSpriteCamera_quitPowerSaving();
static void LinkSpriteCamera_setCompressionRatio(uint8_t ratio);
static void LinkSpriteCamera_setBaudRate(uint32_t baud_rate);
static void LinkSpriteCamera_setSize(ImageSize size);
static void LinkSpriteCamera_sendCommand(const uint8_t cmd[], size_t cmdLen, uint8_t res[], size_t resLen);

static uint32_t LinkSpriteCamera_toSerialBaud(uint32_t baud);

static uint32_t __camera_serial_baudrate = DEFAULT_CAMERA_SERIAL_BAUD_RATE;
static int	__camera_address;
static bool __camera_eof;
static int __camera_imageSize;

const LinkSpriteCamera Camera = {
	LinkSpriteCamera_begin,
	LinkSpriteCamera_end,
	LinkSpriteCamera_reset,
	LinkSpriteCamera_getSize,
	LinkSpriteCamera_takePicture,
	LinkSpriteCamera_stopPicture,
	LinkSpriteCamera_readData,
	LinkSpriteCamera_isEOF,
	LinkSpriteCamera_enterPowerSaving,
	LinkSpriteCamera_quitPowerSaving,
	LinkSpriteCamera_setCompressionRatio,
	LinkSpriteCamera_setBaudRate,
	LinkSpriteCamera_setSize
};

void LinkSpriteCamera_begin(uint32_t baud_rate)
{
	uint32_t serial_baud = LinkSpriteCamera_toSerialBaud(baud_rate);
	CAMERA_SERIAL.begin(serial_baud);
	__camera_serial_baudrate = serial_baud;
	sleep(1);
	LinkSpriteCamera_reset();
}

void LinkSpriteCamera_end()
{
	LinkSpriteCamera_enterPowerSaving();
	CAMERA_SERIAL.end();
}

void LinkSpriteCamera_reset()
{
	const uint8_t cmd[] = {0x56, 0x00, 0x26, 0x00};
	uint8_t res[20];
	const char initEnd[] = "Init end\r";

	while (CAMERA_SERIAL.available() > 0)
		CAMERA_SERIAL.read();

	DEBUG_PRINT("Resetting the camera...");

	// Reset the camera
	LinkSpriteCamera_sendCommand(cmd, sizeof(cmd), res, 4);

#ifndef NDEBUG
	{
		const uint8_t match[] = { 0x76, 0x00, 0x26, 0x00 };
		assert(memcmp(res, match, 4) == 0);
	}
#endif /* NDEBUG */

	do {
		size_t count;
		for (count = 0; (sizeof(res) - count) > 0;) {
			int data;
			data = CAMERA_SERIAL.read();
			if (data == -1) continue;
			if ((char)data == '\n') break;
			res[count] = (uint8_t)data;
			count++;
		}
		if (count >= sizeof(res)) {
		 	count = sizeof(res) - 1;
		}
		res[count] = 0;
		DEBUG_PRINT(res);
	} while(strcmp(initEnd, res) != 0);
	assert(strcmp(initEnd, res) == 0);

	DEBUG_PRINT("Resetting the camera is done.");
}

int LinkSpriteCamera_getSize()
{
	const uint8_t cmd[] = {0x56, 0x00, 0x34, 0x01, 0x00};
	uint8_t res[9];
	int fileSize = 0;

	LinkSpriteCamera_sendCommand(cmd, sizeof(cmd), res, sizeof(res));
#ifndef NDEBUG
	{
		const uint8_t match[] = {0x76, 0x00, 0x34, 0x00, 0x04, 0x00, 0x00};
		assert(memcmp(res, match, sizeof(res) - 2) == 0);
	}
#endif /* NDEBUG */

	fileSize = (res[7] << 8);
	fileSize += res[8];

#ifndef NDEBUG
	Serial.print("filesize=");
	Serial.println_long((long)fileSize, DEC);
	Serial.flush();
#endif /* NDEBUG */

	return fileSize;
}

void LinkSpriteCamera_takePicture()
{
	const uint8_t cmd[] = {0x56, 0x00, 0x36, 0x01, 0x00};
	uint8_t res[5];

	LinkSpriteCamera_sendCommand(cmd, sizeof(cmd), res, sizeof(res));
#ifndef NDEBUG
	{
		const uint8_t match[] = {0x76, 0x00, 0x36, 0x00, 0x00};
		assert(memcmp(res, match, sizeof(res)) == 0);
	}
#endif /* NDEBUG */

	__camera_eof = false;
	__camera_address = 0;
	__camera_imageSize = LinkSpriteCamera_getSize();

	DEBUG_PRINT("Captured a picture.");
}

void LinkSpriteCamera_stopPicture()
{
	const uint8_t cmd[] = {0x56, 0x00, 0x36, 0x01, 0x03};
	uint8_t res[5];

	LinkSpriteCamera_sendCommand(cmd, sizeof(cmd), res, sizeof(res));
#ifndef NDEBUG
	{
		const uint8_t match[] = { 0x76, 0x00, 0x36, 0x00, 0x00 };
		assert(memcmp(res, match, sizeof(res)) == 0);
	}
#endif /* NDEBUG */
}

size_t LinkSpriteCamera_readData(uint8_t* data, size_t read_size)
{
	size_t readBytes;
	
	read_size = read_size & ~0x07;
#ifndef NDEBUG
	DEBUG_PRINT("readData");
	Serial.print("read_size=");
	Serial.println_long((long)read_size, DEC);
#endif /* NDEBUG */

	{
		const uint8_t cmd[] = {0x56, 0x00, 0x32, 0x0c, 0x00, 0x0a, 0x00, 0x00};
		uint8_t res[5];
		size_t count;

		DEBUG_PRINT("camera_address=");
		DEBUG_PRINT_LONG(__camera_address >> 8, HEX);
		DEBUG_PRINT_LONG(__camera_address & 0xff, HEX);
		DEBUG_PRINT("read_size=");
		DEBUG_PRINT_LONG(read_size >> 8, HEX);
		DEBUG_PRINT_LONG(read_size & 0xff, HEX);
		CAMERA_SERIAL.write(cmd, sizeof(cmd));
		CAMERA_SERIAL.write_byte((uint8_t)(__camera_address >> 8));
		CAMERA_SERIAL.write_byte((uint8_t)(__camera_address & 0xff));
		CAMERA_SERIAL.write_byte((uint8_t)0x00);
		CAMERA_SERIAL.write_byte((uint8_t)0x00);
		CAMERA_SERIAL.write_byte((uint8_t)(read_size >> 8));
		CAMERA_SERIAL.write_byte((uint8_t)(read_size & 0xff));
		CAMERA_SERIAL.write_byte((uint8_t)0x00);
		CAMERA_SERIAL.write_byte((uint8_t)0x64);
		// CAMERA_SERIAL.flush();

		// wait for 0x0a times 0.01 msec
		// delay(1);

		// Read response of the Read JPEG file content command
		for (count = 0; count < sizeof(res);) {
			int data = CAMERA_SERIAL.read();
			if (data == -1) continue;
			res[count] = (uint8_t)data;
			// DEBUG_PRINT_LONG(res[count], HEX);
			count++;
		}

#ifndef NDEBUG
		{
			const char match[] = {0x76, 0x00, 0x32, 0x00, 0x00};
			assert(memcmp(res, match, sizeof(res)) == 0);
		}
#endif /* NDEBUG */
	}

	{
		size_t len;
		size_t size = ((size_t)(__camera_imageSize - __camera_address) > read_size) ? read_size : (size_t)(__camera_imageSize - __camera_address);
		for (readBytes = 0; readBytes < size;) {
			int ch;
			ch = CAMERA_SERIAL.read();
			if (ch == -1) continue;
			data[readBytes] = (uint8_t)ch;
			readBytes++;
		}
		__camera_address += readBytes;

	#ifndef NDEBUG
		Serial.print("Read bytes=");
		Serial.print_long((long)readBytes, DEC);
		Serial.print(" Address=");
		Serial.print_long((long)__camera_address, DEC);
		Serial.print(" Image data remaining=");
		Serial.println_long((long)(__camera_imageSize - __camera_address), DEC);
	#endif /* NDEBUG */

		if ((data[readBytes - 1] == 0xD9) && (data[readBytes - 2] == 0xFF)) {
			__camera_eof = true;
	#ifndef NDEBUG
			Serial.println("=== EOF ===");
	#endif /* NDEBUG */
		}

		// 5 for 0x76, 0x00, 0x32, 0x00, 0x00
		for (len = 0; len < read_size - size + 5; ) {
			int ch;
			ch = CAMERA_SERIAL.read();
			if (ch == -1) continue;
			++len;
		}
	}
	
	return readBytes;
}

bool LinkSpriteCamera_isEOF()
{
	return (__camera_eof);
}

void LinkSpriteCamera_enterPowerSaving()
{
	const uint8_t cmd[] = {0x56, 0x00, 0x3E, 0x03, 0x00, 0x01, 0x01};
	uint8_t res[5];

	DEBUG_PRINT("enterPowerSaving");
	LinkSpriteCamera_sendCommand(cmd, sizeof(cmd), res, sizeof(res));
#ifndef NDEBUG
	{
		const char match[] = {0x76, 0x00, 0x3E, 0x00, 0x00};
		assert(memcmp(res, match, sizeof(res)) == 0);
	}
#endif /* NDEBUG */
	CAMERA_SERIAL.end();
}

void LinkSpriteCamera_quitPowerSaving()
{
	const uint8_t cmd[] = {0x56, 0x00, 0x3E, 0x03, 0x00, 0x01, 0x00};
	uint8_t res[5];

	DEBUG_PRINT("quitPowerSaving");
	CAMERA_SERIAL.begin(__camera_serial_baudrate);
	LinkSpriteCamera_sendCommand(cmd, sizeof(cmd), res, sizeof(res));
#ifndef NDEBUG
	{
		const char match[] = {0x76, 0x00, 0x3E, 0x00, 0x00};
		assert(memcmp(res, match, sizeof(res)) == 0);
	}
#endif /* NDEBUG */
}

void LinkSpriteCamera_setCompressionRatio(uint8_t ratio)
{
	uint8_t res[5];
	uint8_t cmd[] = {0x56, 0x00, 0x31, 0x05, 0x01, 0x01, 0x12, 0x04, 0x00};

	cmd[sizeof(cmd) - 1] = ratio;
	LinkSpriteCamera_sendCommand(cmd, sizeof(cmd), res, sizeof(res));
#ifndef NDEBUG
	{
		const char match[] = {0x76, 0x00, 0x31, 0x00, 0x00};
		assert(memcmp(res, match, sizeof(res)) == 0);
	}
#endif /* NDEBUG */
}

void LinkSpriteCamera_setBaudRate(uint32_t baud_rate)
{
	uint32_t serialBaud = LinkSpriteCamera_toSerialBaud(baud_rate);
	uint8_t cmd[] = {0x56, 0x00, 0x24, 0x03, 0x01, 0x00, 0x00};
	uint8_t res[5];

	cmd[sizeof(cmd) - 2] = (uint8_t)(baud_rate >> 8);
	cmd[sizeof(cmd) - 1] = (uint8_t)(baud_rate & 0xff);
	
	LinkSpriteCamera_sendCommand(cmd, sizeof(cmd), res, sizeof(res));
#ifndef NDEBUG
	{
		const char match[] = {0x76, 0x00, 0x24, 0x00, 0x00};
		assert(memcmp(res, match, sizeof(res)) == 0);
	}
#endif /* NDEBUG */

	CAMERA_SERIAL.end();
	CAMERA_SERIAL.begin(serialBaud);
	__camera_serial_baudrate = serialBaud;
	sleep(1);
}

void LinkSpriteCamera_setSize(ImageSize size)
{
	uint8_t cmd[] = {0x56, 0x00, 0x31, 0x05, 0x04, 0x01, 0x00, 0x19, 0x00};
	uint8_t res[5];

	cmd[sizeof(cmd) - 1] = (uint8_t)size;
	LinkSpriteCamera_sendCommand(cmd, sizeof(cmd), res, sizeof(res));
#ifndef NDEBUG
	{
		const char match[] = {0x76, 0x00, 0x31, 0x00, 0x00};
		assert(memcmp(res, match, sizeof(res)) == 0);
	}
#endif /* NDEBUG */

	LinkSpriteCamera_reset();
}

void LinkSpriteCamera_sendCommand(const uint8_t cmd[], size_t cmdLen, uint8_t res[], size_t resLen)
{
	size_t count;

	#ifndef NDEBUG
	Serial.print("Link Sprite Camera Command = ");
	Serial.write(cmd, cmdLen);
	Serial.println("");
	#endif /* NDEBUG */

	CAMERA_SERIAL.write(cmd, cmdLen);
	CAMERA_SERIAL.flush();

	for (count = 0; (resLen - count) > 0;) {
		int data = CAMERA_SERIAL.read();
		if (data == -1) continue;
		res[count] = (uint8_t)data;
		DEBUG_PRINT_LONG(res[count], HEX);
		count++;
	}

	#ifndef NDEBUG
	Serial.println("");
	Serial.print("Result = ");
	Serial.write(res, resLen);
	Serial.println("");
	#endif /* NDEBUG */
}

uint32_t LinkSpriteCamera_toSerialBaud(uint32_t baud)
{
	switch (baud) {
		case CAMERA_BAUD_9600:
			return 9600;
		case CAMERA_BAUD_19200:
			return 19200;
		case CAMERA_BAUD_38400:
			return 38400;
		case CAMERA_BAUD_57600:
			return 57600;
		case CAMERA_BAUD_115200:
			return 115200;
		default:
			return DEFAULT_CAMERA_SERIAL_BAUD_RATE;
	}
}