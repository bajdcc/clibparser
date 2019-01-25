//
// Project: clibparser
// Created by bajdcc
//

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
                if (size > free_size)
                    memory_free.insert(std::make_pair(free_addr + size, size - free_size));
                return free_addr;
            }
        }
        return new_page_all(size);
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