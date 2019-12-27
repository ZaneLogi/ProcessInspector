#pragma once
#include <vector>

class local_server_connection
{
public:
    local_server_connection(void* socket, int readBufferSize = 4096);
    virtual ~local_server_connection();

    virtual bool write(const char*data, int size); // synchronous (for now)
    virtual void* socket();
    virtual void close();

protected:
    virtual void on_read_bytes(const uint8_t* data, int size);
    virtual void on_disconnected();

protected:
    std::vector<uint8_t> m_read_buffer;
};
