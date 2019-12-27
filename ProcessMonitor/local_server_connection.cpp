#include "stdafx.h"
#include "local_server_connection.h"

local_server_connection::local_server_connection(void* socket, int readBufferSize) : m_read_buffer(readBufferSize)
{
}

local_server_connection::~local_server_connection()
{
    close();
}

void local_server_connection::on_read_bytes(const uint8_t*, int)
{
}

bool local_server_connection::write(const char*, int)
{
    return false;
}

void local_server_connection::close(void)
{
}

void local_server_connection::on_disconnected(void)
{
}

void* local_server_connection::socket()
{
    return nullptr;
}
