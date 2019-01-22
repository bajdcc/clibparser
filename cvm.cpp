//
// Project: CMiniLang
// Author: bajdcc
//

#include <cassert>
#include <memory.h>
#include <cstring>
#include "cvm.h"
#include "cgen.h"
#include "cexception.h"
#include "cgui.h"

#define LOG_INS 0
#define LOG_STACK 0
#define LOG_SYSTEM 1

int g_argc;
char **g_argv;

namespace clib {

#define INC_PTR 4
#define VMM_ARG(s, p) ((s) + p * INC_PTR)
#define VMM_ARGS(t, n) vmm_get(t - (n) * INC_PTR)

    uint32_t cvm::pmm_alloc() {
        auto ptr = (uint32_t) memory.alloc_array<byte>(PAGE_SIZE * 2);
        ctx->allocation.push_back(ptr);
        auto page = PAGE_ALIGN_UP(ptr);
        memset((void *) page, 0, PAGE_SIZE);
        return page;
    }

    void cvm::vmm_init() {
        pgd_kern = (pde_t *) malloc(PTE_SIZE * sizeof(pde_t));
        memset(pgd_kern, 0, PTE_SIZE * sizeof(pde_t));
        pte_kern = (pte_t *) malloc(PTE_COUNT * PTE_SIZE * sizeof(pte_t));
        memset(pte_kern, 0, PTE_COUNT * PTE_SIZE * sizeof(pte_t));
        pgdir = pgd_kern;

        uint32_t i;

        // map 4G memory, physcial address = virtual address
        for (i = 0; i < PTE_SIZE; i++) {
            pgd_kern[i] = (uint32_t) pte_kern[i] | PTE_P | PTE_R | PTE_K;
        }

        uint32_t *pte = (uint32_t *) pte_kern;
        for (i = 1; i < PTE_COUNT * PTE_SIZE; i++) {
            pte[i] = (i << 12) | PTE_P | PTE_R | PTE_K; // i是页表号
        }

        for (i = 0; i < TASK_NUM; ++i) {
            tasks[i].flag = 0;
        }
    }

    // 虚页映射
    // va = 虚拟地址  pa = 物理地址
    void cvm::vmm_map(uint32_t va, uint32_t pa, uint32_t flags) {
        uint32_t pde_idx = PDE_INDEX(va); // 页目录号
        uint32_t pte_idx = PTE_INDEX(va); // 页表号

        pte_t *pte = (pte_t *) (pgdir[pde_idx] & PAGE_MASK); // 页表

        if (!pte) { // 缺页
            if (va >= USER_BASE) { // 若是用户地址则转换
                pte = (pte_t *) pmm_alloc(); // 申请物理页框，用作新页表
                pgdir[pde_idx] = (uint32_t) pte | PTE_P | flags; // 设置页表
                pte[pte_idx] = (pa & PAGE_MASK) | PTE_P | flags; // 设置页表项
            } else { // 内核地址不转换
                pte = (pte_t *) (pgd_kern[pde_idx] & PAGE_MASK); // 取得内核页表
                pgdir[pde_idx] = (uint32_t) pte | PTE_P | flags; // 设置页表
            }
        } else { // pte存在
            pte[pte_idx] = (pa & PAGE_MASK) | PTE_P | flags; // 设置页表项
        }

#if 0
        printf("MEMMAP> V=%08X P=%08X\n", va, pa);
#endif
    }

    // 释放虚页
    void cvm::vmm_unmap(uint32_t va) {
        uint32_t pde_idx = PDE_INDEX(va);
        uint32_t pte_idx = PTE_INDEX(va);

        pte_t *pte = (pte_t *) (pgdir[pde_idx] & PAGE_MASK);

        if (!pte) {
            return;
        }

        pte[pte_idx] = 0; // 清空页表项，此时有效位为零
    }

    // 是否已分页
    int cvm::vmm_ismap(uint32_t va, uint32_t *pa) const {
        uint32_t pde_idx = PDE_INDEX(va);
        uint32_t pte_idx = PTE_INDEX(va);

        pte_t *pte = (pte_t *) (pgdir[pde_idx] & PAGE_MASK);
        if (!pte) {
            return 0; // 页表不存在
        }
        if (pte[pte_idx] != 0 && (pte[pte_idx] & PTE_P) && pa) {
            *pa = pte[pte_idx] & PAGE_MASK; // 计算物理页面
            return 1; // 页面存在
        }
        return 0; // 页表项不存在
    }

    char *cvm::vmm_getstr(uint32_t va) {
        uint32_t pa;
        if (vmm_ismap(va, &pa)) {
            return (char *) pa + OFFSET_INDEX(va);
        }
        vmm_map(va, pmm_alloc(), PTE_U | PTE_P | PTE_R);
#if 0
        printf("VMMSTR> Invalid VA: %08X\n", va);
#endif
        assert(0);
        return vmm_getstr(va);
    }

    template<class T>
    T cvm::vmm_get(uint32_t va) {
        if (ctx->flag & CTX_KERNEL)
            va |= ctx->mask;
        uint32_t pa;
        if (vmm_ismap(va, &pa)) {
            return *(T *) ((byte *) pa + OFFSET_INDEX(va));
        }
        vmm_map(va, pmm_alloc(), PTE_U | PTE_P | PTE_R);
#if 0
        printf("VMMGET> Invalid VA: %08X\n", va);
#endif
        error("vmm::get error");
        return vmm_get<T>(va);
    }

    template<class T>
    T cvm::vmm_set(uint32_t va, T value) {
        uint32_t pa;
        if (ctx->flag & CTX_KERNEL)
            va |= ctx->mask;
        if (vmm_ismap(va, &pa)) {
            *(T *) ((byte *) pa + OFFSET_INDEX(va)) = value;
            return value;
        }
        vmm_map(va, pmm_alloc(), PTE_U | PTE_P | PTE_R);
#if 0
        printf("VMMSET> Invalid VA: %08X\n", va);
#endif
        error("vmm::set error");
        return vmm_set(va, value);
    }

    void cvm::vmm_setstr(uint32_t va, const char *value) {
        auto len = strlen(value);
        for (uint32_t i = 0; i < len; i++) {
            vmm_set(va + i, value[i]);
        }
        vmm_set(va + len, '\0');
    }

    uint32_t vmm_pa2va(uint32_t base, uint32_t size, uint32_t pa) {
        return base + (pa & (SEGMENT_MASK));
    }

    uint32_t cvm::vmm_malloc(uint32_t size) {
#if 0
        printf("MALLOC> Available: %08X\n", heap.available() * 0x10);
#endif
        auto ptr = heap.alloc_array<byte>(size);
        if (ptr == nullptr) {
            printf("out of memory");
            exit(-1);
        }
        if (ptr < heapHead) {
            heap.alloc_array<byte>(heapHead - ptr);
#if 0
            printf("MALLOC> Skip %08X bytes\n", heapHead - ptr);
#endif
            return vmm_malloc(size);
        }
        if (ptr + size >= heapHead + HEAP_SIZE * PAGE_SIZE) {
            printf("out of memory");
            exit(-1);
        }
        auto va = vmm_pa2va(HEAP_BASE, HEAP_SIZE, ((uint32_t) ptr - (uint32_t) heapHead));
#if 0
        printf("MALLOC> V=%08X P=%p> %08X bytes\n", va, ptr, size);
#endif
        return va;
    }

    uint32_t cvm::vmm_memset(uint32_t va, uint32_t value, uint32_t count) {
#if 0
        uint32_t pa;
        if (vmm_ismap(va, &pa))
        {
            pa |= OFFSET_INDEX(va);
            printf("MEMSET> V=%08X P=%08X S=%08X\n", va, pa, count);
        }
        else
        {
            printf("MEMSET> V=%08X P=ERROR S=%08X\n", va, count);
        }
#endif
        for (uint32_t i = 0; i < count; i++) {
#if 0
            printf("MEMSET> V=%08X\n", va + i);
#endif
            vmm_set<byte>(va + i, value);
        }
        return 0;
    }

    uint32_t cvm::vmm_memcmp(uint32_t src, uint32_t dst, uint32_t count) {
        for (uint32_t i = 0; i < count; i++) {
#if 0
            printf("MEMCMP> '%c':'%c'\n", vmm_get<byte>(src + i), vmm_get<byte>(dst + i));
#endif
            if (vmm_get<byte>(src + i) > vmm_get<byte>(dst + i))
                return 1;
            if (vmm_get<byte>(src + i) < vmm_get<byte>(dst + i))
                return -1;
        }
        return 0;
    }

    template<class T>
    void cvm::vmm_pushstack(uint32_t &sp, T value) {
        ctx->sp -= sizeof(T);
        vmm_set(ctx->sp, value);
    }

    template<class T>
    T cvm::vmm_popstack(uint32_t &sp) {
        T t = vmm_get(ctx->sp);
        ctx->sp += sizeof(T);
        return t;
    }

    //-----------------------------------------

    cvm::cvm() {
        vmm_init();
    }

    cvm::~cvm() {
        free(pgd_kern);
        free(pte_kern);
    }

    void cvm::init_args(uint32_t *args, uint32_t sp, uint32_t pc, bool converted /*= false*/) {
        auto num = vmm_get(pc + INC_PTR); /* 利用之后的ADJ清栈指令知道函数调用的参数个数 */
        auto tmp = VMM_ARG(ctx->sp, num);
        for (int k = 0; k < num; k++) {
            auto arg = VMM_ARGS(tmp, k + 1);
            if (converted && (arg & DATA_BASE))
                args[k] = (uint32_t) vmm_getstr(arg);
            else
                args[k] = (uint32_t) arg;
        }
    }

    bool cvm::run(int cycle, int &cycles) {
        for (int i = 0; i < TASK_NUM; ++i) {
            if (tasks[i].flag & CTX_VALID) {
                ctx = &tasks[i];
                exec(cycle, cycles);
            }
        }
        ctx = nullptr;
        return available_tasks > 0;
    }

    void cvm::exec(int cycle, int &cycles) {
        if (!ctx)
            error("no process!");
        uint32_t args[6];
        for (auto i = 0; i < cycle; ++i) {
            i++;
            cycles++;
            auto op = vmm_get(ctx->pc); // get next operation code
            ctx->pc += INC_PTR;

#if LOG_INS
            assert(op <= EXIT);
            // print debug info
            {
                printf("%04d> [%08X] %02d %.4s", i, ctx->pc, op, INS_STRING((ins_t) op).c_str());
                if (op == PUSH)
                    printf(" %08X\n", (uint32_t) ctx->ax);
                else if (op <= ADJ)
                    printf(" %08X(%d)\n", vmm_get(ctx->pc), vmm_get(ctx->pc));
                else
                    printf("\n");
            }
#endif
            switch (op) {
                case IMM: {
                    ctx->ax = vmm_get(ctx->pc);
                    ctx->pc += INC_PTR;
                } /* load immediate value to ctx->ax */
                    break;
                case LOAD: {
                    switch (vmm_get(ctx->pc)) {
                        case 1:
                            ctx->ax = vmm_get<byte>((uint32_t) ctx->ax);
                            break;
                        case 2:
                        case 3:
                        case 4:
                            ctx->ax = vmm_get((uint32_t) ctx->ax);
                            break;
                        default:
                            // NOT SUPPORT LONG TYPE NOW
                            ctx->ax = vmm_get((uint32_t) ctx->ax);
                            break;
                    }
                    ctx->pc += INC_PTR;
                } /* load integer to ctx->ax, address in ctx->ax */
                    break;
                case SAVE: {
                    switch (vmm_get(ctx->pc)) {
                        case 1:
                            vmm_set<byte>((uint32_t) vmm_popstack(ctx->sp), (byte) ctx->ax);
                            break;
                        case 2:
                        case 3:
                        case 4:
                            vmm_set((uint32_t) vmm_popstack(ctx->sp), ctx->ax);
                            break;
                        default:
                            // NOT SUPPORT LONG TYPE NOW
                            vmm_set((uint32_t) vmm_popstack(ctx->sp), ctx->ax);
                            break;
                    }
                    ctx->pc += INC_PTR;
                } /* save integer to address, value in ctx->ax, address on stack */
                    break;
                case PUSH: {
                    vmm_pushstack(ctx->sp, ctx->ax);
                } /* push the value of ctx->ax onto the stack */
                    break;
                case POP: {
                    ctx->ax = vmm_popstack(ctx->sp);
                } /* pop the value of ctx->ax from the stack */
                    break;
                case JMP: {
                    ctx->pc = ctx->base + vmm_get(ctx->pc) * INC_PTR;
                } /* jump to the address */
                    break;
                case JZ: {
                    ctx->pc = ctx->ax ? ctx->pc + INC_PTR : (ctx->base + vmm_get(ctx->pc) * INC_PTR);
                } /* jump if ctx->ax is zero */
                    break;
                case JNZ: {
                    ctx->pc = ctx->ax ? (ctx->base + vmm_get(ctx->pc) * INC_PTR) : ctx->pc + INC_PTR;
                } /* jump if ctx->ax is zero */
                    break;
                case CALL: {
                    vmm_pushstack(ctx->sp, ctx->pc + INC_PTR);
                    ctx->pc = ctx->base + vmm_get(ctx->pc) * INC_PTR;
#if 0
                    printf("CALL> PC=%08X\n", ctx->pc);
#endif
                } /* call subroutine */
                    /* break;case RET: {pc = (int *)*sp++;} // return from subroutine; */
                    break;
                case ENT: {
                    vmm_pushstack(ctx->sp, ctx->bp);
                    ctx->bp = ctx->sp;
                    ctx->sp = ctx->sp - vmm_get(ctx->pc);
                    ctx->pc += INC_PTR;
                } /* make new stack frame */
                    break;
                case ADJ: {
                    ctx->sp = ctx->sp + vmm_get(ctx->pc) * INC_PTR;
                    ctx->pc += INC_PTR;
                } /* add esp, <size> */
                    break;
                case LEV: {
                    ctx->sp = ctx->bp;
                    ctx->bp = (uint32_t) vmm_popstack(ctx->sp);
                    ctx->pc = (uint32_t) vmm_popstack(ctx->sp);
#if 0
                    printf("RETURN> PC=%08X\n", ctx->pc);
#endif
                } /* restore call frame and PC */
                    break;
                case LEA: {
                    ctx->ax = ctx->bp + vmm_get(ctx->pc);
                    ctx->pc += INC_PTR;
                } /* load address for arguments. */
                    break;
                case OR:
                    ctx->ax = vmm_popstack(ctx->sp) | ctx->ax;
                    break;
                case XOR:
                    ctx->ax = vmm_popstack(ctx->sp) ^ ctx->ax;
                    break;
                case AND:
                    ctx->ax = vmm_popstack(ctx->sp) & ctx->ax;
                    break;
                case EQ:
                    ctx->ax = vmm_popstack(ctx->sp) == ctx->ax;
                    break;
                case CASE:
                    if (vmm_get(ctx->sp) == ctx->ax) {
                        ctx->sp += INC_PTR;
                        ctx->ax = 0; // 0 for same
                    } else {
                        ctx->ax = 1;
                    }
                    break;
                case NE:
                    ctx->ax = vmm_popstack(ctx->sp) != ctx->ax;
                    break;
                case LT:
                    ctx->ax = vmm_popstack(ctx->sp) < ctx->ax;
                    break;
                case LE:
                    ctx->ax = vmm_popstack(ctx->sp) <= ctx->ax;
                    break;
                case GT:
                    ctx->ax = vmm_popstack(ctx->sp) > ctx->ax;
                    break;
                case GE:
                    ctx->ax = vmm_popstack(ctx->sp) >= ctx->ax;
                    break;
                case SHL:
                    ctx->ax = vmm_popstack(ctx->sp) << ctx->ax;
                    break;
                case SHR:
                    ctx->ax = vmm_popstack(ctx->sp) >> ctx->ax;
                    break;
                case ADD:
                    ctx->ax = vmm_popstack(ctx->sp) + ctx->ax;
                    break;
                case SUB:
                    ctx->ax = vmm_popstack(ctx->sp) - ctx->ax;
                    break;
                case MUL:
                    ctx->ax = vmm_popstack(ctx->sp) * ctx->ax;
                    break;
                case DIV:
                    ctx->ax = vmm_popstack(ctx->sp) / ctx->ax;
                    break;
                case MOD:
                    ctx->ax = vmm_popstack(ctx->sp) % ctx->ax;
                    break;
                case EXIT: {
#if LOG_SYSTEM
                    printf("[SYSTEM] PROC | Exit: PID= #%d, CODE= %d\n", ctx->id, ctx->ax);
#endif
                    destroy();
                    return;
                }
                case INTR: { // 中断调用，以寄存器ax传参
                    switch (vmm_get(ctx->pc)) {
                        case 0:
                            cgui::singleton().put_char((char) ctx->ax);
                            break;
                        case 1:
                            cgui::singleton().put_int((int) ctx->ax);
                            break;
                        case 100:
                            cgui::singleton().record((int) ctx->ax);
                            break;
                        case 101:
                            if (!cgui::singleton().reach()) {
                                ctx->pc -= INC_PTR;
                                return;
                            }
                            break;
                        default:
                            printf("unknown interrupt:%d\n", ctx->ax);
                            error("unknown interrupt");
                            break;
                    }
                    ctx->pc += INC_PTR;
                    break;
                }
                default: {
                    printf("AX: %08X BP: %08X SP: %08X PC: %08X\n", ctx->ax, ctx->bp, ctx->sp, ctx->pc);
                    for (uint32_t j = ctx->sp; j < STACK_BASE + PAGE_SIZE; j += 4) {
                        printf("[%08X]> %08X\n", j, vmm_get<uint32_t>(j));
                    }
                    printf("unknown instruction:%d\n", op);
                    error("unknown instruction");
                }
            }

#if LOG_STACK
            if (ctx->log) {
                printf("\n---------------- STACK BEGIN <<<< \n");
                printf("AX: %08X BP: %08X SP: %08X PC: %08X\n", ctx->ax, ctx->bp, ctx->sp, ctx->pc);
                for (uint32_t j = ctx->sp; j < STACK_BASE + PAGE_SIZE; j += 4) {
                    printf("[%08X]> %08X\n", j, vmm_get<uint32_t>(j));
                }
                printf("---------------- STACK END >>>>\n\n");
            }
#endif
        }
        return;
    }

    void cvm::error(const string_t &str) {
        std::stringstream ss;
        ss << "VM ERROR: " << str;
        throw cexception(ss.str());
    }

    void cvm::load(std::vector<byte> file) {
        if (available_tasks >= TASK_NUM) {
            error("max process num!");
        }
        auto old_ctx = ctx;
        for (int i = 0; i < TASK_NUM; ++i) {
            if (!(tasks[i].flag & CTX_VALID)) {
                tasks[i].flag |= CTX_VALID;
                ctx = &tasks[i];
                ctx->id = i;
                ctx->file = file;
                break;
            }
        }
#if LOG_SYSTEM
        printf("[SYSTEM] PROC | Create: PID= #%d\n", ctx->id);
#endif
        PE *pe = (PE *) file.data();
        // TODO: VALID PE FILE
        uint32_t pa;
        ctx->poolsize = PAGE_SIZE;
        ctx->mask = ((uint) (ctx->id << 16) & 0x00ff0000);
        ctx->entry = pe->entry;
        ctx->stack = STACK_BASE | ctx->mask;
        ctx->data = DATA_BASE | ctx->mask;
        ctx->base = USER_BASE | ctx->mask;
        ctx->heap = HEAP_BASE | ctx->mask;
        ctx->flag |= CTX_KERNEL;
        /* 映射4KB的代码空间 */
        {
            auto size = PAGE_SIZE / sizeof(int);
            auto text_size = pe->text_len / sizeof(int);
            auto text_start = (uint32_t *) (&pe->data + pe->data_len);
            for (uint32_t i = 0, start = 0; start < text_size; ++i, start += size) {
                vmm_map(ctx->base + PAGE_SIZE * i, (uint32_t) pmm_alloc(), PTE_U | PTE_P | PTE_R); // 用户代码空间
                if (vmm_ismap(ctx->base + PAGE_SIZE * i, &pa)) {
                    auto s = start + size > text_size ? (text_size & (size - 1)) : size;
                    for (uint32_t j = 0; j < s; ++j) {
                        *((uint32_t *) pa + j) = (uint) text_start[start + j];
#if 0
                        printf("[%p]> [%08X] %08X\n", (void*)((int*)pa + j), ctx->base + PAGE_SIZE * i + j * 4, vmm_get<uint32_t>(ctx->base + PAGE_SIZE * i + j * 4));
#endif
                    }
                }
            }
        }
        /* 映射4KB的数据空间 */
        {
            auto size = PAGE_SIZE;
            auto data_size = pe->data_len;
            auto data_start = (char *) &pe->data;
            for (uint32_t i = 0, start = 0; start < data_size; ++i, start += size) {
                vmm_map(ctx->data + PAGE_SIZE * i, (uint32_t) pmm_alloc(), PTE_U | PTE_P | PTE_R); // 用户数据空间
                if (vmm_ismap(ctx->data + PAGE_SIZE * i, &pa)) {
                    auto s = start + size > data_size ? ((sint) data_size & (size - 1)) : size;
                    for (uint32_t j = 0; j < s; ++j) {
                        *((char *) pa + j) = data_start[start + j];
#if 0
                        printf("[%p]> [%08X] %d\n", (void*)((char*)pa + j), ctx->data + PAGE_SIZE * i + j, vmm_get<byte>(ctx->data + PAGE_SIZE * i + j));
#endif
                    }
                }
            }
        }
        /* 映射4KB的栈空间 */
        vmm_map(ctx->stack, (uint32_t) pmm_alloc(), PTE_U | PTE_P | PTE_R); // 用户栈空间
        /* 映射16KB的堆空间 */
        {
            auto head = heap.alloc_array<byte>(PAGE_SIZE * (HEAP_SIZE + 2));
#if 0
            printf("HEAP> ALLOC=%p\n", head);
#endif
            heapHead = head; // 得到内存池起始地址
            heap.free_array(heapHead);
            heapHead = (byte *) PAGE_ALIGN_UP((uint32_t) head);
#if 0
            printf("HEAP> HEAD=%p\n", heapHead);
#endif
            memset(heapHead, 0, PAGE_SIZE * HEAP_SIZE);
            for (int i = 0; i < HEAP_SIZE; ++i) {
                vmm_map(ctx->heap + PAGE_SIZE * i, (uint32_t) heapHead + PAGE_SIZE * i, PTE_U | PTE_P | PTE_R);
            }
        }
        ctx->flag &= ~CTX_KERNEL;
        {
            ctx->sp = ctx->stack + ctx->poolsize; // 4KB / sizeof(int) = 1024
            {
                auto argvs = vmm_malloc((uint32_t) g_argc * INC_PTR);
                for (auto i = 0; i < g_argc; i++) {
                    auto str = vmm_malloc(256);
                    vmm_setstr(str, g_argv[i]);
                    vmm_set(argvs + INC_PTR * i, str);
                }

                vmm_pushstack(ctx->sp, EXIT);
                vmm_pushstack(ctx->sp, PUSH);
                auto tmp = ctx->sp;
                vmm_pushstack(ctx->sp, g_argc);
                vmm_pushstack(ctx->sp, argvs);
                vmm_pushstack(ctx->sp, tmp);
            }

            ctx->stack = STACK_BASE;
            ctx->data = DATA_BASE;
            ctx->base = USER_BASE;
            ctx->heap = HEAP_BASE;
            ctx->pc = ctx->base | (ctx->entry * INC_PTR);
            ctx->ax = 0;
            ctx->bp = 0;
            ctx->log = true;
        }
        available_tasks++;
        ctx = old_ctx;
    }

    void cvm::destroy() {
        ctx->flag = 0;
        ctx->file.clear();
        auto old_ctx = ctx;
        {
            PE *pe = (PE *) ctx->file.data();
            ctx->poolsize = PAGE_SIZE;
            ctx->mask = ((uint) (ctx->id << 16) & 0x00ff0000);
            ctx->entry = pe->entry;
            ctx->stack = STACK_BASE | ctx->mask;
            ctx->data = DATA_BASE | ctx->mask;
            ctx->base = USER_BASE | ctx->mask;
            ctx->heap = HEAP_BASE | ctx->mask;
            ctx->flag |= CTX_KERNEL;
            /* 映射4KB的代码空间 */
            {
                auto size = PAGE_SIZE / sizeof(int);
                auto text_size = pe->text_len / sizeof(int);
                for (uint32_t i = 0, start = 0; start < text_size; ++i, start += size) {
                    vmm_unmap(ctx->base + PAGE_SIZE * i); // 用户代码空间
                }
            }
            /* 映射4KB的数据空间 */
            {
                auto size = PAGE_SIZE;
                auto data_size = pe->data_len;
                for (uint32_t i = 0, start = 0; start < data_size; ++i, start += size) {
                    vmm_unmap(ctx->data + PAGE_SIZE * i); // 用户数据空间
                }
            }
            /* 映射4KB的栈空间 */
            vmm_unmap(ctx->stack); // 用户栈空间
            /* 映射16KB的堆空间 */
            {
                for (int i = 0; i < HEAP_SIZE; ++i) {
                    vmm_unmap(HEAP_BASE + PAGE_SIZE * i);
                }
            }
        }
        ctx = old_ctx;
        available_tasks--;
    }
}
