//
// Project: CMiniLang
// Author: bajdcc
//

#include <cassert>
#include <memory.h>
#include <cstring>
#include <regex>
#include "cvm.h"
#include "cgen.h"
#include "cexception.h"
#include "cgui.h"

#define LOG_INS 1
#define LOG_STACK 1
#define LOG_SYSTEM 1

int g_argc;
char **g_argv;

namespace clib {

#define INC_PTR 4

    cvm::global_state_t cvm::global_state;

    uint32_t cvm::pmm_alloc(bool reusable) {
        auto ptr = (uint32_t) memory.alloc_array<byte>(PAGE_SIZE * 2);
        if (!ptr)
            error("alloc page failed");
        if (reusable)
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
            tasks[i].id = i;
            tasks[i].parent = -1;
            tasks[i].state = CTS_DEAD;
        }
        for (i = 0; i < HANDLE_NUM; ++i) {
            handles[i].type = h_none;
        }

        fs.as_root(true);
        fs.mkdir("/proc");
        fs.as_root(false);
    }

    // 虚页映射
    // va = 虚拟地址  pa = 物理地址
    void cvm::vmm_map(uint32_t va, uint32_t pa, uint32_t flags) {
        uint32_t pde_idx = PDE_INDEX(va); // 页目录号
        uint32_t pte_idx = PTE_INDEX(va); // 页表号

        pte_t *pte = (pte_t *) (pgdir[pde_idx] & PAGE_MASK); // 页表

        if (!pte) { // 缺页
            if (va >= USER_BASE) { // 若是用户地址则转换
                pte = (pte_t *) pmm_alloc(false); // 申请物理页框，用作新页表
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

#if 0
        printf("MEMUNMAP> V=%08X\n", va);
#endif
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

    string_t cvm::vmm_getstr(uint32_t va) {
        std::stringstream ss;
        char c;
        while (c = vmm_get<char>(va++), c) ss << c;
        return ss.str();
    }

    template<class T>
    T cvm::vmm_get(uint32_t va) {
        if (!(ctx->flag & CTX_KERNEL))
            va |= ctx->mask;
        uint32_t pa;
        if (vmm_ismap(va, &pa)) {
            return *(T *) ((byte *) pa + OFFSET_INDEX(va));
        }
        //vmm_map(va, pmm_alloc(), PTE_U | PTE_P | PTE_R);
#if 0
        printf("VMMGET> Invalid VA: %08X\n", va);
#endif
        error("vmm::get error");
        return vmm_get<T>(va);
    }

    template<class T>
    T cvm::vmm_set(uint32_t va, T value) {
        if (!(ctx->flag & CTX_KERNEL)) {
            if ((ctx->flag & CTX_USER_MODE) && (va & 0xF0000000) == USER_BASE) {
                error("code segment cannot be written");
            }
            va |= ctx->mask;
        }
        uint32_t pa;
        if (vmm_ismap(va, &pa)) {
            *(T *) ((byte *) pa + OFFSET_INDEX(va)) = value;
            return value;
        }
        //vmm_map(va, pmm_alloc(), PTE_U | PTE_P | PTE_R);
#if 1
        printf("VMMSET> Invalid VA: %08X\n", va);
#endif
        error("vmm::set error");
        return vmm_set(va, value);
    }

    void cvm::vmm_setstr(uint32_t va, const string_t &str) {
        auto len = str.length();
        for (uint32_t i = 0; i < len; i++) {
            vmm_set(va + i, str[i]);
        }
        vmm_set(va + len, '\0');
    }

    uint32_t vmm_pa2va(uint32_t base, uint32_t pa) {
        return base + (pa & (SEGMENT_MASK));
    }

    uint32_t cvm::vmm_malloc(uint32_t size) {
        return vmm_pa2va(ctx->heap, ctx->pool->alloc(size));
    }

    uint32_t cvm::vmm_free(uint32_t addr) {
        if ((addr & 0xF0000000) != HEAP_BASE) {
            return 0;
        }
        return ctx->pool->free(K2U(addr));
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
                return 2;
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

    bool cvm::run(int cycle, int &cycles) {
        for (int i = 0; i < TASK_NUM; ++i) {
            if (tasks[i].flag & CTX_VALID) {
                if (tasks[i].state == CTS_RUNNING) {
                    ctx = &tasks[i];
                    exec(cycle, cycles);
                } else if (tasks[i].state == CTS_WAIT &&
                           tasks[i].input_redirect != -1 &&
                           tasks[i].output_redirect != -1) {
                    copy(tasks[i].input_queue.begin(), tasks[i].input_queue.end(),
                         std::back_inserter(tasks[tasks[i].output_redirect].input_queue));
                    tasks[i].input_queue.clear();
                }
            }
        }
        ctx = nullptr;
        return available_tasks > 0;
    }

    void cvm::exec(int cycle, int &cycles) {
        if (!ctx)
            error("no process!");
        for (auto i = 0; i < cycle; ++i) {
            i++;
            cycles++;
            if ((ctx->pc & 0xF0000000) != USER_BASE) {
                if (ctx->pc != 0xE0000FF8 && ctx->pc != 0xE0000FFC) {
#if LOG_SYSTEM
                    printf("[SYSTEM] ERR  | Invalid PC: %p\n", (void *) ctx->pc);
#endif
                    error("only code segment can execute");
                }
            }
            auto op = vmm_get(ctx->pc); // get next operation code
            ctx->pc += INC_PTR;

#if LOG_INS
            assert(op <= EXIT);
            // print debug info
            if (ctx->debug) {
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
                case IMX: {
                    ctx->ax = vmm_get(ctx->pc);
                    ctx->pc += INC_PTR;
                    ctx->bx = vmm_get(ctx->pc);
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
                    destroy(ctx->id);
                    return;
                }
                case INTR: // 中断调用，以寄存器ax传参
                    if (interrupt())
                        return;
                    break;
                default: {
#if LOG_SYSTEM
                    printf("[SYSTEM] ERR  | AX: %08X BP: %08X SP: %08X PC: %08X\n", ctx->ax, ctx->bp, ctx->sp, ctx->pc);
                    for (uint32_t j = ctx->sp; j < STACK_BASE + PAGE_SIZE; j += 4) {
                        printf("[SYSTEM] ERR  | [%08X]> %08X\n", j, vmm_get<uint32_t>(j));
                    }
                    printf("[SYSTEM] ERR  | unknown instruction: %d\n", op);
#endif
                    error("unknown instruction");
                }
            }

#if LOG_STACK
            if (ctx->debug) {
                printf("\n---------------- STACK BEGIN <<<< \n");
                printf("AX: %08X BP: %08X SP: %08X PC: %08X\n", ctx->ax, ctx->bp, ctx->sp, ctx->pc);
                for (uint32_t j = ctx->sp; j < STACK_BASE + PAGE_SIZE; j += 4) {
                    printf("[%08X]> %08X\n", j, vmm_get<uint32_t>(j));
                }
                printf("---------------- STACK END >>>>\n\n");
            }
#endif
        }
    }

    void cvm::error(const string_t &str) {
        throw cexception(ex_vm, str);
    }

    int cvm::load(const string_t &path, const std::vector<byte> &file, const std::vector<string_t> &args) {
        auto old_ctx = ctx;
        new_pid();
        ctx->file = file;
#if LOG_SYSTEM
        printf("[SYSTEM] PROC | Create: PID= #%d\n", ctx->id);
#endif
        PE *pe = (PE *) file.data();
        // TODO: VALID PE FILE
        uint32_t pa;
        ctx->poolsize = PAGE_SIZE;
        ctx->mask = U2K(ctx->id);
        ctx->entry = pe->entry;
        ctx->stack = STACK_BASE | ctx->mask;
        ctx->data = DATA_BASE | ctx->mask;
        ctx->base = USER_BASE | ctx->mask;
        ctx->heap = HEAP_BASE | ctx->mask;
        ctx->pool = std::make_unique<cmem>(this);
        ctx->flag |= CTX_KERNEL;
        ctx->state = CTS_RUNNING;
        ctx->path = path;
        /* 映射4KB的代码空间 */
        {
            auto size = PAGE_SIZE / sizeof(int);
            auto text_size = pe->text_len / sizeof(int);
            auto text_start = (uint32_t *) (&pe->data + pe->data_len);
            for (uint32_t i = 0, start = 0; start < text_size; ++i, start += size) {
                auto new_page = (uint32_t) pmm_alloc();
                ctx->text_mem.push_back(new_page);
                vmm_map(ctx->base + PAGE_SIZE * i, new_page, PTE_U | PTE_P); // 用户代码空间
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
                auto new_page = (uint32_t) pmm_alloc();
                ctx->data_mem.push_back(new_page);
                vmm_map(ctx->data + PAGE_SIZE * i, new_page, PTE_U | PTE_P | PTE_R); // 用户数据空间
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
        {
            auto new_page = (uint32_t) pmm_alloc();
            ctx->stack_mem.push_back(new_page);
            vmm_map(ctx->stack, new_page, PTE_U | PTE_P | PTE_R); // 用户栈空间
        }
        ctx->flag &= ~CTX_KERNEL;
        {
            ctx->stack = STACK_BASE;
            ctx->data = DATA_BASE;
            ctx->base = USER_BASE;
            ctx->heap = HEAP_BASE;
            ctx->sp = ctx->stack + ctx->poolsize; // 4KB / sizeof(int) = 1024
            ctx->pc = ctx->base | (ctx->entry * INC_PTR);
            ctx->ax = 0;
            ctx->bx = 0;
            ctx->bp = 0;

            auto _argc = args.size();
            assert(_argc > 0);
            vmm_pushstack(ctx->sp, EXIT);
            vmm_pushstack(ctx->sp, PUSH);
            auto tmp = ctx->sp;
            auto argvs = vmm_malloc(_argc * INC_PTR);
            vmm_pushstack(ctx->sp, _argc);
            vmm_pushstack(ctx->sp, argvs);
            for (auto i = 0; i < _argc; i++) {
                auto str = vmm_malloc(args[i].length() + 1);
                vmm_setstr(str, args[i]);
                vmm_set(argvs + INC_PTR * i, str);
            }
            vmm_pushstack(ctx->sp, tmp);
        }
        ctx->flag |= CTX_USER_MODE;
        ctx->debug = false;
        ctx->waiting_ms = 0;
        ctx->input_redirect = -1;
        ctx->output_redirect = -1;
        ctx->input_queue.clear();
        ctx->input_stop = false;
        available_tasks++;
        auto pid = ctx->id;
        ctx = old_ctx;
        return pid;
    }

    void cvm::destroy(int id) {
        auto old_ctx = ctx;
        ctx = &tasks[id];
        {
            if (global_state.input_lock == ctx->id) {
                global_state.input_lock = -1;
                global_state.input_read_ptr = -1;
                global_state.input_content.clear();
                global_state.input_success = false;
                cgui::singleton().reset_cmd();
            }
            if (ctx->output_redirect != -1 && tasks[ctx->output_redirect].flag & CTX_VALID) {
                if (tasks[id].input_redirect != -1) {
                    copy(tasks[id].input_queue.begin(), tasks[id].input_queue.end(),
                         std::back_inserter(tasks[tasks[id].output_redirect].input_queue));
                    tasks[id].input_queue.clear();
                }
                tasks[ctx->output_redirect].input_stop = true;
                ctx->output_redirect = -1;
            }
        }
        if (!ctx->child.empty()) {
            ctx->state = CTS_ZOMBIE;
            ctx = old_ctx;
            return;
        }
#if LOG_SYSTEM
        printf("[SYSTEM] PROC | Destroy: PID= #%d\n", ctx->id);
#endif
        ctx->flag = 0;
        {
            PE *pe = (PE *) ctx->file.data();
            ctx->poolsize = PAGE_SIZE;
            ctx->mask = U2K(ctx->id);
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
                for (int i = 0; i < ctx->pool->page_size(); ++i) {
                    vmm_unmap(ctx->heap + PAGE_SIZE * i);
                }
            }
            {
                for (auto &a : ctx->allocation) {
                    memory.free_array((byte *) a);
                }
            }
            ctx->child.clear();
            ctx->state = CTS_DEAD;
            ctx->file.clear();
            ctx->allocation.clear();
            ctx->pool.reset();
            ctx->flag = 0;
            for (auto &h : ctx->handles) {
                destroy_handle(h);
            }
            ctx->handles.clear();
            if (ctx->parent != -1) {
                auto &parent = tasks[ctx->parent];
                parent.child.erase(ctx->id);
                if (parent.state == CTS_ZOMBIE)
                    destroy(ctx->parent);
                else if (parent.state == CTS_WAIT)
                    parent.state = CTS_RUNNING;
                ctx->parent = -1;
            }
            ctx->data_mem.clear();
            ctx->text_mem.clear();
            ctx->stack_mem.clear();
            ctx->input_queue.clear();
            {
                std::stringstream ss;
                ss << "/proc/" << ctx->id;
                fs.rm(ss.str());
            }
        }
        ctx = old_ctx;
        available_tasks--;
    }

    string_t get_args(const string_t &path, std::vector<string_t> &args) {
        std::stringstream ss(path);
        string_t temp;
        while (std::getline(ss, temp, ' ')) {
            args.push_back(temp);
        }
        return args.front();
    }

    string_t trim(string_t str) {
        string_t::size_type pos = str.find_last_not_of(' ');
        if (pos != string_t::npos) {
            str.erase(pos + 1);
            pos = str.find_first_not_of(' ');
            if (pos != string_t::npos) str.erase(0, pos);
        } else str.erase(str.begin(), str.end());
        return str;
    }

    int cvm::exec_file(const string_t &path) {
        if (path.empty())
            return -1;
        auto new_path = trim(path);
#if LOG_SYSTEM
        printf("[SYSTEM] PROC | Exec: Command= %s\n", new_path.data());
#endif
        std::vector<string_t> args;
        auto file = get_args(new_path, args);
        auto pid = cgui::singleton().compile(file, args);
        if (pid >= 0) { // SUCCESS
            ctx->child.insert(pid);
            tasks[pid].parent = ctx->id;
#if LOG_SYSTEM
            printf("[SYSTEM] PROC | Exec: Parent= #%d, Child= #%d\n", ctx->id, pid);
#endif
        }
        return pid;
    }

    int cvm::fork() {
        auto old_ctx = ctx;
        new_pid();
        ctx->file = old_ctx->file;
#if LOG_SYSTEM
        printf("[SYSTEM] PROC | Fork: Parent= #%d, Child= #%d\n", old_ctx->id, ctx->id);
#endif
        PE *pe = (PE *) ctx->file.data();
        // TODO: VALID PE FILE
        uint32_t pa;
        ctx->poolsize = PAGE_SIZE;
        ctx->mask = ((uint) (ctx->id << 16) & 0x00ff0000);
        ctx->entry = old_ctx->entry;
        ctx->stack = old_ctx->stack | ctx->mask;
        ctx->data = old_ctx->data | ctx->mask;
        ctx->base = old_ctx->base | ctx->mask;
        ctx->heap = old_ctx->heap | ctx->mask;
        ctx->pool = std::make_unique<cmem>(this);
        ctx->flag |= CTX_KERNEL;
        ctx->state = CTS_RUNNING;
        ctx->path = old_ctx->path;
        /* 映射4KB的代码空间 */
        {
            auto size = PAGE_SIZE / sizeof(int);
            auto text_size = pe->text_len / sizeof(int);
            for (uint32_t i = 0, start = 0; start < text_size; ++i, start += size) {
                auto new_page = (uint32_t) pmm_alloc();
                std::copy((byte *) (old_ctx->text_mem[i]),
                          (byte *) (old_ctx->text_mem[i]) + PAGE_SIZE,
                          (byte *) new_page);
                ctx->text_mem.push_back(new_page);
                vmm_map(ctx->base + PAGE_SIZE * i, new_page, PTE_U | PTE_P | PTE_R); // 用户代码空间
                if (!vmm_ismap(ctx->base + PAGE_SIZE * i, &pa)) {
                    destroy(ctx->id);
                    error("fork: text segment copy failed");
                }
            }
        }
        /* 映射4KB的数据空间 */
        {
            auto size = PAGE_SIZE;
            auto data_size = pe->data_len;
            for (uint32_t i = 0, start = 0; start < data_size; ++i, start += size) {
                auto new_page = (uint32_t) pmm_alloc();
                ctx->data_mem.push_back(new_page);
                std::copy((byte *) (old_ctx->data_mem[i]),
                          (byte *) (old_ctx->data_mem[i]) + PAGE_SIZE,
                          (byte *) new_page);
                vmm_map(ctx->data + PAGE_SIZE * i, new_page, PTE_U | PTE_P | PTE_R); // 用户数据空间
                if (!vmm_ismap(ctx->data + PAGE_SIZE * i, &pa)) {
                    destroy(ctx->id);
                    error("fork: data segment copy failed");
                }
            }
        }
        /* 映射4KB的栈空间 */
        {
            auto new_page = (uint32_t) pmm_alloc();
            ctx->stack_mem.push_back(new_page);
            std::copy((byte *) (old_ctx->stack_mem[0]),
                      (byte *) (old_ctx->stack_mem[0]) + PAGE_SIZE,
                      (byte *) new_page);
            vmm_map(ctx->stack, new_page, PTE_U | PTE_P | PTE_R); // 用户栈空间
            if (!vmm_ismap(ctx->stack, &pa)) {
                destroy(ctx->id);
                error("fork: stack segment copy failed");
            }
        }
        /* 映射堆空间 */
        ctx->pool->copy_from(*old_ctx->pool);
        ctx->flag &= ~CTX_KERNEL;
        ctx->sp = old_ctx->sp;
        ctx->stack = old_ctx->stack;
        ctx->data = old_ctx->data;
        ctx->base = old_ctx->base;
        ctx->heap = old_ctx->heap;
        ctx->pc = old_ctx->pc;
        ctx->ax = -1;
        ctx->bx = 0;
        ctx->bp = old_ctx->bp;
        ctx->debug = old_ctx->debug;
        ctx->waiting_ms = 0;
        ctx->input_redirect = old_ctx->input_redirect;
        ctx->output_redirect = old_ctx->output_redirect;
        ctx->input_stop = old_ctx->input_stop;
        ctx->handles = old_ctx->handles;
        available_tasks++;
        auto pid = ctx->id;
        ctx = old_ctx;
        return pid;
    }

    void cvm::map_page(uint32_t addr, uint32_t id) {
        uint32_t pa;
        auto va = (ctx->heap | ctx->mask) | (PAGE_SIZE * id);
        vmm_map(va, addr, PTE_U | PTE_P | PTE_R);
#if LOG_SYSTEM
        printf("[SYSTEM] MEM  | Map: PA= %p, VA= %p\n", (void *) addr, (void *) va);
#endif
        if (!vmm_ismap(va, &pa)) {
            destroy(ctx->id);
            error("heap alloc: alloc page failed");
        }
    }

    void cvm::as_root(bool flag) {
        fs.as_root(flag);
    }

    bool cvm::read_vfs(const string_t &path, std::vector<byte> &data) const {
        return fs.read_vfs(path, data);
    }

    bool cvm::write_vfs(const string_t &path, const std::vector<byte> &data) {
        return fs.write_vfs(path, data);
    }

    string_t cvm::callback(const string_t &path) {
        if (path.substr(0, 5) == "/proc") {
            static string_t pat{ R"(/proc/(\d+)/([a-z_]+))" };
            static std::regex re(pat);
            std::smatch res;
            if (std::regex_match(path, res, re)) {
                auto id = std::stoi(res[1].str());
                if (!(tasks[id].flag & CTX_VALID)) {
                    return "[ERROR] Invalid pid";
                }
                const auto &op = res[2].str();printf("%s\n",op.data());
                if (op == "exe") {
                    return tasks[id].path;
                } else if (op == "parent") {
                    char sz[16];
                    sprintf(sz, "%d", tasks[id].parent);
                    return sz;
                } else if (op == "heap_size") {
                    char sz[16];
                    sprintf(sz, "%d", tasks[id].pool->page_size());
                    return sz;
                }
            }
        }
        return "[ERROR] Undefined";
    }

    int cvm::new_pid() {
        if (available_tasks >= TASK_NUM) {
            error("max process num!");
        }
        auto end = TASK_NUM + pids;
        for (int i = pids; i < end; ++i) {
            auto j = i % TASK_NUM;
            if (!(tasks[j].flag & CTX_VALID)) {
                tasks[j].flag |= CTX_VALID;
                ctx = &tasks[j];
                pids = (j + 1) % TASK_NUM;
                ctx->id = j;
                {
                    std::stringstream ss;
                    ss << "/proc/" << j;
                    auto dir = ss.str();
                    fs.as_root(true);
                    if (fs.mkdir(dir) == 0) { // '/proc/[pid]'
                        static std::vector<string_t> ps =
                            {"exe", "parent", "heap_size"};
                        dir += "/";
                        for (auto &_ps : ps) {
                            ss.str("");
                            ss << dir << _ps;
                            fs.func(ss.str(), this);
                        }
                    }
                    fs.as_root(false);
                }
                return j;
            }
        }
        error("max process num!");
    }

    int cvm::new_handle(cvm::handle_type type) {
        if (available_handles >= HANDLE_NUM)
            error("max handle num!");
        auto end = HANDLE_NUM + handle_ids;
        for (int i = handle_ids; i < end; ++i) {
            auto j = i % HANDLE_NUM;
            if (handles[j].type == h_none) {
                handles[j].type = type;
                handle_ids = (j + 1) % HANDLE_NUM;
                available_handles++;
                ctx->handles.insert(j);
                return j;
            }
        }
        error("max handle num!");
    }

    int cvm::destroy_handle(int handle) {
        if (handles[handle].type != h_none) {
            auto h = &handles[handle];
            if (h->type == h_file) {
                delete h->data.file;
            }
            h->type = h_none;
            ctx->handles.erase(handle);
            available_handles--;
        } else {
            assert(!"destroy handle failed!");
        }
    }

    bool cvm::interrupt() {
        switch (vmm_get(ctx->pc)) {
            case 0:
                if (ctx->output_redirect != -1) {
                    tasks[ctx->output_redirect].input_queue.push_back((char) ctx->ax);
                } else if (global_state.input_lock == -1) {
                    cgui::singleton().put_char((char) ctx->ax);
                } else {
                    if (global_state.input_lock != ctx->id)
                        global_state.input_waiting_list.push_back(ctx->id);
                    ctx->state = CTS_WAIT;
                    ctx->pc -= INC_PTR;
                    return true;
                }
                break;
            case 1:
                if (ctx->output_redirect != -1) {
                    static char str[256];
                    sprintf(str, "%d", (int) ctx->ax);
                    auto s = str;
                    while (*s)
                        tasks[ctx->output_redirect].input_queue.push_back(*s++);
                }
                if (global_state.input_lock == -1) {
                    cgui::singleton().put_int((int) ctx->ax);
                } else {
                    if (global_state.input_lock != ctx->id)
                        global_state.input_waiting_list.push_back(ctx->id);
                    ctx->state = CTS_WAIT;
                    ctx->pc -= INC_PTR;
                    return true;
                }
                break;
            case 2:
                if (ctx->output_redirect != -1) {
                    static char str[256];
                    sprintf(str, "%p", (void *) ctx->ax);
                    auto s = str;
                    while (*s)
                        tasks[ctx->output_redirect].input_queue.push_back(*s++);
                } else if (global_state.input_lock == -1) {
                    cgui::singleton().put_hex((int) ctx->ax);
                } else {
                    if (global_state.input_lock != ctx->id)
                        global_state.input_waiting_list.push_back(ctx->id);
                    ctx->state = CTS_WAIT;
                    ctx->pc -= INC_PTR;
                    return true;
                }
                break;
            case 3:
                ctx->debug = !ctx->debug;
                break;
            case 5:
                vmm_setstr((uint32_t) ctx->ax, global_state.hostname);
                break;
            case 10: {
                if (ctx->input_redirect != -1) {
                    ctx->ax = ctx->input_stop ? 0 : 1;
                    ctx->pc += INC_PTR;
                } else {
                    if (global_state.input_lock == -1) {
                        global_state.input_lock = ctx->id;
                        ctx->pc += INC_PTR;
                        cgui::singleton().input_set(true);
                    } else {
                        global_state.input_waiting_list.push_back(ctx->id);
                        ctx->state = CTS_WAIT;
                        ctx->pc -= INC_PTR;
                    }
                }
                return true;
            }
            case 11: {
                if (ctx->input_redirect != -1) {
                    if (!ctx->input_queue.empty()) {
                        ctx->ax = ctx->input_queue.front();
                        ctx->input_queue.pop_front();
                        break;
                    } else if (!ctx->input_stop) {
                        ctx->pc -= INC_PTR;
                        return true;
                    }
                } else if (global_state.input_lock == ctx->id) {
                    if (global_state.input_success) {
                        if (global_state.input_read_ptr >= global_state.input_content.length()) {
                            ctx->ax = -1;
                            ctx->pc += INC_PTR;
                            // INPUT COMPLETE
                            for (auto &_id : global_state.input_waiting_list) {
                                if (tasks[_id].flag & CTX_VALID) {
                                    assert(tasks[_id].state == CTS_WAIT);
                                    tasks[_id].state = CTS_RUNNING;
                                }
                            }
                            global_state.input_lock = -1;
                            global_state.input_waiting_list.clear();
                            global_state.input_read_ptr = -1;
                            global_state.input_content.clear();
                            global_state.input_success = false;
                            cgui::singleton().input_set(false);
                            return true;
                        } else {
                            ctx->ax = global_state.input_content[global_state.input_read_ptr++];
                            break;
                        }
                    } else {
                        ctx->pc -= INC_PTR;
                        return true;
                    }
                }
                ctx->ax = -1;
                ctx->pc += INC_PTR;
                return true;
            }
            case 12: {
                if (ctx->input_redirect == -1 && global_state.input_lock == ctx->id) {
                    if (global_state.input_success) {
                        // INPUT INTERRUPT
                        for (auto &_id : global_state.input_waiting_list) {
                            if (tasks[_id].flag & CTX_VALID) {
                                assert(tasks[_id].state == CTS_WAIT);
                                tasks[_id].state = CTS_RUNNING;
                            }
                        }
                        global_state.input_lock = -1;
                        global_state.input_waiting_list.clear();
                        global_state.input_read_ptr = -1;
                        global_state.input_content.clear();
                        global_state.input_success = false;
                        cgui::singleton().input_set(false);
                    }
                }
            }
                break;
            case 13: {
                ctx->ax = ctx->input_redirect != -1 ? 0 : 1;
            }
                break;
            case 20: {
                if (global_state.input_lock == -1) {
                    cgui::singleton().resize(ctx->ax >> 16, ctx->ax & 0xFFFF);
                } else {
                    if (global_state.input_lock != ctx->id)
                        global_state.input_waiting_list.push_back(ctx->id);
                    ctx->state = CTS_WAIT;
                    ctx->pc -= INC_PTR;
                    return true;
                }
                break;
            }
            case 30:
                if (ctx->ax != 0)
                    ctx->ax = vmm_malloc((uint32_t) ctx->ax);
                break;
            case 31:
                ctx->ax = vmm_free((uint32_t) ctx->ax);
                break;
            case 51:
                ctx->ax = exec_file(vmm_getstr((uint32_t) ctx->ax));
                ctx->pc += INC_PTR;
                return true;
            case 50:
                ctx->ax = ctx->id;
                break;
            case 52: {
                if (!ctx->child.empty()) {
                    ctx->state = CTS_WAIT;
                    ctx->pc += INC_PTR;
                    return true;
                } else {
                    ctx->ax = -1;
                }
            }
                break;
            case 53: {
                ctx->ax = exec_file(vmm_getstr((uint32_t) ctx->ax));
                tasks[ctx->ax].state = CTS_WAIT;
                break;
            }
            case 54: {
                tasks[ctx->ax].state = CTS_RUNNING;
                break;
            }
            case 55: {
                ctx->pc += INC_PTR;
                ctx->ax = fork();
                return true;
            }
            case 56: {
                auto left = ctx->ax >> 16;
                auto right = ctx->ax & 0xFFFF;
                if ((left == ctx->id || ctx->child.find(left) != ctx->child.end()) &&
                    (right == ctx->id || ctx->child.find(right) != ctx->child.end())) {
                    tasks[right].input_redirect = left;
                    tasks[left].output_redirect = right;
                }
                break;
            }
            case 57: {
                std::vector<int> ids(ctx->child.begin(), ctx->child.end());
                for (auto &id : ids) {
                    destroy(id);
                }
                break;
            }
            case 60:
                vmm_setstr((uint32_t) ctx->ax, fs.get_pwd());
                break;
            case 61:
                vmm_setstr((uint32_t) ctx->ax, fs.get_user());
                break;
            case 62:
                ctx->ax = fs.cd(trim(vmm_getstr((uint32_t) ctx->ax)));
                break;
            case 63:
                ctx->ax = fs.mkdir(trim(vmm_getstr((uint32_t) ctx->ax)));
                break;
            case 64:
                ctx->ax = fs.touch(trim(vmm_getstr((uint32_t) ctx->ax)));
                break;
            case 65: {
                auto path = trim(vmm_getstr((uint32_t) ctx->ax));
                vfs_node_dec *dec;
                auto s = fs.get(path, &dec, this);
                if (s != 0) {
                    ctx->ax = s;
                    break;
                }
                auto h = new_handle(h_file);
                handles[h].name = path;
                handles[h].data.file = dec;
                ctx->ax = h;
            }
                break;
            case 66: {
                auto h = ctx->ax;
                if (ctx->handles.find(h) != ctx->handles.end()) {
                    auto dec = handles[h].data.file;
                    ctx->ax = dec->index();
                    if (ctx->ax >= 0)
                        dec->advance();
                } else {
                    ctx->ax = -3;
                }
            }
                break;
            case 67: {
                auto h = ctx->ax;
                if (ctx->handles.find(h) != ctx->handles.end()) {
                    destroy_handle(h);
                } else {
                    ctx->ax = -1;
                }
            }
                break;
            case 68: {
                ctx->ax = fs.rm_safe(trim(vmm_getstr((uint32_t) ctx->ax)));
            }
                break;
            case 100:
                ctx->record_now = std::chrono::high_resolution_clock::now();
                ctx->waiting_ms = ctx->ax * 0.001;
                break;
            case 101: {
                auto now = std::chrono::high_resolution_clock::now();
                if (std::chrono::duration_cast<std::chrono::duration<decimal>>(
                    now - ctx->record_now).count() <= ctx->waiting_ms) {
                    ctx->pc -= INC_PTR;
                    return true;
                }
            }
                break;
            default:
#if LOG_SYSTEM
                printf("[SYSTEM] ERR  | unknown interrupt: %d\n", ctx->ax);
#endif
                error("unknown interrupt");
                break;
        }
        ctx->pc += INC_PTR;
        return false;
    }
}
