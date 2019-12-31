#pragma once

class d3d11_method_table
{
private:
    d3d11_method_table() = default;
    d3d11_method_table(const d3d11_method_table*) = delete;
    ~d3d11_method_table();

public:
    static d3d11_method_table* instance();

    bool init();
    void deinit();

    bool bind(uint16_t _index, void** _original, void* _function);
    void unbind(uint16_t index);

    DWORD_PTR* operator[] (int index);
    DWORD_PTR* swapchain_present1();

private:
    DWORD_PTR* m_methodsTable = nullptr;
    DWORD_PTR* m_swapchain_present1 = nullptr;
};
