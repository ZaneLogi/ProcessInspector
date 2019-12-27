#include "stdafx.h"
#include <iostream>
#include "demo_connection.h"
#include "demo_server.h"

// NEED TO DO
// process the messages in the function on_read_bytes()

demo_connection::demo_connection(void* socket, demo_server* server) :
    local_server_connection_win(socket),
    m_server(server)
{
}

demo_connection::~demo_connection(void)
{
    close();
}

void demo_connection::on_read_bytes(const uint8_t* data, int size)
{
    m_data.insert(m_data.end(), data, data + size);
    while (true)
    {
        if (m_data.size() >= sizeof(packet_header))
        {
            // try to extract next command
            packet_header* in_header = (packet_header*)m_data.data();
            if (m_data.size() >= (in_header->payload_size + sizeof(packet_header)))
            {
                process_command(in_header);

                // eat up the bytes consumed
                m_data.erase(m_data.begin(), m_data.begin() + in_header->payload_size + sizeof(packet_header));
            }
            else
            {
                // not enough bytes for a complete packet - maybe next time...
                break;
            }
        }
        else
        {
            // not enough bytes for even a header...
            break;
        }
    }
}

void demo_connection::on_disconnected()
{

}

void demo_connection::process_command(packet_header *header)
{
    switch (header->packet_type) {
    case COMMAND_INIT:
        on_cmd_init((cmd_init_packet*)header);
        break;
    case COMMAND_BYE:
        on_cmd_bye((cmd_bye_packet*)header);
        break;
    default:
        break;
    }
}

void demo_connection::on_cmd_init(cmd_init_packet* cmd)
{
    m_server->notify_main_window(COMMAND_INIT, cmd->process_id);
}

void demo_connection::on_cmd_bye(cmd_bye_packet* cmd)
{
    m_server->notify_main_window(COMMAND_BYE, cmd->process_id);
}