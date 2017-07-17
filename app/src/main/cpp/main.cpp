
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <linux/android/binder.h>

#include <dlfcn.h>

#include "elf_hooker.h"

static void* (*__old_impl_ioctl)(int __fd, unsigned long __request, void * arg);
static int (*__old_impl_connect)(int sockfd,struct sockaddr * serv_addr,int addrlen);

extern "C" {

    void bin_to_Hex(unsigned char *pSrc, int nSrcLen, char *pDst, int bIsAscii)
    {
        int i = 0;
        int j = 0;
        char ch;

        if (pDst != NULL)
        {
            pDst[0] = '\0';
        }

        if (pSrc == NULL || nSrcLen <= 0 || pDst == NULL)
        {
            return;
        }

        /* 0x0-0xf */
        const char szTable[] = "0123456789ABCDEF";
        for(i = 0; i<nSrcLen; i++)
        {
            pDst[j++] = '0';
            pDst[j++] = 'x';

            /* high 4 */
            pDst[j++] = szTable[pSrc[i] >> 4];

            /* low 4 */
            pDst[j++] = szTable[pSrc[i] & 0x0f];

            pDst[j++] = ' ';
        }

        if (1 == bIsAscii)
        {
            pDst[j++] = '\n';

            for (i = 0; i < nSrcLen; i++) {
                ch = pSrc[i];
                pDst[j++] = (isascii(ch) && isprint(ch)) ? ch : '.';
            }
        }

        pDst[j] = '\0';

        return;
    }

    void parse_binder(struct binder_transaction_data* pdata, int type)
    {
        char printBuf[4096] = {0};
        bin_to_Hex(pdata->data.buf, pdata->data_size, printBuf, 1);
        log_error("\n%s\n", printBuf);

        return;

        if(type == 1)//BC 系列命令,target.handle指定到需要访问的Binder对象
        {
            log_info("binder_transaction_data----->reply to binder: %x, transaction code : %d\n", pdata->target.handle, pdata->code);

        }
        else //从Binder驱动里读回返回值时，需要指定返回的一段地址，target.ptr则会指向这个地址空间
        {
            log_info("binder_transaction_data----->got from binder: %x, transaction code : %d\n", pdata->target.ptr, pdata->code);

        }
        if(pdata->data_size > 8)//通信数据量比较大，包含很多binder对象。
        {
            int n=0;
            log_info("binder_transaction_data size > 8");
            for (;n<pdata->offsets_size/4; n++)//获取每一个Binder对象
            {
                log_info("data_size: %d, buffer: %x, offsets: %x, offset_size: %d, offset %d:%x!",
                         pdata->data_size, pdata->data.ptr.buffer, pdata->data.ptr.offsets, pdata->offsets_size, n, ((int*)(pdata->data.ptr.offsets))[n]);
                struct flat_binder_object* pbinder = (struct flat_binder_object*)((char*)pdata->data.ptr.buffer + ((int*)(pdata->data.ptr.offsets))[n]);
                log_info("got %d binder object in %x!", n, pbinder);
                continue;

                unsigned long type = pbinder->type;
                switch (type)
                {
                    case BINDER_TYPE_BINDER:
                        log_info("flat_binder_object----->binder:%x, cookie:%x", pbinder->binder, pbinder->cookie);
                        break;
                    default:
                        break;
                }
            }
        }
        else
        {
            log_info("flat_binder_object----->binder data : %x %x\n", pdata->data.buf[0], pdata->data.buf[4]);
        }
        return;
    }

    static void* __nativehook_impl_ioctl(int __fd, unsigned long __request, void * arg)
    {
        log_info("__nativehook_impl_ioctl -> \n");

        log_info("request : %d", __request);
        log_info("BINDER_WRITE_READ : %d", BINDER_WRITE_READ);

        if ( __request == BINDER_WRITE_READ )
        {
            struct binder_write_read* tmp = (struct binder_write_read*) arg;
            signed long write_size = tmp->write_size;
            signed long read_size = tmp->read_size;

            int dir  =  _IOC_DIR(__request);   //根据命令获取传输方向
            int type =  _IOC_TYPE(__request);  //根据命令获取类型
            int nr   =  _IOC_NR(__request);    //根据命令获取类型命令
            int size =  _IOC_SIZE(__request);  //根据命令获取传输数据大小

            log_info("new call to ioctl, dir:%d, type:%d, nr:%d, size:%d\n", dir, type, nr, size);
            if(write_size > 0)//该命令将write_buffer中的数据写入到binder
            {
                log_info("binder_write_read----->write size: %d,write_consumed :%d", tmp->write_size, tmp->write_consumed);
                int already_got_size = 0;
                unsigned long *pcmd = 0;

                log_error("=================write_buffer process start!");
                while(already_got_size < write_size)//循环处理buffer中的每一个命令
                {
                    pcmd = (unsigned long *)(tmp->write_buffer + already_got_size);      //指针后移
                    int code = pcmd[0];
                    log_info("pcmd: %x, already_got_size: %d", pcmd, already_got_size);

                    int dir  =  _IOC_DIR(code);   //根据命令获取传输方向
                    int type =  _IOC_TYPE(code);  //根据命令获取类型
                    int nr   =  _IOC_NR(code);    //根据命令获取类型命令
                    int size =  _IOC_SIZE(code);  //根据命令获取传输数据大小
                    //log_info("cmdcode:%d, dir:%d, type:%c, nr:%d, size:%d\n", code, dir, type, nr, size);

                    struct binder_transaction_data* pdata = (struct binder_transaction_data*)(&pcmd[1]);
                    switch (code)
                    {
                        case BC_TRANSACTION:
                            //log_info("pid: %d, BC_TRANSACTION, dir:%d, type:%c, nr:%d, size:%d\n", pdata->sender_pid, dir, type, nr, size);
                            parse_binder(pdata, 1);
                            break;

                        case BC_REPLY:
                            //log_info("pid: %d, BC_REPLY, dir:%d, type:%c, nr:%d, size:%d\n", pdata->sender_pid, dir, type, nr, size);
                            //parse_binder(pdata, 1);
                            break;

                        default:
                            break;
                    }
                    already_got_size += (size+4);
                }
                log_error("=================write_buffer process end!");
            }
            if(read_size > 0)//从binder中读取数据写入到read_buffer
            {
                void* res = __old_impl_ioctl(__fd, __request, arg);
                return res;

                log_info("binder_write_read----->read size: %d, read_consumed: %d", tmp->read_size, tmp->read_consumed);
                int already_got_size = 0;
                unsigned long *pret = 0;

                log_info("=================read_buffer process start!");
                while(already_got_size < read_size)//循环处理buffer中的每一个命令
                {
                    pret = (unsigned long *)(tmp->read_buffer + already_got_size);       //指针后移
                    unsigned long code = pret[0];
                    //log_info("pret: %x, already_got_size: %d", pret, already_got_size);

                    int dir  =  _IOC_DIR(code);   //根据命令获取传输方向
                    int type =  _IOC_TYPE(code);  //根据命令获取类型
                    int nr   =  _IOC_NR(code);    //根据命令获取类型命令
                    int size =  _IOC_SIZE(code);  //根据命令获取传输数据大小
                    //log_info("retcode:%d, dir:%d, type:%c, nr:%d, size:%d\n", code, dir, type, nr, size);

                    struct binder_transaction_data* pdata = (struct binder_transaction_data*)(&pret[1]);
                    switch (code)
                    {
                        case BR_TRANSACTION:
                            //log_info("pid: %d, BR_TRANSACTION, dir:%d, type:%c, nr:%d, size:%d\n", pdata->sender_pid, dir, type, nr, size);
                            parse_binder(pdata, 2);
                            break;

                        case BR_REPLY:
                            //log_info("pid: %d, BR_REPLY, dir:%d, type:%c, nr:%d, size:%d\n", pdata->sender_pid, dir, type, nr, size);
                            parse_binder(pdata, 2);
                            break;

                        default:
                            break;
                    }
                    already_got_size += (size+4);//数据内容加上命令码
                }
                log_info("=================read_buffer process end!");

            }
        }

        void* res = __old_impl_ioctl(__fd, __request, arg);
        return res;
    }

    static int __nativehook_impl_connect(int sockfd,struct sockaddr * serv_addr,int addrlen)
    {
        //log_info("__nativehook_impl_connect ->\n");

        int res = __old_impl_connect(sockfd, serv_addr, addrlen);
        return res;
    }
}


static bool __prehook(const char* module_name, const char* func_name)
{
    if (strstr(module_name, "libstdc++.so") != NULL)
    {
       return true;
    }
    return true;
}

void __attribute__((constructor)) libhook_main(int argc, char* argv[])
{
    char ch = 0;
    elf_hooker hooker;

    hooker.set_prehook_cb(__prehook);
    hooker.phrase_proc_maps();
    hooker.dump_module_list();

    //hooker.hook_all_modules("connect", (void*)__nativehook_impl_connect, (void**)&__old_impl_connect);
    hooker.hook_all_modules("ioctl", (void*)__nativehook_impl_ioctl, (void**)&__old_impl_ioctl);
}
