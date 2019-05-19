
#include "pos_cmd_client.h"
#include "pos_cmd_common.h"

#include <stdint.h>
#include <stddef.h>

#include "access.h"
#include "access_config.h"
#include "access_reliable.h"
#include "device_state_manager.h"
#include "nrf_mesh.h"
#include "nrf_mesh_assert.h"
#include "log.h"

/*****************************************************************************
 * Static variables
 *****************************************************************************/

/** Keeps a single global TID for all transfers. */
static uint8_t m_tid;

/*****************************************************************************
 * Static functions
 *****************************************************************************/

static void reliable_status_cb(access_model_handle_t model_handle,
                               void * p_args,
                               access_reliable_status_t status)
{
    pos_cmd_client_t * p_client = p_args;
    NRF_MESH_ASSERT(p_client->status_cb != NULL);

    p_client->state.reliable_transfer_active = false;
    switch (status)
    {
        case ACCESS_RELIABLE_TRANSFER_SUCCESS:
            /* Ignore */
            break;
        case ACCESS_RELIABLE_TRANSFER_TIMEOUT:
            p_client->status_cb(p_client, POS_CMD_STATUS_ERROR_NO_REPLY, NRF_MESH_ADDR_UNASSIGNED);
            break;
        case ACCESS_RELIABLE_TRANSFER_CANCELLED:
            p_client->status_cb(p_client, POS_CMD_STATUS_CANCELLED, NRF_MESH_ADDR_UNASSIGNED);
            break;
        default:
            /* Should not be possible. */
            NRF_MESH_ASSERT(false);
            break;
    }
}

static uint32_t send_reliable_message(const pos_cmd_client_t * p_client,
                                      pos_cmd_opcode_t opcode,
                                      const uint8_t * p_data,
                                      uint16_t length)
{
    access_reliable_t reliable;
    reliable.model_handle = p_client->model_handle;
    reliable.message.p_buffer = p_data;
    reliable.message.length = length;
    reliable.message.opcode.opcode = opcode;
    reliable.message.opcode.company_id = POS_CMD_COMPANY_ID;
    reliable.message.force_segmented = false;
    reliable.message.transmic_size = NRF_MESH_TRANSMIC_SIZE_DEFAULT;
    reliable.message.access_token = nrf_mesh_unique_token_get();
    reliable.reply_opcode.opcode = POS_CMD_OPCODE_STATUS;
    reliable.reply_opcode.company_id = POS_CMD_COMPANY_ID;
    reliable.timeout = POS_CMD_CLIENT_ACKED_TRANSACTION_TIMEOUT;
    reliable.status_cb = reliable_status_cb;

    return access_model_reliable_publish(&reliable);
}

/*****************************************************************************
 * Opcode handler callback(s)
 *****************************************************************************/

static void handle_status_cb(access_model_handle_t handle, const access_message_rx_t * p_message, void * p_args)
{
    pos_cmd_client_t * p_client = p_args;
    NRF_MESH_ASSERT(p_client->status_cb != NULL);

    pos_cmd_msg_status_t * p_status = (pos_cmd_msg_status_t *) p_message->p_data;
    pos_cmd_status_t com_data = *p_status->present_state;
    p_client->status_cb(p_client, com_data, p_message->meta_data.src.value);
}

static const access_opcode_handler_t m_opcode_handlers[] =
{
    {{POS_CMD_OPCODE_STATUS, POS_CMD_COMPANY_ID}, handle_status_cb}
};

static void handle_publish_timeout(access_model_handle_t handle, void * p_args)
{
    pos_cmd_client_t * p_client = p_args;

    if (p_client->timeout_cb != NULL)
    {
        p_client->timeout_cb(handle, p_args);
    }
}
/*****************************************************************************
 * Public API
 *****************************************************************************/

uint32_t pos_cmd_client_init(pos_cmd_client_t * p_client, uint16_t element_index)
{
    if (p_client == NULL ||
        p_client->status_cb == NULL)
    {
        return NRF_ERROR_NULL;
    }

    access_model_add_params_t init_params;
    init_params.model_id.model_id = POS_CMD_CLIENT_MODEL_ID;
    init_params.model_id.company_id = POS_CMD_COMPANY_ID;
    init_params.element_index = element_index;
    init_params.p_opcode_handlers = &m_opcode_handlers[0];
    init_params.opcode_count = sizeof(m_opcode_handlers) / sizeof(m_opcode_handlers[0]);
    init_params.p_args = p_client;
    init_params.publish_timeout_cb = handle_publish_timeout;
    return access_model_add(&init_params, &p_client->model_handle);
}

uint32_t pos_cmd_client_set(pos_cmd_client_t * p_client, uint8_t * com_data)
{
    if (p_client == NULL || p_client->status_cb == NULL)
    {
        return NRF_ERROR_NULL;
    }
    else if (p_client->state.reliable_transfer_active)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    p_client->state.data.com_data = com_data;
    p_client->state.data.tid = m_tid++;

    uint32_t status = send_reliable_message(p_client,
                                            POS_CMD_OPCODE_SET,
                                            (const uint8_t *)&p_client->state.data,
                                            sizeof(pos_cmd_msg_set_t));
    if (status == NRF_SUCCESS)
    {
        p_client->state.reliable_transfer_active = true;
    }
    return status;

}

uint32_t pos_cmd_client_set_unreliable(pos_cmd_client_t * p_client, uint8_t * com_data, uint16_t len, uint8_t repeats)
{

    access_message_tx_t message;
    message.opcode.opcode = POS_CMD_OPCODE_SET_UNRELIABLE;
    message.opcode.company_id = POS_CMD_COMPANY_ID;
    message.p_buffer = com_data;
    message.length = len;
    message.force_segmented = false;
    message.transmic_size = NRF_MESH_TRANSMIC_SIZE_DEFAULT;

    p_client->state.data.com_data = com_data;
    p_client->state.data.tid = m_tid++;

    uint32_t status = NRF_SUCCESS;
    for (uint8_t i = 0; i < repeats; ++i)
    {
        message.access_token = nrf_mesh_unique_token_get();
        status = access_model_publish(p_client->model_handle, &message);
        if (status != NRF_SUCCESS)
        {
            break;
        }
    }
    
    __LOG_XB(LOG_SRC_APP, LOG_LEVEL_INFO, "Raw", message.p_buffer, len);

    return status;
}

uint32_t pos_cmd_client_get(pos_cmd_client_t * p_client)
{
    if (p_client == NULL || p_client->status_cb == NULL)
    {
        return NRF_ERROR_NULL;
    }
    else if (p_client->state.reliable_transfer_active)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    uint32_t status = send_reliable_message(p_client,
                                            POS_CMD_OPCODE_GET,
                                            NULL,
                                            0);
    if (status == NRF_SUCCESS)
    {
        p_client->state.reliable_transfer_active = true;
    }
    return status;
}

/**
 * Cancel any ongoing reliable message transfer.
 *
 * @param[in] p_client Pointer to the client instance structure.
 */
void pos_cmd_client_pending_msg_cancel(pos_cmd_client_t * p_client)
{
    (void)access_model_reliable_cancel(p_client->model_handle);
}
