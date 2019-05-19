#ifndef POS_CMD_COMMON_H__
#define POS_CMD_COMMON_H__

#include <stdint.h>
#include "access.h"

/** Vendor specific company ID for PosCmd model */
#define POS_CMD_COMPANY_ID    (ACCESS_COMPANY_ID_NORDIC)

struct position_t
{
    int16_t x;
    int16_t y; 
}  __attribute((packed)); 

/** PosCmd opcodes. */
typedef enum
{
    POS_CMD_OPCODE_SET = 0xC1,            /**< PosCmd Acknowledged Set. */
    POS_CMD_OPCODE_GET = 0xC2,            /**< PosCmd Get. */
    POS_CMD_OPCODE_SET_UNRELIABLE = 0xC3, /**< PosCmd Set Unreliable. */
    POS_CMD_OPCODE_STATUS = 0xC4          /**< PosCmd Status. */
} pos_cmd_opcode_t;

/** Message format for the PosCmd Set message. */
typedef struct __attribute((packed))
{
    uint8_t * com_data; /**< State to set. */
    uint8_t tid;    /**< Transaction number. */
} pos_cmd_msg_set_t;

/** Message format for th PosCmd Set Unreliable message. */
typedef struct __attribute((packed))
{
    uint8_t * com_data; /**< State to set. */
    uint8_t tid;    /**< Transaction number. */
} pos_cmd_msg_set_unreliable_t;

/** Message format for the PosCmd Status message. */
typedef struct __attribute((packed))
{
    uint8_t * present_state; /**< Current state. */
} pos_cmd_msg_status_t;


/** @} end of POS_CMD_COMMON */
/** @} end of POS_CMD_MODEL */
#endif /* POS_CMD_COMMON_H__ */
