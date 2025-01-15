#include <stddef.h>  // For size_t

// Define the total heap size
#define HEAP_SIZE  (64 * 1024)

// Create a static array in RAM to serve as the heap space.
static char heap_area[HEAP_SIZE];

/*
 * Each memory block will have a header containing metadata:
 * - size: size of the memory portion available for allocation (excluding the header).
 * - next: pointer to the next block in the free list.
 * - free: flag indicating whether this block is free (1) or allocated (0).
 */
typedef struct block_header {
    size_t size;                 // Size of the usable memory within this block.
    struct block_header *next;   // Pointer to the next block in the free list.
    int free;                    // 1 if block is free, 0 if it's allocated.
} block_header_t;

// Macro to easily get the size of the block header.
#define HEADER_SIZE sizeof(block_header_t)

// A pointer to the first block of our free list.
static block_header_t *free_list = NULL;

/*
 * Initialize the heap by creating a single large free block that encompasses
 * the entire heap area. This should be called before any allocations are made.
 */
void init_heap() {
    // Set free_list to the beginning of our heap area.
    free_list = (block_header_t *)heap_area;
    // Size of this block is the total heap minus the header.
    free_list->size = HEAP_SIZE - HEADER_SIZE;
    // There are no other blocks yet, so next is set to NULL.
    free_list->next = NULL;
    // Mark this block as free.
    free_list->free = 1;
}

/*
 * Allocate a block of memory of at least 'size' bytes.
 */
void *malloc(size_t size) {
    block_header_t *current = free_list;
    block_header_t *prev = NULL;

    // Align the requested size to 8 bytes (for ARM architecture alignment).
    size = (size + 7) & ~7;

    // Traverse the free list to find a block that can accommodate 'size'.
    while (current) {
        // Check if the current block is free and large enough.
        if (current->free && current->size >= size) {
            /*
             * If the current block is significantly larger than needed, split it:
             * - Create a new block for the remainder after allocation.
             * - Only split if the remainder can at least hold a header and some data.
             */
            if (current->size >= size + HEADER_SIZE + 8) {
                // Calculate the address for the new block header, after the allocated space.
                block_header_t *new_block = (block_header_t *)((char *)current + HEADER_SIZE + size);

                // Set the new block's size to the leftover size.
                new_block->size = current->size - size - HEADER_SIZE;
                // Insert the new block into the free list after the current block.
                new_block->next = current->next;
                // Mark the new block as free.
                new_block->free = 1;

                // Adjust the current block's size to exactly what was requested.
                current->size = size;
                // Link the current block to the new block.
                current->next = new_block;
            }

            // Mark the current block as allocated.
            current->free = 0;

            // Return a pointer to the usable memory area, which is just past the header.
            return (void *)((char *)current + HEADER_SIZE);
        }

        // Move to the next block in the list.
        prev = current;
        current = current->next;
    }

    // If we reach here, no suitable free block was found. Return NULL.
    return NULL;
}

/*
 * Free a previously allocated block of memory pointed to by 'ptr'.
 */
void free(void *ptr) {
    // If the pointer is NULL, there's nothing to free.
    if (!ptr) return;

    // Get a pointer to the block header by subtracting the header size.
    block_header_t *block = (block_header_t *)((char *)ptr - HEADER_SIZE);

    // Mark the block as free.
    block->free = 1;

    // Coalesce adjacent free blocks to avoid fragmentation.
    // Start from the beginning of the free list.
    block_header_t *current = free_list;
    while (current) {
        // Check if the current block and the next block are both free.
        if (current->free && current->next && current->next->free) {
            /*
             * Merge the current block with the next block:
             * - Increase the size of the current block to include the next block and its header.
             * - Link the current block to the block after the next block.
             */
            current->size += HEADER_SIZE + current->next->size;
            current->next = current->next->next;
            // After merging, do not advance 'current' pointer since there could be more adjacent free blocks.
        } else {
            // Move to the next block in the list.
            current = current->next;
        }
    }
}
