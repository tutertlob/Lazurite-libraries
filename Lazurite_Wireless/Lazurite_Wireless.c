#include "Lazurite_Wireless.h"
#include <DebugUtils.h>

#define LAZURITE_PAYLOAD_SIZE	        (250 - 11)
#define LAZURITE_PACKET_HEADER_SIZE     1
#define LAZURITE_PACKET_BODY_SIZE       (LAZURITE_PAYLOAD_SIZE - LAZURITE_PACKET_HEADER_SIZE)

#define LAZURITE_PACKET_TYPE_I			0
#define LAZURITE_PACKET_TYPE_MASK		(0x07)
#define LAZURITE_PACKET_FLAG_I			0
#define LAZURITE_PACKET_FLAG_MASK		(0x18)
#define LAZURITE_PACKET_FLAG_MASK_FRAG	(0x10)
#define LAZURITE_PACKET_FLAG_MASK_ACK	(0x08)

#define LAZURITE_ACK_CMD_I	        0
#define LAZURITE_ACK_COMMAND_SIZE   1
#define LAZURITE_ACK_RESPONSE_I         (LAZURITE_ACK_CMD_I + LAZURITE_ACK_COMMAND_SIZE)
#define LAZURITE_ACK_RESPONSE_MAX_LEN   (LAZURITE_PACKET_BODY_SIZE - LAZURITE_ACK_COMMAND_SIZE)

#define LAZURITE_COMMAND_CMD_I          0
#define LAZURITE_COMMAND_CMD_SIZE       1
#define LAZURITE_COMMAND_PARAM_I         (LAZURITE_COMMAND_CMD_I + LAZURITE_COMMAND_CMD_SIZE)
#define LAZURITE_COMMAND_PARAM_MAX_LEN   (LAZURITE_PACKET_BODY_SIZE - LAZURITE_COMMAND_CMD_SIZE)

#define LAZURITE_DATA_MAX_SIZE      (LAZURITE_PACKET_BODY_SIZE)

#define LAZURITE_NOTICE_MAX_SIZE      (LAZURITE_PACKET_BODY_SIZE)


typedef struct {
    uint8_t _payload[LAZURITE_PAYLOAD_SIZE+1];
    size_t _length;
} Payload;

static uint8_t* Payload_getBodyArray(Payload * const self);
static size_t Payload_getLength(const Payload * const self);
static PacketType Payload_getPacketType(const Payload * const self);
static uint8_t* Payload_getPayloadArray(Payload * const self);
static size_t Payload_getPayloadLength(Payload * const self);
static bool Payload_isFragmented(const Payload * const self);
static bool Payload_isResponseRequested(Payload * const self);
static void Payload_setFragmented(Payload * const self, bool fragment);
static void Payload_setResponseRequested(Payload * const self, bool requested);
static void Payload_setPacketType(Payload * const self, PacketType type);
// static void Payload_setPayload(Payload * const self, uint8_t from[], size_t length);
// static void Payload_resetPayloadLength(Payload * const self, size_t length);
static void Payload_resetLength(Payload * const self, size_t length);

static SUBGHZ_MSG LazuriteWireless_init();
static SUBGHZ_MSG LazuriteWireless_begin(uint8_t ch, uint16_t panid, SUBGHZ_RATE rate, SUBGHZ_POWER txPower);
static SUBGHZ_MSG LazuriteWireless_end();
static int LazuriteWireless_listen(Packet *packet);
static SUBGHZ_MSG LazuriteWireless_send(const Packet * const packet, uint16_t panid, uint16_t dstAddr);
static size_t LazuriteWireless_sendData(uint16_t panid, uint16_t dstAddr, const uint8_t data[], size_t size, bool fragmented);
static int LazuriteWireless_sendCommand(uint16_t panid, uint16_t dstAddr, uint8_t cmd, const char param[]);
static int LazuriteWireless_sendCommandWithAck(uint16_t panid, uint16_t dstAddr, uint8_t cmd, const char param[]);
static int LazuriteWireless_sendAck(uint16_t panid, uint16_t dstAddr, uint8_t cmd, const char response[]);
static int LazuriteWireless_sendNotice(uint16_t panid, uint16_t dstAddr, const char notice[]);
static SUBGHZ_MSG LazuriteWireless_enableRx();
static SUBGHZ_MSG LazuriteWireless_disableRx();
static uint8_t LazuriteWireless_getAddrType();
static uint16_t LazuriteWireless_getMyAddress();
static SUBGHZ_MSG LazuriteWireless_setAckReq(bool on);
static SUBGHZ_MSG LazuriteWireless_setBroadcastEnb(bool on);
static SUBGHZ_MSG LazuriteWireless_setPromiscuous(bool on);
static uint8_t LazuriteWireless_setTxRetry();
static SUBGHZ_MSG LazuriteWireless_setSendMode(uint8_t addrType, uint8_t txRetry);
static void LazuriteWireless_callback(uint8_t rssi, uint8_t status);

static uint8_t Ack_getCommand(const Packet * const self);
static const char* Ack_getResponse(const Packet * const self);
// static char* Ack_getResponseArray(Packet * const self);
static size_t Ack_getResponseLength(const Packet * const self);
static void Ack_initialize(Packet * const self);
static void Ack_setCommand(Packet * const self, uint8_t command);
static size_t Ack_setResponse(Packet * const self, const char response[]);
// static int Ack_resetResponseLength(Packet * const self, size_t length);
static void Command_initialize(Packet * const self);
static void Command_enableAckRequest(Packet * const self);
static bool Command_isResponseRequested(const Packet * const self);
static uint8_t Command_getCommand(const Packet * const self);
static const char* Command_getCommandParam(const Packet * const self);
static size_t Command_getCommandParamLength(const Packet * const self);
static void Command_setCommand(Packet * const self, uint8_t command);
static size_t Command_setCommandParam(Packet * const self, const char param[]);
static void Command_setResponseRequested(Packet * const self, bool requested);
// static int Command_resetCommandParamLength(Packet * const self, size_t length);
static void Data_initialize(Packet * const self);
static bool Data_isFragmented(const Packet * const self);
static const uint8_t* Data_getData(const Packet * const self);
static uint8_t* Data_getDataArray(Packet * const self);
static size_t Data_getDataSize(const Packet * const self);
static size_t Data_setData(Packet * const self, const uint8_t from[], size_t size);
static void Data_setFragmented(Packet * const self, bool fragmented);
static int Data_resetDataSize(Packet * const self, size_t size);
static const char* Notice_getNotice(const Packet * const self);
// static char* Notice_getNoticeArray(Packet * const self);
static size_t Notice_getNoticeLength(const Packet * const self);
static void Notice_initialize(Packet * const self);
static size_t Notice_setNotice(Packet * const self, const char notice[]);
// static int Notice_resetNoticeLength(Packet * const self, size_t length);

const LazuriteWireless Wireless = {
    LazuriteWireless_init,
    LazuriteWireless_begin,
    LazuriteWireless_end,
    LazuriteWireless_enableRx,
    LazuriteWireless_disableRx,
    LazuriteWireless_listen,
    LazuriteWireless_send,
    LazuriteWireless_sendData,
    LazuriteWireless_sendCommand,
    LazuriteWireless_sendCommandWithAck,
    LazuriteWireless_sendAck,
    LazuriteWireless_sendNotice,
    LazuriteWireless_getAddrType,
    LazuriteWireless_getMyAddress,
    LazuriteWireless_setAckReq,
    LazuriteWireless_setBroadcastEnb,
    LazuriteWireless_setPromiscuous,
    LazuriteWireless_setTxRetry,
    LazuriteWireless_setSendMode
};

static Payload __payload;
static Packet * __packet = (Packet *)&__payload;



static SUBGHZ_MSG LazuriteWireless_init()
{
    SUBGHZ_MSG ret;

    ret = SubGHz.init();
    assert(ret == SUBGHZ_OK);

    return ret;
}

static SUBGHZ_MSG LazuriteWireless_begin(uint8_t ch, uint16_t panid, SUBGHZ_RATE rate, SUBGHZ_POWER txPower)
{
    SUBGHZ_MSG ret;

    ret = SubGHz.begin(ch, panid, rate, txPower);
    assert(ret == SUBGHZ_OK);

    return ret;
}

static SUBGHZ_MSG LazuriteWireless_end()
{
    SUBGHZ_MSG ret;

    ret = SubGHz.close();
    assert(ret == SUBGHZ_OK);

    return ret;
}

static int LazuriteWireless_listen(Packet *packet)
{
    int ret = 0;
    uint8_t *payload = Payload_getPayloadArray((Payload *)packet);
    short size = LAZURITE_PAYLOAD_SIZE;

    size = SubGHz.readData(payload, LAZURITE_PAYLOAD_SIZE);

    if (size > 0) {
        DEBUG_WRITE(payload, size);
        Payload_resetLength((Payload *)packet, (size_t)size);
    } else {
        ret = -1;
    }

    return ret;
}

static SUBGHZ_MSG LazuriteWireless_send(const Packet * const packet, uint16_t panid, uint16_t dstAddr)
{
    SUBGHZ_MSG ret = 0;
    const uint8_t *data = Payload_getPayloadArray((Payload *)packet);
    size_t size = Payload_getPayloadLength((Payload *)packet);

    ret = SubGHz.send(panid, dstAddr, data, (uint16_t)size, NULL);
    Serial.println_long((long)ret, DEC);
    assert(ret == SUBGHZ_OK);

    return ret;
}

static size_t LazuriteWireless_sendData(uint16_t panid, uint16_t dstAddr, const uint8_t data[], size_t size, bool fragmented)
{
    SUBGHZ_MSG ret;
    Data *idata;

    Packet_initialize(__packet);
    Packet_setType(__packet, DATA);
    
    if (size > LAZURITE_DATA_MAX_SIZE) {
        fragmented = true;
    }

    idata = (Data *)Packet_getInterface(__packet);
    size = idata->setData(__packet, data, size);
    idata->setFragmented(__packet, fragmented);
 
    ret = LazuriteWireless_send(__packet, panid, dstAddr);
    assert(ret == SUBGHZ_OK);
    if (ret != SUBGHZ_OK) {
        size = 0;
    }

    return size;
}

static int LazuriteWireless_sendCommand(uint16_t panid, uint16_t dstAddr, uint8_t cmd, const char param[])
{
    SUBGHZ_MSG ret;
    Command *icommand;

    Packet_initialize(__packet);
    Packet_setType(__packet, COMMAND);

    icommand = (Command *)Packet_getInterface(__packet);
    icommand->setCommand(__packet, cmd);
    icommand->setCommandParam(__packet, param);

    ret = LazuriteWireless_send(__packet, panid, dstAddr);
    assert(ret == SUBGHZ_OK);

    return ret;
}

static int LazuriteWireless_sendCommandWithAck(uint16_t panid, uint16_t dstAddr, uint8_t cmd, const char param[])
{
    SUBGHZ_MSG ret;
    Command *icommand;

    Packet_initialize(__packet);
    Packet_setType(__packet, COMMAND);

    icommand = (Command *)Packet_getInterface(__packet);
    icommand->setCommand(__packet, cmd);
    icommand->setCommandParam(__packet, param);
    icommand->enableAckRequest(__packet);

    ret = LazuriteWireless_send(__packet, panid, dstAddr);
    assert(ret == SUBGHZ_OK);

    return ret;
}

static int LazuriteWireless_sendAck(uint16_t panid, uint16_t dstAddr, uint8_t cmd, const char response[])
{
    SUBGHZ_MSG ret;
    Ack *iack;

    Packet_initialize(__packet);
    Packet_setType(__packet, ACK);

    iack = (Ack *)Packet_getInterface(__packet);
    iack->setCommand(__packet, cmd);
    iack->setResponse(__packet, response);

    ret = LazuriteWireless_send(__packet, panid, dstAddr);
    assert(ret == SUBGHZ_OK);

    return ret;
}

static int LazuriteWireless_sendNotice(uint16_t panid, uint16_t dstAddr, const char notice[])
{
    SUBGHZ_MSG ret;
    Notice *inotice;

    Packet_initialize(__packet);
    Packet_setType(__packet, NOTICE);

    inotice = (Notice *)Packet_getInterface(__packet);
    inotice->setNotice(__packet, notice);

    ret = LazuriteWireless_send(__packet, panid, dstAddr);
    assert(ret == SUBGHZ_OK);

    return ret;
}

static SUBGHZ_MSG LazuriteWireless_enableRx()
{
    SUBGHZ_MSG ret;

    ret = SubGHz.rxEnable(NULL);
    assert(ret == SUBGHZ_OK);

    return ret;
}

static SUBGHZ_MSG LazuriteWireless_disableRx()
{
    SUBGHZ_MSG ret;

    ret = SubGHz.rxDisable();
    assert(ret == SUBGHZ_OK);

    return ret;
}

static uint8_t LazuriteWireless_getAddrType()
{
    SUBGHZ_MSG ret;
    SUBGHZ_PARAM param;

    ret = SubGHz.getSendMode(&param);
    assert(ret == SUBGHZ_OK);
    DEBUG_PRINT("PARAM=");
    DEBUG_WRITE((char *)&param, sizeof(SUBGHZ_PARAM));

    return param.addrType;
}

static uint16_t LazuriteWireless_getMyAddress()
{
    uint16_t address;

    address = SubGHz.getMyAddress();
    DEBUG_PRINT("address=");
    DEBUG_PRINT_LONG(address, HEX);

    return address;
}

static SUBGHZ_MSG LazuriteWireless_setAckReq(bool on)
{
    SUBGHZ_MSG ret;

    ret = SubGHz.setAckReq(on);
    assert(ret == SUBGHZ_OK);

    return ret;
}

static SUBGHZ_MSG LazuriteWireless_setBroadcastEnb(bool on)
{
    SUBGHZ_MSG ret;

    ret = SubGHz.setBroadcastEnb(on);
    assert(ret == SUBGHZ_OK);

    return ret;
}

static SUBGHZ_MSG LazuriteWireless_setPromiscuous(bool on)
{
    SUBGHZ_MSG ret;

    ret = SubGHz.setPromiscuous(on);
    assert(ret == SUBGHZ_OK);

    return ret;
}

static uint8_t LazuriteWireless_setTxRetry()
{
    SUBGHZ_MSG ret;
    SUBGHZ_PARAM param;

    ret = SubGHz.getSendMode(&param);
    assert(ret == SUBGHZ_OK);

    return param.txRetry;
}

static SUBGHZ_MSG LazuriteWireless_setSendMode(uint8_t addrType, uint8_t txRetry)
{
    SUBGHZ_MSG ret;
    SUBGHZ_PARAM param;

    SubGHz.getSendMode(&param);
    param.addrType = addrType;
    param.txRetry = txRetry;
    ret = SubGHz.setSendMode(&param);
    assert(ret == SUBGHZ_OK);

    return ret;
}

static void LazuriteWireless_callback(uint8_t rssi, uint8_t status)
{
    Serial.println("Sending a packet is complete.");
}

static uint8_t* Payload_getBodyArray(Payload * const self)
{
    return &self->_payload[LAZURITE_PACKET_HEADER_SIZE];
}

static size_t Payload_getLength(const Payload * const self)
{
    // return self->_length - LAZURITE_PACKET_HEADER_SIZE;
    return self->_length;
}

static PacketType Payload_getPacketType(const Payload * const self)
{
    return (PacketType)(self->_payload[LAZURITE_PACKET_TYPE_I] & LAZURITE_PACKET_TYPE_MASK);
}

static uint8_t* Payload_getPayloadArray(Payload * const self)
{
    return self->_payload;
}

static size_t Payload_getPayloadLength(Payload * const self)
{
    return self->_length + LAZURITE_PACKET_HEADER_SIZE;
}

static bool Payload_isFragmented(const Payload * const self)
{
    return (self->_payload[LAZURITE_PACKET_FLAG_I] & LAZURITE_PACKET_FLAG_MASK_FRAG) ? true : false;
}

static bool Payload_isResponseRequested(Payload * const self)
{
    return (self->_payload[LAZURITE_PACKET_FLAG_I] & LAZURITE_PACKET_FLAG_MASK_ACK) ? true : false;
}

static void Payload_setFragmented(Payload * const self, bool fragment)
{
    if (fragment)
        self->_payload[LAZURITE_PACKET_FLAG_I] |= LAZURITE_PACKET_FLAG_MASK_FRAG;
    else
        self->_payload[LAZURITE_PACKET_FLAG_I] &= ~LAZURITE_PACKET_FLAG_MASK_FRAG;
}

static void Payload_setResponseRequested(Payload * const self, bool requested)
{
    if (requested)
        self->_payload[LAZURITE_PACKET_FLAG_I] |= LAZURITE_PACKET_FLAG_MASK_ACK;
    else
        self->_payload[LAZURITE_PACKET_FLAG_I] &= ~LAZURITE_PACKET_FLAG_MASK_ACK;
}

static void Payload_setPacketType(Payload * const self, PacketType type)
{
    self->_payload[LAZURITE_PACKET_TYPE_I] &= ~LAZURITE_PACKET_TYPE_MASK;
    self->_payload[LAZURITE_PACKET_TYPE_I] |= (uint8_t)type;
}

// static void Payload_setPayload(Payload * const self, uint8_t from[], size_t length)
// {
//     memcpy(self->_payload, from, length);
// }

// static void Payload_resetPayloadLength(Payload * const self, size_t length)
// {
//     self->_length = length;
// }

static void Payload_resetLength(Payload * const self, size_t length)
{
    // self->_length = length + LAZURITE_PACKET_HEADER_SIZE;
    self->_length = length;
}


Packet * Packet_new()
{
    Payload *instance = (Payload *)malloc(sizeof(Payload));
    Packet_initialize((Packet *)instance);
    return (Packet *)instance;
}

void Packet_free(Packet * instance)
{
    free((void *)instance);
    instance = NULL;
}

PacketType Packet_getType(const Packet * const self)
{
    PacketType type = Payload_getPacketType((Payload *)self);
    DEBUG_PRINT_LONG((long)type, DEC);
    return type;
}

PacketInterfaceBase* Packet_getInterface(const Packet * const self)
{
    static const Ack __ack = {
        {
            Ack_initialize
        },
        Ack_getCommand,
        Ack_getResponse,
        Ack_getResponseLength,
        Ack_setCommand,
        Ack_setResponse
    };
    static const Command __command = {
        {
            Command_initialize,
        },
        Command_enableAckRequest,
        Command_isResponseRequested,
        Command_getCommand,
        Command_getCommandParam,
        Command_getCommandParamLength,
        Command_setCommand,
        Command_setCommandParam,
        Command_setResponseRequested
    };
    static const Data __data = {
        {
            Data_initialize,
        },
        Data_isFragmented,
        Data_getData,
        Data_getDataArray,
        Data_getDataSize,
        Data_setData,
        Data_setFragmented,
        Data_resetDataSize
    };
    static const Notice __notice = {
        {
            Notice_initialize,
        },
        Notice_getNotice,
        Notice_getNoticeLength,
        Notice_setNotice
    };

    PacketType type = Packet_getType(self);
    PacketInterfaceBase *interface = NULL;

    switch(type) {
        case DATA:
            interface = (PacketInterfaceBase *)&__data;
            break;
        case COMMAND:
            interface = (PacketInterfaceBase *)&__command;
            break;
        case ACK:
            interface = (PacketInterfaceBase *)&__ack;
            break;
        case NOTICE:
            interface = (PacketInterfaceBase *)&__notice;
            break;
        default:
            Serial.println("The packet type is unknown type.");
            break;
    }

    return interface;
}

void Packet_initialize(Packet * const self)
{
    Payload * const payload = self;
    payload->_payload[0] = 0;
    payload->_length = 0;
    // memset(payload->_payload, 0, sizeof(uint8_t) * LAZURITE_PAYLOAD_SIZE + 1);
}

void Packet_setType(Packet * const self, PacketType type)
{
    Payload_setPacketType((Payload *)self, type);
}


static uint8_t Ack_getCommand(const Packet * const self)
{
    const uint8_t *body = Payload_getBodyArray((Payload *)self);
    return body[LAZURITE_ACK_CMD_I];
}

static const char* Ack_getResponse(const Packet * const self)
{
    const char *body = (const char *)Payload_getBodyArray((Payload *)self);
    return &body[LAZURITE_ACK_RESPONSE_I];
}

// static char* Ack_getResponseArray(Packet * const self)
// {
//     char *body = (char *)Payload_getBodyArray((Payload *)self);
//     return body[LAZURITE_ACK_PARAM_I];
// }

static size_t Ack_getResponseLength(const Packet * const self)
{
    const char *body = (const char *)Payload_getBodyArray((Payload *)self);
    const char *response = &body[LAZURITE_ACK_RESPONSE_I];
    size_t length = Payload_getLength((Payload *)self) - LAZURITE_ACK_COMMAND_SIZE;

#ifndef NDEBUG
    {
        size_t len;
        len = strlen(response);
        assert(len == length);
    }
#endif

    return length;
}

static void Ack_initialize(Packet * const self)
{
    Packet_initialize(self);
    Packet_setType(self, ACK);
}

static void Ack_setCommand(Packet * const self, uint8_t command)
{
    uint8_t *body = Payload_getBodyArray((Payload *)self);
    body[LAZURITE_ACK_CMD_I] = command;
}

static size_t Ack_setResponse(Packet * const self, const char response[])
{
    char *body = (char *)Payload_getBodyArray((Payload *)self);
    char *dst = &body[LAZURITE_ACK_RESPONSE_I];
    size_t length = strlen(response);

    if (length > LAZURITE_ACK_RESPONSE_MAX_LEN) {
        length = LAZURITE_ACK_RESPONSE_MAX_LEN;
    }
    strncpy(dst, response, length + 1);
    body[LAZURITE_ACK_RESPONSE_I + length] = '\0';
    Payload_resetLength((Payload *)self, LAZURITE_ACK_COMMAND_SIZE + length);

    return length;
}

// static int Ack_resetResponseLength(Packet * const self, size_t length)
// {
//     assert(length <= LAZURITE_ACK_RESPONSE_MAX_LEN);
//     if (length > LAZURITE_ACK_RESPONSE_MAX_LEN) {
//         return -1;
//     }

//     Payload_resetLength((Payload *)self, LAZURITE_ACK_RESPONSE_I + length);
//     return 0;
// }


static void Command_initialize(Packet * const self)
{
    Packet_initialize(self);
    Packet_setType(self, COMMAND);
}

static void Command_enableAckRequest(Packet * const self)
{
    Payload_setResponseRequested((Payload *)self, true);
}

static bool Command_isResponseRequested(const Packet * const self)
{
    return Payload_isResponseRequested((Payload * )self);
}

static uint8_t Command_getCommand(const Packet * const self)
{
    const uint8_t *body = Payload_getBodyArray((Payload *)self);
    return body[LAZURITE_COMMAND_CMD_I];
}

static const char* Command_getCommandParam(const Packet * const self)
{
    const char *body = (const char *)Payload_getBodyArray((Payload *)self);
    return body + LAZURITE_COMMAND_PARAM_I;
}

// static char* Command_getCommandParamArray(Packet * const self)
// {
//     char *body = (char *)Payload_getBodyArray((Payload *)self);
//     return body + LAZURITE_COMMAND_PARAM_I;
// }

static size_t Command_getCommandParamLength(const Packet * const self)
{
    const char *body = (const char *)Payload_getBodyArray((Payload *)self);
    const char *param = &body[LAZURITE_COMMAND_PARAM_I];
    size_t length = Payload_getLength((Payload *)self) - LAZURITE_COMMAND_CMD_SIZE;

#ifndef NDEBUG
    {
        size_t len;
        len = strlen(param);
        assert(len == length);
    }
#endif

    return length;
}

static void Command_setCommand(Packet * const self, uint8_t command)
{
    uint8_t *body = Payload_getBodyArray((Payload *)self);
    body[LAZURITE_COMMAND_CMD_I] = command;
}

static size_t Command_setCommandParam(Packet * const self, const char param[])
{
    char *body = (char *)Payload_getBodyArray((Payload *)self);
    char *dst = &body[LAZURITE_COMMAND_PARAM_I];
    size_t length = strlen(param);

    if (length > LAZURITE_COMMAND_PARAM_MAX_LEN) {
        length = LAZURITE_COMMAND_PARAM_MAX_LEN;
    }
    strncpy(dst, param, length + 1);
    body[LAZURITE_COMMAND_PARAM_I + length] = '\0';
    Payload_resetLength((Payload * )self, LAZURITE_COMMAND_CMD_SIZE + length);

    return length;
}

static void Command_setResponseRequested(Packet * const self, bool requested)
{
    Payload_setResponseRequested((Payload *)self, requested);
}


// static int Command_resetCommandParamLength(Packet * const self, size_t length)
// {
//     assert(length <= LAZURITE_COMMAND_PARAM_MAX_LEN);
//     if (length > LAZURITE_COMMAND_PARAM_MAX_LEN) {
//         return -1;
//     }

//     Payload_resetLength((Payload *)self, LAZURITE_COMMAND_PARAM_I + length);
//     return 0;
// }


static bool Data_isFragmented(const Packet * const self)
{
    return Payload_isFragmented((Payload * )self);
}

static const uint8_t* Data_getData(const Packet * const self)
{
    return Payload_getBodyArray((Payload *)self);
}

static uint8_t* Data_getDataArray(Packet * const self)
{
    return Payload_getBodyArray((Payload *)self);
}

static size_t Data_getDataSize(const Packet * const self)
{
    return Payload_getLength((Payload * )self);
}

static void Data_initialize(Packet * const self)
{
    Packet_initialize(self);
    Packet_setType(self, DATA);
}

static size_t Data_setData(Packet * const self, const uint8_t from[], size_t size)
{
    uint8_t *dst = Payload_getBodyArray((Payload *)self);

    if (size > LAZURITE_DATA_MAX_SIZE) {
        size = LAZURITE_DATA_MAX_SIZE;
    }

    memcpy(dst, from, size);
    Payload_resetLength((Payload * )self, size);

    return size;
}

static void Data_setFragmented(Packet * const self, bool fragmented)
{
    Payload_setFragmented((Payload *)self, fragmented);
}

static int Data_resetDataSize(Packet * const self, size_t size)
{
    assert(!(size > LAZURITE_DATA_MAX_SIZE));
    if (size > LAZURITE_DATA_MAX_SIZE) {
        return -1;
    }
    Payload_resetLength((Payload *)self, size);
    return 0;
}


static const char* Notice_getNotice(const Packet * const self)
{
    return (const char *)Payload_getBodyArray((Payload *)self);
}

// static char* Notice_getNoticeArray(Packet * const self)
// {
//     return (char *)Payload_getBodyArray((Payload *)self);
// }

static size_t Notice_getNoticeLength(const Packet * const self)
{
    return Payload_getLength((Payload * )self);
}

static void Notice_initialize(Packet * const self)
{
    Packet_initialize(self);
    Packet_setType(self, NOTICE);
}

static size_t Notice_setNotice(Packet * const self, const char notice[])
{
    char *body = (char *)Payload_getBodyArray((Payload *)self);
    size_t length = strlen(notice);

    if (length > LAZURITE_NOTICE_MAX_SIZE) {
        length = LAZURITE_NOTICE_MAX_SIZE;
    }

    strncpy(body, notice, length + 1);
    body[length] = '\0';
    Payload_resetLength((Payload *)self, length);

    return length;
}

// static int Notice_resetNoticeLength(Packet * const self, size_t length)
// {
//     if (length > LAZURITE_NOTICE_MAX_SIZE) {
//         return -1;
//     }
//     Payload_resetLength((Payload *)self, length);
//     return 0;
// }
