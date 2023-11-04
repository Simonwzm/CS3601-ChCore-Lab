/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
 * Licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#include <common/util.h>
#include <common/macro.h>
#include <common/kprint.h>
#include <mm/buddy.h>

static struct page *get_buddy_chunk(struct phys_mem_pool *pool,
                                    struct page *chunk)
{
        vaddr_t chunk_addr;
        vaddr_t buddy_chunk_addr;
        int order;

        /* Get the address of the chunk. */
        chunk_addr = (vaddr_t)page_to_virt(chunk);
        order = chunk->order;
        /*
         * Calculate the address of the buddy chunk according to the address
         * relationship between buddies.
         */
        buddy_chunk_addr = chunk_addr
                           ^ (1UL << (order + BUDDY_PAGE_SIZE_ORDER));

        /* Check whether the buddy_chunk_addr belongs to pool. */
        if (buddy_chunk_addr < pool->pool_start_addr) {
                // kinfo("error1 \n");
                return NULL;}
        if ((buddy_chunk_addr + (1 << order) * BUDDY_PAGE_SIZE)
                > (pool->pool_start_addr + pool->pool_mem_size)) {
                // kinfo("error2 \n");
                return NULL;
        }

        return virt_to_page((void *)buddy_chunk_addr);
}

/* The most recursion level of split_chunk is decided by the macro of
 * BUDDY_MAX_ORDER. */
static struct page *split_chunk(struct phys_mem_pool *pool, int order,
                                struct page *chunk)
{
        /* LAB 2 TODO 1 BEGIN */
        /*
         * Hint: Recursively put the buddy of current chunk into
         * a suitable free list.
         */
//     if (chunk->order > order) {
//         int buddy_order = chunk->order - 1;

//         // Split the chunk into two buddies
//         struct page *buddy_chunk = get_buddy_chunk(pool, chunk);
//         buddy_chunk->order = buddy_order;



//         // If buddy_chunk is not already allocated, add it to the free list
//         if (!buddy_chunk->allocated) {
//             struct list_head *free_list = &(pool->free_lists[buddy_order].free_list);
//             list_add(&(buddy_chunk->node), free_list);  // Use node instead of list
//             pool->free_lists[buddy_order].nr_free++;
//         }
        
//     }
//             kinfo("single split ok");
//     return chunk;
        if (chunk->order > order) {
                int buddy_order = chunk->order - 1;

                // Calculate the address of the new buddy chunk
                vaddr_t chunk_addr = (vaddr_t)page_to_virt(chunk);
                vaddr_t buddy_chunk_addr = chunk_addr + (1UL << (buddy_order + BUDDY_PAGE_SIZE_ORDER));

                // Create the new buddy chunk and set its order
                struct page *buddy_chunk = virt_to_page((void *)buddy_chunk_addr);
                buddy_chunk->order = buddy_order;

                // If buddy_chunk is not already allocated, add it to the free list
                if (!buddy_chunk->allocated) {
                        struct list_head *free_list = &(pool->free_lists[buddy_order].free_list);
                        list_add(&(buddy_chunk->node), free_list);  // Use node instead of list
                        pool->free_lists[buddy_order].nr_free++;
                }

        // Reduce the order of the original chunk and recursively split
                chunk->order = buddy_order;
                return split_chunk(pool, order, chunk);
        }
        return chunk;
        /* LAB 2 TODO 1 END */
}

/* The most recursion level of merge_chunk is decided by the macro of
 * BUDDY_MAX_ORDER. */
static struct page *merge_chunk(struct phys_mem_pool *pool, struct page *chunk)
{
        /* LAB 2 TODO 1 BEGIN */
        /*
         * Hint: Recursively merge current chunk with its buddy
         * if possible.
         */
    struct page *buddy_chunk, *merged_chunk = chunk;
    int preorder = chunk->order ;

    while (chunk->order < BUDDY_MAX_ORDER-1 ) {
        // kinfo("temp sign 1 \n");
        buddy_chunk = get_buddy_chunk(pool, chunk);
        // if (chunk->order == 4) {
                // kinfo("order 4 reached");
                // kinfo("buddy allocated: %d \n", buddy_chunk->allocated);
        // }


        
        // If buddy is not free or of different order, break
        // if (buddy_chunk == NULL && chunk->order == 4) {
        //         get_free_mem_size_from_buddy(pool);
        // }

        if (buddy_chunk == NULL || buddy_chunk->allocated || buddy_chunk->order != chunk->order) break;

        // Remove buddy from free list
        
        list_del(&buddy_chunk->node);
        // kinfo("temp sign 2 \n");
        pool->free_lists[chunk->order].nr_free--;


        // Determine which chunk is lower in memory
        merged_chunk = (chunk < buddy_chunk) ? chunk : buddy_chunk;

        // Increase order and continue trying to merge
        merged_chunk->order++;
        chunk = merged_chunk;
    }
            if (chunk->order == 4) {
                // kinfo("order 4 produced");
        }
    
        if (merged_chunk->order == 14) {
                kinfo("merge ok from %d to %d \n", preorder, merged_chunk->order);      
        }


    return merged_chunk;
        /* LAB 2 TODO 1 END */
}

/*
 * The layout of a phys_mem_pool:
 * | page_metadata are (an array of struct page) | alignment pad | usable memory
 * |
 *
 * The usable memory: [pool_start_addr, pool_start_addr + pool_mem_size).
 */
void init_buddy(struct phys_mem_pool *pool, struct page *start_page,
                vaddr_t start_addr, unsigned long page_num)
{
        int order;
        int page_idx;
        struct page *page;

        BUG_ON(lock_init(&pool->buddy_lock) != 0);

        /* Init the physical memory pool. */
        // kinfo("max order is %d \n", BUDDY_MAX_ORDER);
        pool->pool_start_addr = start_addr;
        pool->page_metadata = start_page;
        pool->pool_mem_size = page_num * BUDDY_PAGE_SIZE;
        /* This field is for unit test only. */
        pool->pool_phys_page_num = page_num;
        // kinfo("num of pages: %d \n", page_num);

        /* Init the free lists */
        for (order = 0; order < BUDDY_MAX_ORDER; ++order) {
                pool->free_lists[order].nr_free = 0;
                init_list_head(&(pool->free_lists[order].free_list));
        }

        /* Clear the page_metadata area. */
        memset((char *)start_page, 0, page_num * sizeof(struct page));

        /* Init the page_metadata area. */
        for (page_idx = 0; page_idx < page_num; ++page_idx) {
                page = start_page + page_idx;
                page->allocated = 1;
                page->order = 0;
                page->pool = pool;
        }

        /* Put each physical memory page into the free lists. */
        for (page_idx = 0; page_idx < page_num; ++page_idx) {
                page = start_page + page_idx;
                buddy_free_pages(pool, page);
        }
        kinfo("init ok \n");
        // get_free_mem_size_from_buddy(pool);
}

struct page *buddy_get_pages(struct phys_mem_pool *pool, int order)
{
        int cur_order;
        struct list_head *free_list;
        struct page *page = NULL;

        get_free_mem_size_from_buddy(pool);

        if (unlikely(order >= BUDDY_MAX_ORDER)) {
                kwarn("ChCore does not support allocating such too large "
                      "contious physical memory\n");
                return NULL;
        }

        lock(&pool->buddy_lock);

        /* LAB 2 TODO 1 BEGIN */
        /*
         * Hint: Find a chunk that satisfies the order requirement
         * in the free lists, then split it if necessary.
        */
        for (cur_order = order; cur_order < BUDDY_MAX_ORDER; cur_order++) {
                free_list = &(pool->free_lists[cur_order].free_list);

                if (!list_empty(free_list)) {
                        // Remove the chunk from the free list
                        page = container_of(free_list->next, struct page, node);
                        list_del(&page->node);
                        pool->free_lists[cur_order].nr_free--;

                        // If necessary, split the chunk
                        split_chunk(pool, order, page);
                        page->allocated = 1;
                        page->order = order;
                        break;
                }
        }

        /* LAB 2 TODO 1 END */
out:
        unlock(&pool->buddy_lock);
        return page;
}

void buddy_free_pages(struct phys_mem_pool *pool, struct page *page)
{
        int order;
        struct list_head *free_list;

        lock(&pool->buddy_lock);

        /* LAB 2 TODO 1 BEGIN */
        /*
         * Hint: Merge the chunk with its buddy and put it into
         * a suitable free list.
         */
    // Reset page state
        page->allocated = 0;
        // order = page->order;

        // Try to merge the page with its buddy
        struct page *merged_page = merge_chunk(pool, page);

        // Add the merged page to the free list
        // kwarn("1 \n");
        free_list = &(pool->free_lists[merged_page->order].free_list);
        // kwarn("2 \n");
        list_add(&(merged_page->node), free_list);
        // kwarn("3 \n");
        pool->free_lists[merged_page->order].nr_free++;
        // kwarn("4 \n");
        /* LAB 2 TODO 1 END */
        //     kinfo("buddy free a chunk ok %d \n", merged_page->order);
        unlock(&pool->buddy_lock);
}

void *page_to_virt(struct page *page)
{
        vaddr_t addr;
        struct phys_mem_pool *pool = page->pool;

        BUG_ON(pool == NULL);

        /* page_idx * BUDDY_PAGE_SIZE + start_addr */
        addr = (page - pool->page_metadata) * BUDDY_PAGE_SIZE
               + pool->pool_start_addr;
        return (void *)addr;
}

struct page *virt_to_page(void *ptr)
{
        struct page *page;
        struct phys_mem_pool *pool = NULL;
        vaddr_t addr = (vaddr_t)ptr;
        int i;

        /* Find the corresponding physical memory pool. */
        for (i = 0; i < physmem_map_num; ++i) {
                if (addr >= global_mem[i].pool_start_addr
                    && addr < global_mem[i].pool_start_addr
                                       + global_mem[i].pool_mem_size) {
                        pool = &global_mem[i];
                        break;
                }
        }

        if (pool == NULL) {
                kdebug("invalid pool in %s", __func__);
                return NULL;
        }

        page = pool->page_metadata
               + (((vaddr_t)addr - pool->pool_start_addr) / BUDDY_PAGE_SIZE);
        return page;
}

unsigned long get_free_mem_size_from_buddy(struct phys_mem_pool *pool)
{
        int order;
        struct free_list *list;
        unsigned long current_order_size;
        unsigned long total_size = 0;

        for (order = 0; order < BUDDY_MAX_ORDER; order++) {
                /* 2^order * 4K */
                current_order_size = BUDDY_PAGE_SIZE * (1 << order);
                list = pool->free_lists + order;
                total_size += list->nr_free * current_order_size;

                /* debug : print info about current order */
                kdebug("buddy memory chunk order: %d, size: 0x%lx, num: %d\n",
                       order,
                       current_order_size,
                       list->nr_free);
        }
        return total_size;
}
