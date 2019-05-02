#ifndef _LAZURITE_WIRELESS_H_
#define _LAZURITE_WIRELESS_H_

#include "lazurite.h"
#include <stdlib.h>
#include <string.h>
#include "assert.h"

typedef enum {
    DATA = 0,
    COMMAND = 1,
    ACK = 2,
    NOTICE = 3
} PacketType;

typedef void Packet;

typedef struct {
    void (*initialize)();
} PacketInterfaceBase;

typedef struct {
    SUBGHZ_MSG (*init)();
    SUBGHZ_MSG (*begin)(uint8_t ch, uint16_t panid, SUBGHZ_RATE rate, SUBGHZ_POWER txPower);
    SUBGHZ_MSG (*end)();
    SUBGHZ_MSG (*enableRx)();
    SUBGHZ_MSG (*disableRx)();
    int (*listen)(Packet *);
    SUBGHZ_MSG (*send)(const Packet *, uint16_t panid, uint16_t dstAddr);
    size_t (*sendData)(uint16_t panid, uint16_t dstAddr, const uint8_t data[], size_t size, bool fragmented);
    int (*sendCommand)(uint16_t panid, uint16_t dstAddr, uint8_t cmd, const char param[]);
    int (*sendCommandWithAck)(uint16_t panid, uint16_t dstAddr, uint8_t cmd, const char param[]);
    int (*sendAck)(uint16_t panid, uint16_t dstAddr, uint8_t cmd, const char response[]);
    int (*sendNotice)(uint16_t panid, uint16_t dstAddr, const char notice[]);
    uint8_t (*getAddrType)();
    uint16_t (*getMyAddress)();
    SUBGHZ_MSG (*setAckReq)(bool on);
    SUBGHZ_MSG (*setBroadcastEnb)(bool on);
    SUBGHZ_MSG (*setPromiscuous)(bool on);
    uint8_t (*setTxRetry)();
    SUBGHZ_MSG (*setSendMode)(uint8_t addrType, uint8_t txRetry);
} LazuriteWireless;

typedef struct {
    PacketInterfaceBase    base;
    uint8_t (*getCommand)(const Packet * const);
    const char* (*getResponse)(const Packet * const);
    // char* (*getResponseArray)(Packet * const);
    size_t (*getResponseLength)(const Packet * const);
    void (*setCommand)(Packet * const, uint8_t command);
    size_t (*setResponse)(Packet * const, const char response[]);
    // int (*resetResponseLength)(Packet * const, size_t length);
} Ack;

typedef struct {
    PacketInterfaceBase    base;
    void (*enableAckRequest)(Packet * const);
    bool (*isResponseRequested)(const Packet * const);
    uint8_t (*getCommand)(const Packet * const);
    const char* (*getCommandParam)(const Packet * const);
    // char* (*getCommandParamArray)(Packet * const);
    size_t (*getCommandParamLength)(const Packet * const);
    void (*setCommand)(Packet * const, uint8_t command);
    size_t (*setCommandParam)(Packet * const, const char param[]);
    void (*setResponseRequested)(Packet * const, bool requested);
    // int (*resetCommandParamLength)(Packet * const, size_t length);
} Command;

typedef struct {
    PacketInterfaceBase    base;
    bool (*isFragmented)(const Packet * const);
    const uint8_t* (*getData)(const Packet * const);
    uint8_t* (*getDataArray)(Packet * const);
    size_t (*getDataSize)(const Packet * const);
    size_t (*setData)(Packet * const, const uint8_t from[], size_t size);
    void (*setFragmented)(Packet * const, bool fragmented);
    int (*resetDataSize)(Packet * const, size_t size);
} Data;

typedef struct {
    PacketInterfaceBase    base;
    const char* (*getNotice)(const Packet * const);
    // char* (*getNoticeArray)(Packet * const);
    size_t (*getNoticeLength)(const Packet * const);
    size_t (*setNotice)(Packet * const, const char notice[]);
    // int (*resetNoticeLength)(Packet * const, size_t length);
} Notice;

extern Packet * Packet_new();
extern void Packet_free(Packet *);
extern PacketType Packet_getType(const Packet * const);
extern PacketInterfaceBase* Packet_getInterface(const Packet * const);
extern void Packet_initialize(Packet * const);
extern void Packet_setType(Packet * const, PacketType);

extern const LazuriteWireless Wireless;

#endif /* _LAZURITE_WIRELESS_H_ */
