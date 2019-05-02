#ifndef _LINKSPRITECAMERA_H_
#define _LINKSPRITECAMERA_H_

#include "lazurite.h"

#define CAMERA_BAUD_9600	(0xaec8)
#define CAMERA_BAUD_19200	(0x56e4)
#define CAMERA_BAUD_38400	(0x2af2)
#define CAMERA_BAUD_57600	(0x1c4c)
#define CAMERA_BAUD_115200	(0x0da6)

typedef enum {
    VGA = 0x00,
    QVGA = 0x11,
    QQVGA = 0x22
} ImageSize;

typedef struct {
	void (*begin)(uint32_t baud_rate);
	void (*end)();
	void (*reset)();
	int (*getSize)();
	void (*takePicture)();
	void (*stopPicture)();
	size_t (*readData)(uint8_t* data, size_t read_size);
	bool (*isEOF)();
	void (*enterPowerSaving)();
	void (*quitPowerSaving)();
	void (*setCompressionRatio)(uint8_t ratio);
	void (*setBaudRate)(uint32_t baud_rate);
	void (*setSize)(ImageSize size);
} LinkSpriteCamera;

extern const LinkSpriteCamera Camera;

#endif /* _LINKSPRITECAMERA_H_ */
