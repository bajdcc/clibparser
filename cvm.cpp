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

#define LOG_INS 0
#define LOG_STACK 0

int g_argc;
char **g_argv;

namespace clib {

#define INC_PTR 4
#define VMM_ARG(s, p) ((s) + p * INC_PTR)
#define VMM_ARGS(t, n) vmm_get(t - (n) * INC_PTR)

    uint32_t cvm::pmm_alloc() {
        auto page = PAGE_ALIGN_UP((uint32_t) memory.alloc_array<byte>(PAGE_SIZE * 2));
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
    void cvm::vmm_unmap(pde_t *pde, uint32_t va) {
        uint32_t pde_idx = PDE_INDEX(va);
        uint32_t pte_idx = PTE_INDEX(va);

        pte_t *pte = (pte_t *) (pde[pde_idx] & PAGE_MASK);

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
        uint32_t pa;
        if (vmm_ismap(va, &pa)) {
            return *(T *) ((byte *) pa + OFFSET_INDEX(va));
        }
        vmm_map(va, pmm_alloc(), PTE_U | PTE_P | PTE_R);
#if 1
        printf("VMMGET> Invalid VA: %08X\n", va);
#endif
        error("vmm::get error");
        return vmm_get<T>(va);
    }

    template<class T>
    T cvm::vmm_set(uint32_t va, T value) {
        uint32_t pa;
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
        sp -= sizeof(T);
        vmm_set(sp, value);
    }

    template<class T>
    T cvm::vmm_popstack(uint32_t &sp) {
        T t = vmm_get(sp);
        sp += sizeof(T);
        return t;
    }

    //-----------------------------------------

    cvm::cvm(const std::vector<LEX_T(int)> &text, const std::vector<LEX_T(char)> &data) {
        vmm_init();
        uint32_t pa;
        /* 映射4KB的代码空间 */
        {
            auto size = PAGE_SIZE / sizeof(int);
            for (uint32_t i = 0, start = 0; start < text.size(); ++i, start += size) {
                vmm_map(USER_BASE + PAGE_SIZE * i, (uint32_t) pmm_alloc(), PTE_U | PTE_P | PTE_R); // 用户代码空间
                if (vmm_ismap(USER_BASE + PAGE_SIZE * i, &pa)) {
                    auto s = start + size > text.size() ? (text.size() & (size - 1)) : size;
                    for (uint32_t j = 0; j < s; ++j) {
                        *((uint32_t *) pa + j) = (uint) text[start + j];
#if 0
                        printf("[%p]> [%08X] %08X\n", (int*)pa + j, USER_BASE + PAGE_SIZE * i + j * 4, vmm_get<uint32_t>(USER_BASE + PAGE_SIZE * i + j * 4));
#endif
                    }
                }
            }
        }
        /* 映射4KB的数据空间 */
        {
            auto size = PAGE_SIZE;
            for (uint32_t i = 0, start = 0; start < data.size(); ++i, start += size) {
                vmm_map(DATA_BASE + PAGE_SIZE * i, (uint32_t) pmm_alloc(), PTE_U | PTE_P | PTE_R); // 用户数据空间
                if (vmm_ismap(DATA_BASE + PAGE_SIZE * i, &pa)) {
                    auto s = start + size > data.size() ? ((sint) data.size() & (size - 1)) : size;
                    for (uint32_t j = 0; j < s; ++j) {
                        *((char *) pa + j) = data[start + j];
#if 0
                        printf("[%p]> [%08X] %d\n", (char*)pa + j, DATA_BASE + PAGE_SIZE * i + j, vmm_get<byte>(DATA_BASE + PAGE_SIZE * i + j));
#endif
                    }
                }
            }
        }
        /* 映射4KB的栈空间 */
        vmm_map(STACK_BASE, (uint32_t) pmm_alloc(), PTE_U | PTE_P | PTE_R); // 用户栈空间
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
                vmm_map(HEAP_BASE + PAGE_SIZE * i, (uint32_t) heapHead + PAGE_SIZE * i, PTE_U | PTE_P | PTE_R);
            }
        }
    }

    cvm::~cvm() {
        free(pgd_kern);
        free(pte_kern);
    }

    void cvm::init_args(uint32_t *args, uint32_t sp, uint32_t pc, bool converted /*= false*/) {
        auto num = vmm_get(pc + INC_PTR); /* 利用之后的ADJ清栈指令知道函数调用的参数个数 */
        auto tmp = VMM_ARG(sp, num);
        for (int k = 0; k < num; k++) {
            auto arg = VMM_ARGS(tmp, k + 1);
            if (converted && (arg & DATA_BASE))
                args[k] = (uint32_t) vmm_getstr(arg);
            else
                args[k] = (uint32_t) arg;
        }
    }

    int cvm::exec(int entry) {
        auto poolsize = PAGE_SIZE;
        auto stack = STACK_BASE;
        auto data = DATA_BASE;
        auto base = USER_BASE;

        uint32_t sp = stack + poolsize; // 4KB / sizeof(int) = 1024

        {
            auto argvs = vmm_malloc((uint32_t) g_argc * INC_PTR);
            for (auto i = 0; i < g_argc; i++) {
                auto str = vmm_malloc(256);
                vmm_setstr(str, g_argv[i]);
                vmm_set(argvs + INC_PTR * i, str);
            }

            vmm_pushstack(sp, EXIT);
            vmm_pushstack(sp, PUSH);
            auto tmp = sp;
            vmm_pushstack(sp, g_argc);
            vmm_pushstack(sp, argvs);
            vmm_pushstack(sp, tmp);
        }

        uint32_t pc = USER_BASE + entry * INC_PTR;
        auto ax = 0;
        uint32_t bp = 0;
        auto log = true;

#if 0
        if (log) {
            printf("\n---------------- STACK BEGIN <<<< \n");
            printf("AX: %08X BP: %08X SP: %08X\n", ax, bp, sp);
            for (uint32_t i = sp; i < STACK_BASE + PAGE_SIZE; i += 4) {
                printf("[%08X]> %08X\n", i, vmm_get<uint32_t>(i));
            }
            printf("---------------- STACK END >>>>\n\n");
        }
#endif

        auto cycle = 0;
        uint32_t args[6];
        while (true) {
            cycle++;
            auto op = vmm_get(pc); // get next operation code
            pc += INC_PTR;

#if LOG_INS
            assert(op <= EXIT);
            // print debug info
            if (true) {
                printf("%04d> [%08X] %02d %.4s", cycle, pc, op, INS_STRING((ins_t) op).c_str());
                if (op == PUSH)
                    printf(" %08X\n", (uint32_t) ax);
                else if (op <= ADJ)
                    printf(" %08X(%d)\n", vmm_get(pc), vmm_get(pc));
                else
                    printf("\n");
            }
#endif
            switch (op) {
                case IMM: {
                    ax = vmm_get(pc);
                    pc += INC_PTR;
                } /* load immediate value to ax */
                    break;
                case LOAD: {
                    switch (vmm_get(pc)) {
                        case 1:
                            ax = vmm_get<byte>((uint32_t) ax);
                            break;
                        case 2:
                        case 3:
                        case 4:
                            ax = vmm_get((uint32_t)ax);
                            break;
                        default:
                            // NOT SUPPORT LONG TYPE NOW
                            ax = vmm_get((uint32_t)ax);
                            break;
                    }
                    pc += INC_PTR;
                } /* load integer to ax, address in ax */
                    break;
                case SAVE: {
                    switch (vmm_get(pc)) {
                        case 1:
                            vmm_set<byte>((uint32_t)vmm_popstack(sp), (byte) ax);
                            break;
                        case 2:
                        case 3:
                        case 4:
                            vmm_set((uint32_t) vmm_popstack(sp), ax);
                            break;
                        default:
                            // NOT SUPPORT LONG TYPE NOW
                            vmm_set((uint32_t) vmm_popstack(sp), ax);
                            break;
                    }
                    pc += INC_PTR;
                } /* save integer to address, value in ax, address on stack */
                    break;
                case PUSH: {
                    vmm_pushstack(sp, ax);
                } /* push the value of ax onto the stack */
                    break;
                case POP: {
                    ax = vmm_popstack(sp);
                } /* pop the value of ax from the stack */
                    break;
                case JMP: {
                    pc = base + vmm_get(pc) * INC_PTR;
                } /* jump to the address */
                    break;
                case JZ: {
                    pc = ax ? pc + INC_PTR : (base + vmm_get(pc) * INC_PTR);
                } /* jump if ax is zero */
                    break;
                case JNZ: {
                    pc = ax ? (base + vmm_get(pc) * INC_PTR) : pc + INC_PTR;
                } /* jump if ax is zero */
                    break;
                case CALL: {
                    vmm_pushstack(sp, pc + INC_PTR);
                    pc = base + vmm_get(pc) * INC_PTR;
#if 0
                    printf("CALL> PC=%08X\n", pc);
#endif
                } /* call subroutine */
                    /* break;case RET: {pc = (int *)*sp++;} // return from subroutine; */
                    break;
                case ENT: {
                    vmm_pushstack(sp, bp);
                    bp = sp;
                    sp = sp - vmm_get(pc);
                    pc += INC_PTR;
                } /* make new stack frame */
                    break;
                case ADJ: {
                    sp = sp + vmm_get(pc) * INC_PTR;
                    pc += INC_PTR;
                } /* add esp, <size> */
                    break;
                case LEV: {
                    sp = bp;
                    bp = (uint32_t) vmm_popstack(sp);
                    pc = (uint32_t) vmm_popstack(sp);
#if 0
                    printf("RETURN> PC=%08X\n", pc);
#endif
                } /* restore call frame and PC */
                    break;
                case LEA: {
                    ax = bp + vmm_get(pc);
                    pc += INC_PTR;
                } /* load address for arguments. */
                    break;
                case OR:
                    ax = vmm_popstack(sp) | ax;
                    break;
                case XOR:
                    ax = vmm_popstack(sp) ^ ax;
                    break;
                case AND:
                    ax = vmm_popstack(sp) & ax;
                    break;
                case EQ:
                    ax = vmm_popstack(sp) == ax;
                    break;
                case NE:
                    ax = vmm_popstack(sp) != ax;
                    break;
                case LT:
                    ax = vmm_popstack(sp) < ax;
                    break;
                case LE:
                    ax = vmm_popstack(sp) <= ax;
                    break;
                case GT:
                    ax = vmm_popstack(sp) > ax;
                    break;
                case GE:
                    ax = vmm_popstack(sp) >= ax;
                    break;
                case SHL:
                    ax = vmm_popstack(sp) << ax;
                    break;
                case SHR:
                    ax = vmm_popstack(sp) >> ax;
                    break;
                case ADD:
                    ax = vmm_popstack(sp) + ax;
                    break;
                case SUB:
                    ax = vmm_popstack(sp) - ax;
                    break;
                case MUL:
                    ax = vmm_popstack(sp) * ax;
                    break;
                case DIV:
                    ax = vmm_popstack(sp) / ax;
                    break;
                case MOD:
                    ax = vmm_popstack(sp) % ax;
                    break;
                    // --------------------------------------
                case PRTF: {
                    init_args(args, sp, pc);
                    ax = printf(vmm_getstr(args[0]), args[1], args[2], args[3], args[4], args[5]);
                }
                    break;
                case EXIT: {
                    printf("exit(%d)\n", ax);
                    return ax;
                }
                    break;
                case OPEN: {
                    init_args(args, sp, pc);
                    ax = (int) fopen(vmm_getstr(args[0]), "rb");
#if 0
                    printf("OPEN> name=%s fd=%08X\n", vmm_getstr(args[0]), ax);
#endif
                }
                    break;
                case READ: {
                    init_args(args, sp, pc);
#if 0
                    printf("READ> src=%p size=%08X fd=%08X\n", vmm_getstr(args[1]), args[2], args[0]);
#endif
                    ax = (int) fread(vmm_getstr(args[1]), 1, (size_t) args[2], (FILE *) args[0]);
                    if (ax > 0) {
                        rewind((FILE *) args[0]); // 坑：避免重复读取
                        ax = (int) fread(vmm_getstr(args[1]), 1, (size_t) ax, (FILE *) args[0]);
                        vmm_getstr(args[1])[ax] = 0;
#if 0
                        printf("READ> %s\n", vmm_getstr(args[1]));
#endif
                    }
                }
                    break;
                case CLOS: {
                    init_args(args, sp, pc);
                    ax = (int) fclose((FILE *) args[0]);
                }
                    break;
                case MALC: {
                    init_args(args, sp, pc);
                    ax = (int) vmm_malloc((uint32_t) args[0]);
                }
                    break;
                case MSET: {
                    init_args(args, sp, pc);
#if 0
                    printf("MEMSET> PTR=%08X SIZE=%08X VAL=%d\n", (uint32_t)vmm_getstr(args[0]), (uint32_t)args[2], (uint32_t)args[1]);
#endif
                    ax = (int) vmm_memset(args[0], (uint32_t) args[1], (uint32_t) args[2]);
                }
                    break;
                case MCMP: {
                    init_args(args, sp, pc);
                    ax = (int) vmm_memcmp(args[0], args[1], (uint32_t) args[2]);
                }
                    break;
                case TRAC: {
                    init_args(args, sp, pc);
                    ax = log;
                    log = args[0] != 0;
                }
                    break;
                case TRAN: {
                    init_args(args, sp, pc);
                    ax = (uint32_t) vmm_getstr(args[0]);
                }
                    break;
                default: {
                    printf("AX: %08X BP: %08X SP: %08X PC: %08X\n", ax, bp, sp, pc);
                    for (uint32_t i = sp; i < STACK_BASE + PAGE_SIZE; i += 4) {
                        printf("[%08X]> %08X\n", i, vmm_get<uint32_t>(i));
                    }
                    printf("unknown instruction:%d\n", op);
                    error("unknown instruction");
                }
            }

#if LOG_STACK
            if (log) {
                printf("\n---------------- STACK BEGIN <<<< \n");
                printf("AX: %08X BP: %08X SP: %08X PC: %08X\n", ax, bp, sp, pc);
                for (uint32_t i = sp; i < STACK_BASE + PAGE_SIZE; i += 4) {
                    printf("[%08X]> %08X\n", i, vmm_get<uint32_t>(i));
                }
                printf("---------------- STACK END >>>>\n\n");
            }
#endif
        }
        return 0;
    }

    void cvm::error(const string_t &str) {
        std::stringstream ss;
        ss << "VM ERROR: " << str;
        throw cexception(ss.str());
    }
}
