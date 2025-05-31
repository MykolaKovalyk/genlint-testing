#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// Ring buffer entry structure
typedef struct {
  uint32_t write_index; // Index when this entry was written
  uint8_t data[];       // Flexible array member for actual data
} rb_entry_t;

// Ring buffer structure
typedef struct {
  uint8_t *buffer;      // Raw buffer memory
  uint32_t size;        // Total buffer size in bytes
  uint32_t entry_size;  // Size of each entry (including header)
  uint32_t num_entries; // Number of entries that fit in buffer
  uint32_t write_index; // Current write index (monotonically increasing)
} ringbuffer_t;

// Return codes
typedef enum {
  RB_OK = 0,
  RB_ERROR = -1,
  RB_OVERRUN = -2,
  RB_INVALID_PARAM = -3
} rb_result_t;

/**
 * Initialize ring buffer
 * @param rb Ring buffer structure
 * @param buffer Memory buffer to use
 * @param buffer_size Total size of buffer in bytes
 * @param data_size Size of data portion of each entry
 * @return RB_OK on success, error code otherwise
 */
rb_result_t rb_init(ringbuffer_t *rb, uint8_t *buffer, uint32_t buffer_size,
                    uint32_t data_size);

/**
 * Write data to ring buffer
 * @param rb Ring buffer structure
 * @param data Data to write
 * @param data_size Size of data to write
 * @return RB_OK on success, error code otherwise
 */
rb_result_t rb_write(ringbuffer_t *rb, const void *data, uint32_t data_size);

/**
 * Read data from ring buffer using reader's index
 * @param rb Ring buffer structure
 * @param reader_index Reader's current index
 * @param data Buffer to store read data
 * @param data_size Size of data buffer
 * @param actual_index Pointer to store the actual index of read entry (can be
 * NULL)
 * @return RB_OK on success, RB_OVERRUN if data was overwritten, error code
 * otherwise
 */
rb_result_t rb_read(ringbuffer_t *rb, uint32_t reader_index, void *data,
                    uint32_t data_size, uint32_t *actual_index);

/**
 * Get current write index (for readers to track their position)
 * @param rb Ring buffer structure
 * @return Current write index
 */
uint32_t rb_get_write_index(const ringbuffer_t *rb);

/**
 * Get number of entries available for reading from a given reader index
 * @param rb Ring buffer structure
 * @param reader_index Reader's current index
 * @return Number of entries available
 */
uint32_t rb_available(const ringbuffer_t *rb, uint32_t reader_index);

/**
 * Check if the ring buffer is valid
 * @param rb Ring buffer structure
 * @return true if valid, false otherwise
 */
bool rb_is_valid(const ringbuffer_t *rb);

/**
 * Check if a specific entry has been overrun
 * @param rb Ring buffer structure
 * @param reader_index Index to check
 * @return true if overrun detected, false otherwise
 */
bool rb_is_overrun(const ringbuffer_t *rb, uint32_t reader_index);

#endif // RINGBUFFER_H