#pragma once

#if defined(_MSC_VER)
#include <windows.h>
#else
#include <cstdlib>
#include <cwchar>
#include <vector>
#include <dlfcn.h>
#endif

struct Module
{
#if defined(_MSC_VER)
    typedef HMODULE platform_module_type;
    
    platform_module_type load_impl(const char *path) { return LoadLibraryA(path); }
    platform_module_type load_impl(const wchar_t *path) { return LoadLibraryW(path); }
    void unload_impl(platform_module_type handle) { assert(handle); FreeLibrary(handle); }
#else
    typedef    void *    platform_module_type;
    
    platform_module_type load_impl(const char *path) { return dlopen(path, RTLD_LAZY); }
    platform_module_type load_impl(const wchar_t *path)
    {
        auto const error_result = static_cast<std::size_t>(-1);
        
        auto const num_estimated_bytes = std::wcstombs(NULL, path, 0);
        if (error_result == num_estimated_bytes) {
            return NULL;
        }
        auto const buffer_length = (num_estimated_bytes / sizeof(char)) + 1;
        std::vector<char> tmp(buffer_length);
        std::mbstate_t state = std::mbstate_t();
        auto const num_converted_bytes = std::wcsrtombs(tmp.data(),
                                                        &path,
                                                        buffer_length * sizeof(char),
                                                        &state);
        if (error_result == num_converted_bytes) {
            return NULL;
        }
        
        return load(tmp.data());
    }
    
    platform_module_type load(const char *path)
    {
        return dlopen(path, RTLD_LAZY);
    }
    
    void unload_impl(platform_module_type handle) { assert(handle); dlclose(handle); }
#endif
    
public:
    typedef platform_module_type    module_type;
    
    Module() {}
    
    explicit
    Module(char const *path)
    :    module_(load_impl(path))
    {}
    
    explicit
    Module(wchar_t const *path)
    :    module_(load_impl(path))
    {}
    
    Module(module_type module)
    :    module_(module)
    {}
    
    Module(Module &&r)
    :    module_(std::exchange(r.module_, platform_module_type()))
    {
        r.module_ = nullptr;
    }
    
    Module & operator=(Module &&r)
    {
        module_ = std::exchange(r.module_, platform_module_type());
        return *this;
    }
    
    void * get_proc_address(char const *function_name) const
    {
#if defined(_MSC_VER)
        return GetProcAddress(module_, function_name);
#else
        return dlsym(module_, function_name);
#endif
    }
    
    ~Module()
    {
        if(module_) { unload_impl(module_); }
    }
    
public:
    module_type get() { return module_; }
    module_type get() const { return module_; }
    module_type release() { return std::exchange(module_, platform_module_type()); }
    
    template<class... Args>
    void reset(Args&&... args) { *this = Module(args...); }
    
    explicit
    operator bool() const { return module_ != platform_module_type(); }
    
private:
    module_type    module_ = platform_module_type();
};
