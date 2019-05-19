/************************************************************************/
// File:            pos_cmd_server.c
// Author:          Kristian Fjelde Pedersen, NTNU Spring 2019
//                  Credit is given where credit is due.
// Purpose: 
//                  pos_cmd_server is a minimalistic communication server
//                  model. It is fully embedded into BLE Mesh and sends
//                  an array of uint8_t by any desires.
//
/***********************************************************************/

#include "pos_cmd_server.h"
#include "pos_cmd_common.h"

#include <stdint.h>
#include <stddef.h>

#include "access.h"
#include "nrf_mesh_assert.h"
#include "log.h"

/*****************************************************************************
 * Static functions
 *****************************************************************************/

static void reply_status(const pos_cmd_server_t * p_server,
                         const access_message_rx_t * p_message,
                         uint8_t * present_state)
{
    pos_cmd_msg_status_t status;
    status.present_state = present_state;
    access_message_tx_t reply;
    reply.opcode.opcode = POS_CMD_OPCODE_STATUS;
    reply.opcode.company_id = POS_CMD_COMPANY_ID;
    reply.p_buffer = (const uint8_t *) &status;
    reply.length = sizeof(status);
    reply.force_segmented = false;
    reply.transmic_size = NRF_MESH_TRANSMIC_SIZE_DEFAULT;
    reply.access_token = nrf_mesh_unique_token_get();

    (void) access_model_reply(p_server->model_handle, p_message, &reply);
}

/*****************************************************************************
 * Opcode handler callbacks
 *****************************************************************************/

static void handle_set_cb(access_model_handle_t handle, const access_message_rx_t * p_message, void * p_args)
{
    pos_cmd_server_t * p_server = p_args;
    NRF_MESH_ASSERT(p_server->set_cb != NULL);

    uint8_t * value = ((pos_cmd_msg_set_t*) &p_message->p_data)->com_data;
    uint16_t len = p_message->length;
    value = p_server->set_cb(p_server, value, len);
    reply_status(p_server, p_message, value);
    (void) pos_cmd_server_status_publish(p_server, value); /* We don't care about status */
}

static void handle_get_cb(access_model_handle_t handle, const access_message_rx_t * p_message, void * p_args)
{
    pos_cmd_server_t * p_server = p_args;
    NRF_MESH_ASSERT(p_server->get_cb != NULL);
    reply_status(p_server, p_message, p_server->get_cb(p_server));
}

static void handle_set_unreliable_cb(access_model_handle_t handle, const access_message_rx_t * p_message, void * p_args)
{
    pos_cmd_server_t * p_server = p_args;
    NRF_MESH_ASSERT(p_server->set_cb != NULL);

    uint8_t * data = p_message->p_data;
    uint16_t len = p_message->length;
    p_server->set_cb(p_server, data, len);
    /* Don't care at the moment */
    //(void) pos_cmd_server_status_publish(p_server, value);
}

static const access_opcode_handler_t m_opcode_handlers[] =
{
    {ACCESS_OPCODE_VENDOR(POS_CMD_OPCODE_SET,            POS_CMD_COMPANY_ID), handle_set_cb},
    {ACCESS_OPCODE_VENDOR(POS_CMD_OPCODE_GET,            POS_CMD_COMPANY_ID), handle_get_cb},
    {ACCESS_OPCODE_VENDOR(POS_CMD_OPCODE_SET_UNRELIABLE, POS_CMD_COMPANY_ID), handle_set_unreliable_cb}
};

/*****************************************************************************
 * Public API
 *****************************************************************************/

uint32_t pos_cmd_server_init(pos_cmd_server_t * p_server, uint16_t element_index)
{

    uint32_t status;

    if (p_server == NULL ||
        p_server->get_cb == NULL ||
        p_server->set_cb == NULL)
    {
        return NRF_ERROR_NULL;
    }

    access_model_add_params_t init_params;
    init_params.element_index =  element_index;
    init_params.model_id.model_id = POS_CMD_SERVER_MODEL_ID;
    init_params.model_id.company_id = POS_CMD_COMPANY_ID;
    init_params.p_opcode_handlers = &m_opcode_handlers[0];
    init_params.opcode_count = sizeof(m_opcode_handlers) / sizeof(m_opcode_handlers[0]);
    init_params.p_args = p_server;
    init_params.publish_timeout_cb = NULL;
    status = access_model_add(&init_params, &p_server->model_handle);

    if (status == NRF_SUCCESS)
    {
        status = access_model_subscription_list_alloc(p_server->model_handle);
    }

    return status;

}

uint32_t pos_cmd_server_status_publish(pos_cmd_server_t * p_server, uint8_t * value)
{
    pos_cmd_msg_status_t status;
    status.present_state = value;
    access_message_tx_t msg;
    msg.opcode.opcode = POS_CMD_OPCODE_STATUS;
    msg.opcode.company_id = POS_CMD_COMPANY_ID;
    msg.p_buffer = (const uint8_t *) &status;
    msg.length = sizeof(status);
    msg.force_segmented = false;
    msg.transmic_size = NRF_MESH_TRANSMIC_SIZE_DEFAULT;
    msg.access_token = nrf_mesh_unique_token_get();
    return access_model_publish(p_server->model_handle, &msg);
}
