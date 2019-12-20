#pragma once

class d3d9_method_table
{
private:
    d3d9_method_table() = default;
    d3d9_method_table(const d3d9_method_table*) = delete;
    ~d3d9_method_table();

public:
    static d3d9_method_table* instance();

    bool init();
    void deinit();

    bool bind(uint16_t _index, void** _original, void* _function);
    void unbind(uint16_t index);

    DWORD_PTR* operator[] (int index);

private:
    DWORD_PTR* m_methodsTable = nullptr;
};
