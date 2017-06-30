
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <dlfcn.h>

#include "elf_hooker.h"

static void* (*__old_impl_dlopen)(const char* filename, int flag);

static int (*__old_impl_connect)(int sockfd,struct sockaddr * serv_addr,int addrlen);

static void* (*__old_impl_android_dlopen_ext)(const char* filename, int flags, const void* extinfo);

extern "C" {

    static void* __nativehook_impl_dlopen(const char* filename, int flag)
    {
        log_info("__nativehook_impl_dlopen -> (%s)\n", filename);
        void* res = __old_impl_dlopen(filename, flag);
        return res;
    }

    static int __nativehook_impl_connect(int sockfd,struct sockaddr * serv_addr,int addrlen)
    {
        log_info("__nativehook_impl_connect ->\n");
        int res = __old_impl_connect(sockfd, serv_addr, addrlen);
        return res;
    }

    static void* __nativehook_impl_android_dlopen_ext(const char* filename, int flags, const void* extinfo)
    {
        log_info("__nativehook_impl_android_dlopen_ext -> (%s)\n", filename);
        void* res = __old_impl_android_dlopen_ext(filename, flags, extinfo);
        return res;
    }

}

static bool __prehook(const char* module_name, const char* func_name)
{
    if (strstr(module_name, "libwebviewchromium_loader.so") != NULL)
    {
       return true;
    }
    return false;
}

void __attribute__((constructor)) libhook_main(int argc, char* argv[])
{
    char ch = 0;
    elf_hooker hooker;

    hooker.set_prehook_cb(__prehook);
    hooker.phrase_proc_maps();
    hooker.dump_module_list();
    //hooker.hook_all_modules("dlopen", (void*)__nativehook_impl_dlopen, (void**)&__old_impl_dlopen);
    //hooker.hook_all_modules("connect", (void*)__nativehook_impl_connect, (void**)&__old_impl_connect);
    hooker.hook_all_modules("android_dlopen_ext", (void*)__nativehook_impl_android_dlopen_ext, (void**)&__old_impl_android_dlopen_ext);

}
