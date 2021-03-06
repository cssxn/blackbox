/* **********************************************************
 * Copyright (c) 2011-2013 Google, Inc.  All rights reserved.
 * Copyright (c) 2008-2010 VMware, Inc.  All rights reserved.
 * **********************************************************/

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * 
 * * Neither the name of VMware, Inc. nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL VMWARE, INC. OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

/*
 * tls.h - kernel tls support shared among os-specific files, but not
 * exported to the rest of the code.
 *
 * XXX: originally I was going to have this just be kernel tls support
 * and leave os_local_state_t inside os.c, but it was a pain to refactor
 * os_local_state_t access out of the routines here.  We should either
 * go ahead and do that, or pull all the os_local_state_t setup into here?
 */

#ifndef _OS_TLS_H_
#define _OS_TLS_H_ 1

#include "os_private.h"  /* ASM_XAX */

/* We support 3 different methods of creating a segment (see os_tls_init()) */
typedef enum {
    TLS_TYPE_NONE,
    TLS_TYPE_LDT,
    TLS_TYPE_GDT,
#ifdef X64
    TLS_TYPE_ARCH_PRCTL,
#endif
} tls_type_t;

extern tls_type_t tls_global_type;

/* XXX: more cleanly separate the code so we don't need this here */
extern bool return_stolen_lib_tls_gdt;

#define GDT_NO_SIZE_LIMIT  0xfffff

/* The ldt struct in Linux asm/ldt.h used to be just "struct modify_ldt_ldt_s"; then that
 * was also typdef-ed as modify_ldt_t; then it was just user_desc.
 * To compile on old and new we inline our own copy of the struct.
 * We also use this as a cross-platform representation.
 */
typedef struct _our_modify_ldt_t {
    unsigned int  entry_number;
    unsigned int  base_addr;
    unsigned int  limit;
    unsigned int  seg_32bit:1;
    unsigned int  contents:2;
    unsigned int  read_exec_only:1;
    unsigned int  limit_in_pages:1;
    unsigned int  seg_not_present:1;
    unsigned int  useable:1;
} our_modify_ldt_t;

/* segment selector format:
 * 15..............3      2          1..0
 *        index      0=GDT,1=LDT  Requested Privilege Level
 */
#define USER_PRIVILEGE  3
#define LDT_NOT_GDT     1
#define GDT_NOT_LDT     0
#define SELECTOR_IS_LDT  0x4
#define LDT_SELECTOR(idx) ((idx) << 3 | ((LDT_NOT_GDT) << 2) | (USER_PRIVILEGE))
#define GDT_SELECTOR(idx) ((idx) << 3 | ((GDT_NOT_LDT) << 2) | (USER_PRIVILEGE))
#define SELECTOR_INDEX(sel) ((sel) >> 3)

#define WRITE_DR_SEG(val) \
    ASSERT(sizeof(val) == sizeof(reg_t));                           \
    asm volatile("mov %0,%%"ASM_XAX"; mov %%"ASM_XAX", %"ASM_SEG";" \
                 : : "m" ((val)) : ASM_XAX);

#define WRITE_LIB_SEG(val) \
    ASSERT(sizeof(val) == sizeof(reg_t));                               \
    asm volatile("mov %0,%%"ASM_XAX"; mov %%"ASM_XAX", %"LIB_ASM_SEG";" \
                 : : "m" ((val)) : ASM_XAX);

static inline uint
read_selector(reg_id_t seg)
{
    uint sel;
    if (seg == SEG_FS) {
        asm volatile("movl %%fs, %0" : "=r"(sel));
    } else if (seg == SEG_GS) {
        asm volatile("movl %%gs, %0" : "=r"(sel));
    } else {
        ASSERT_NOT_REACHED();
        return 0;
    }
    /* Pre-P6 family leaves upper 2 bytes undefined, so we clear them.  We don't
     * clear and then use movw because that takes an extra clock cycle, and gcc
     * can optimize this "and" into "test %?x, %?x" for calls from
     * is_segment_register_initialized().
     */
    sel &= 0xffff;
    return sel;
}

#if defined(LINUX) && defined(X64) && !defined(ARCH_SET_GS)
#  define ARCH_SET_GS 0x1001
#  define ARCH_SET_FS 0x1002
#  define ARCH_GET_FS 0x1003
#  define ARCH_GET_GS 0x1004
#endif

#ifdef LINUX
#  define GDT_NUM_TLS_SLOTS 3
#elif defined(MACOS)
/* XXX: rename to APP_SAVED_TLS_SLOTS or sthg? */
#  define GDT_NUM_TLS_SLOTS 2
#endif

#define MAX_NUM_CLIENT_TLS 64

/* i#107: handle segment reg usage conflicts */
typedef struct _os_seg_info_t {
    int   tls_type;
    void *dr_fs_base;
    void *dr_gs_base;
    our_modify_ldt_t app_thread_areas[GDT_NUM_TLS_SLOTS];
} os_seg_info_t;

/* layout of our TLS */
typedef struct _os_local_state_t {
    /* put state first to ensure that it is cache-line-aligned */
    /* On Linux, we always use the extended structure. */
    local_state_extended_t state;
    /* linear address of tls page */
    struct _os_local_state_t *self;
    /* store what type of TLS this is so we can clean up properly */
    tls_type_t tls_type;
    /* For pre-SYS_set_thread_area kernels (pre-2.5.32, pre-NPTL), each
     * thread needs its own ldt entry */
    int ldt_index;
    /* tid needed to ensure children are set up properly */
    thread_id_t tid;
    /* i#107 application's gs/fs value and pointed-at base */
    ushort app_gs;      /* for mangling seg update/query */
    ushort app_fs;      /* for mangling seg update/query */
    void  *app_gs_base; /* for mangling segmented memory ref */
    void  *app_fs_base; /* for mangling segmented memory ref */
    union {
        /* i#107: We use space in os_tls to store thread area information
         * thread init. It will not conflict with the client_tls usage,
         * so we put them into a union for saving space. 
         */
        os_seg_info_t os_seg_info;
        void *client_tls[MAX_NUM_CLIENT_TLS];
    };
} os_local_state_t;

os_local_state_t *
get_os_tls(void);

void
tls_thread_init(os_local_state_t *os_tls, byte *segment);

void
tls_thread_free(tls_type_t tls_type, int index);

/* Assumes it's passed either SEG_FS or SEG_GS.
 * Returns POINTER_MAX on failure.
 */
byte *
tls_get_fs_gs_segment_base(uint seg);

/* Assumes it's passed either SEG_FS or SEG_GS.
 * Sets only the base: does not change the segment selector register.
 */
bool
tls_set_fs_gs_segment_base(tls_type_t tls_type, uint seg,
                           /* For x64 and TLS_TYPE_ARCH_PRCTL, base is used:
                            * else, desc is used.
                            */
                           byte *base, our_modify_ldt_t *desc);

void
tls_init_descriptor(our_modify_ldt_t *desc OUT, void *base, size_t size, uint index);

bool
tls_get_descriptor(int index, our_modify_ldt_t *desc OUT);

bool
tls_clear_descriptor(int index);

int
tls_dr_index(void);

int
tls_priv_lib_index(void);

bool
tls_dr_using_msr(void);

void
tls_initialize_indices(os_local_state_t *os_tls);

int
tls_min_index(void);

#if defined(LINUX) && defined(X64)
void
tls_handle_post_arch_prctl(dcontext_t *dcontext, int code, reg_t base);
#endif

#endif /* _OS_TLS_H_ */
