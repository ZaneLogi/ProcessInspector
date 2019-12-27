#pragma once

#include <memory>
#include "local_server_connection_win.h"

#pragma pack(1)

enum packet_type
{
    COMMAND_INIT,
    COMMAND_BYE
};

typedef struct packet_header
{
    uint32_t packet_type;
    uint32_t payload_size;
} packet_header;

typedef struct text_packet
{
    packet_header hdr;
    wchar_t message[1];
} text_packet;

typedef struct cmd_init_packet
{
    packet_header hdr;
    uint32_t process_id;
} cmd_init_packet;

typedef struct cmd_bye_packet
{
    packet_header hdr;
    uint32_t process_id;
} cmd_bye_packet;

#pragma pack()

class demo_server;

class demo_connection : public local_server_connection_win
{
public:
    demo_connection(void* socket, demo_server* server);
    virtual ~demo_connection();

protected:
    virtual void on_read_bytes(const uint8_t* data, int size) override;
    virtual void on_disconnected() override;

    void process_command(packet_header *header);
    void on_cmd_init(cmd_init_packet* cmd);
    void on_cmd_bye(cmd_bye_packet* cmd);

private:
    demo_server* m_server;
    std::vector<uint8_t> m_data;

};

using demo_connection_ptr = std::shared_ptr<demo_connection>;
