

#ifndef POS_CMD_CLIENT_H__
#define POS_CMD_CLIENT_H__

#include <stdint.h>
#include "access.h"
#include "pos_cmd_common.h"

/**
 * @defgroup POS_CMD_CLIENT PosCmd Client
 * @ingroup POS_CMD_MODEL
 * This module implements a vendor specific PosCmd Client.
 *
 * @{
 */

/** Acknowledged message transaction timeout */
#ifndef POS_CMD_CLIENT_ACKED_TRANSACTION_TIMEOUT
#define POS_CMD_CLIENT_ACKED_TRANSACTION_TIMEOUT  (SEC_TO_US(60))
#endif

/** PosCmd Client model ID. */
#define POS_CMD_CLIENT_MODEL_ID (0x0004)

/** PosCmd status codes. */
typedef enum
{
    /** Received status OFF from the server. */
    POS_CMD_STATUS_OFF,
    /** Received status ON from the server. */
    POS_CMD_STATUS_ON,
    /** The server did not reply to a PosCmd Set/Get. */
    POS_CMD_STATUS_ERROR_NO_REPLY,
    /** PosCmd Set/Get was cancelled. */
    POS_CMD_STATUS_CANCELLED
} pos_cmd_status_t;

/** Forward declaration. */
typedef struct __pos_cmd_client pos_cmd_client_t;

/**
 * PosCmd status callback type.
 *
 * @param[in] p_self Pointer to the PosCmd client structure that received the status.
 * @param[in] status The received status of the remote server.
 * @param[in] src    Element address of the remote server.
 */
typedef void (*pos_cmd_status_cb_t)(const pos_cmd_client_t * p_self, pos_cmd_status_t status, uint16_t src);
/**
 * PosCmd timeout callback type.
 *
 * @param[in] handle Model handle
 * @param[in] p_self Pointer to the PosCmd client structure that received the status.
 */
typedef void (*pos_cmd_timeout_cb_t)(access_model_handle_t handle, void * p_self);

/** PosCmd Client state structure. */
struct __pos_cmd_client
{
    /** Model handle assigned to the client. */
    access_model_handle_t model_handle;
    /** Status callback called after status received from server. */
    pos_cmd_status_cb_t status_cb;
    /** Timeout callback called after acknowledged message sending times out */
    pos_cmd_timeout_cb_t timeout_cb;
    /** Internal client state. */
    struct
    {
        bool reliable_transfer_active; /**< Variable used to determine if a transfer is currently active. */
        pos_cmd_msg_set_t data;  /**< Variable reflecting the data stored in the server. */
    } state;
};

/**
 * Initializes the PosCmd client.
 *
 * @note This function should only be called _once_.
 * @note The client handles the model allocation and adding.
 *
 * @param[in,out] p_client      PosCmd Client structure pointer.
 * @param[in]     element_index Element index to add the server model.
 *
 * @retval NRF_SUCCESS         Successfully added client.
 * @retval NRF_ERROR_NULL      NULL pointer supplied to function.
 * @retval NRF_ERROR_NO_MEM    No more memory available to allocate model.
 * @retval NRF_ERROR_FORBIDDEN Multiple model instances per element is not allowed.
 * @retval NRF_ERROR_NOT_FOUND Invalid element index.
 */
uint32_t pos_cmd_client_init(pos_cmd_client_t * p_client, uint16_t element_index);

/**
 * Sets the state of the PosCmd server.
 *
 * @param[in,out] p_client PosCmd Client structure pointer.
 * @param[in]     on_off   Value to set the PosCmd Server state to.
 *
 * @retval NRF_SUCCESS              Successfully sent message.
 * @retval NRF_ERROR_NULL           NULL pointer in function arguments
 * @retval NRF_ERROR_NO_MEM         Not enough memory available for message.
 * @retval NRF_ERROR_NOT_FOUND      Invalid model handle or model not bound to element.
 * @retval NRF_ERROR_INVALID_ADDR   The element index is greater than the number of local unicast
 *                                  addresses stored by the @ref DEVICE_STATE_MANAGER.
 * @retval NRF_ERROR_INVALID_STATE  Message already scheduled for a reliable transfer.
 * @retval NRF_ERROR_INVALID_PARAM  Model not bound to appkey, publish address not set or wrong
 *                                  opcode format.
 */
uint32_t pos_cmd_client_set(pos_cmd_client_t * p_client, uint8_t * on_off);

/**
 * Sets the state of the PosCmd Server unreliably (without acknowledgment).
 *
 * @param[in,out] p_client PosCmd Client structure pointer.
 * @param[in]     on_off   Value to set the PosCmd Server state to.
 * @param[in]     repeats  Number of messages to send in a single burst. Increasing the number may
 *                     increase probability of successful delivery.
 *
 * @retval NRF_SUCCESS              Successfully sent message.
 * @retval NRF_ERROR_NULL           NULL pointer in function arguments
 * @retval NRF_ERROR_NO_MEM         Not enough memory available for message.
 * @retval NRF_ERROR_NOT_FOUND      Invalid model handle or model not bound to element.
 * @retval NRF_ERROR_INVALID_ADDR   The element index is greater than the number of local unicast
 *                                  addresses stored by the @ref DEVICE_STATE_MANAGER.
 * @retval NRF_ERROR_INVALID_PARAM  Model not bound to appkey, publish address not set or wrong
 *                                  opcode format.
 */
uint32_t pos_cmd_client_set_unreliable(pos_cmd_client_t * p_client, uint8_t * com_data, uint16_t len, uint8_t repeats);

/**
 * Gets the state of the PosCmd server.
 *
 * @note The state of the server will be given in the @ref pos_cmd_status_cb_t callback.
 *
 * @param[in,out] p_client PosCmd Client structure pointer.
 *
 * @retval NRF_SUCCESS              Successfully sent message.
 * @retval NRF_ERROR_NULL           NULL pointer in function arguments
 * @retval NRF_ERROR_NO_MEM         Not enough memory available for message.
 * @retval NRF_ERROR_NOT_FOUND      Invalid model handle or model not bound to element.
 * @retval NRF_ERROR_INVALID_ADDR   The element index is greater than the number of local unicast
 *                                  addresses stored by the @ref DEVICE_STATE_MANAGER.
 * @retval NRF_ERROR_INVALID_STATE  Message already scheduled for a reliable transfer.
 * @retval NRF_ERROR_INVALID_PARAM  Model not bound to appkey, publish address not set or wrong
 *                                  opcode format.
 */
uint32_t pos_cmd_client_get(pos_cmd_client_t * p_client);

/**
 * Cancel any ongoing reliable message transfer.
 *
 * @param[in,out] p_client Pointer to the client instance structure.
 */
void pos_cmd_client_pending_msg_cancel(pos_cmd_client_t * p_client);



/** @} end of POS_CMD_CLIENT */

#endif /* POS_CMD_CLIENT_H__ */
