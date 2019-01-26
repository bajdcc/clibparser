//
// Project: clibparser
// Created by bajdcc
//

#include <algorithm>
#include "cmem.h"
#include "cvm.h"
#include "cexception.h"

namespace clib {

    cmem::cmem(imem *m) : m(m) { }

    static uint32_t size_align(uint32_t size) {
        return (size + 3U) & ~3U;
    }

    uint32_t cmem::alloc(uint32_t size) {
        size = size_align(size);
        if (available_size < size) {
            return new_page_all(size);
        }
        for (auto &f : memory_free) {
            if (f.second >= size) {
                auto free_addr = f.first;
                auto free_size = f.second;
                available_size -= size;
                memory_free.erase(free_addr);
                memory_used.insert(std::make_pair(free_addr, size));
                if (size < free_size)
                    memory_free.insert(std::make_pair(free_addr + size, size - free_size));
                return free_addr;
            }
        }
        return new_page_all(size);
    }

    uint32_t cmem::free(uint32_t addr) {
        auto f = memory_used.find(addr);
        if (f == memory_used.end()) {
            return 0;
        }
        auto used_addr = f->first;
        auto used_size = f->second;
        available_size += used_size;
        auto prev_flag = true;
        auto prev_addr = 0U;
        auto prev_size = 0U;
        auto next_flag = true;
        auto next_addr = 0U;
        auto next_size = 0U;
        if (f != memory_used.begin()) {
            f--;
            if (f->first + f->second == used_addr) {
                prev_flag = false;
                prev_addr = f->first;
                prev_size = f->second;
            }
            f++;
        }
        if (prev_flag) {
            if (f == memory_used.begin()) {
                auto first = memory_free.begin();
                if (first->first + first->second == used_addr) {
                    prev_addr = first->first;
                    prev_size = first->second;
                } else {
                    prev_flag = false;
                }
            } else {
                auto f1 = memory_free.upper_bound(prev_addr);
                if (f1 == memory_free.end()) {
                    prev_flag = false;
                } else {
                    prev_addr = f1->first;
                    prev_size = f1->second;
                }
            }
        }
        if (f != memory_used.end()) {
            f++;
            if (used_addr + used_size == f->first) {
                next_flag = false;
            }
            f--;
        }
        if (next_flag) {
            auto f2 = memory_free.lower_bound(used_addr + used_size);
            if (f2 == memory_free.end()) {
                next_flag = false;
            } else {
                next_addr = f2->first;
                next_size = f2->second;
            }
        }
        if (prev_flag) {
            if (next_flag) {
                // [prev:free] [current:free] [next:free]
                memory_free.erase(prev_addr);
                memory_used.erase(used_addr);
                memory_free.erase(next_addr);
                memory_free.insert(std::make_pair(prev_addr, prev_size + used_size + next_size));
            } else {
                // [prev:free] [current:free] [next:used]
                memory_free.erase(prev_addr);
                memory_used.erase(used_addr);
                memory_free.insert(std::make_pair(prev_addr, prev_size + used_size));
            }
        } else {
            if (next_size) {
                // [prev:used] [current:free] [next:free]
                memory_used.erase(used_addr);
                memory_free.erase(next_addr);
                memory_free.insert(std::make_pair(used_addr, used_size + next_size));
            } else {
                // [prev:used] [current:free] [next:used]
                memory_used.erase(used_addr);
                memory_free.insert(std::make_pair(used_addr, used_size));
            }
        }
        return used_size;
    }

    int cmem::page_size() const {
        return (int) memory.size();
    }

    uint32_t cmem::new_page(uint32_t size) {
        auto id = memory.size();
        while (size > 0) {
            new_page_single();
            if (size < PAGE_SIZE)
                break;
            size -= PAGE_SIZE;
        }
        return id * PAGE_SIZE;
    }

    uint32_t cmem::new_page_single() {
        if (memory.size() >= MAX_PAGE_PER_PROCESS) {
            error("exceed max page per process");
        }
        available_size += PAGE_SIZE;
        memory.emplace_back();
        memory.back().resize(PAGE_SIZE * 2);
        auto page_addr = PAGE_ALIGN_UP((uint32) memory.back().data());
        memory_page.push_back(page_addr);
        auto id = memory.size() - 1;
        m->map_page(page_addr, id);
        return id * PAGE_SIZE;
    }

    uint32_t cmem::new_page_all(uint32_t size) {
        auto page = new_page(size);
        available_size -= size;
        memory_used.insert(std::make_pair(page, size));
        if (OFFSET_INDEX(size)) {
            memory_free.insert(std::make_pair(page + size, page + PAGE_SIZE - OFFSET_INDEX(size)));
        }
        return page;
    }

    void cmem::copy_from(const cmem &mem) {
        available_size = mem.available_size;
        m = mem.m;
        memory = mem.memory;
        for (uint32_t i = 0; i < memory.size(); ++i) {
            memory_page.push_back(PAGE_ALIGN_UP((uint32_t) memory[i].data()));
            m->map_page(memory_page.back(), i);
        }
        memory_free = mem.memory_free;
        memory_used = mem.memory_used;
    }

    void cmem::error(const string_t &str) {
        throw cexception(ex_mem, str);
    }
}