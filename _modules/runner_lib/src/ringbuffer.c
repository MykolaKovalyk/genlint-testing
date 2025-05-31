
// Implementation
#include "ringbuffer.h"

rb_result_t rb_init(ringbuffer_t *rb, uint8_t *buffer, uint32_t buffer_size,
                    uint32_t data_size) {
  if (!rb || !buffer || buffer_size == 0 || data_size == 0) {
    return RB_INVALID_PARAM;
  }

  rb->buffer = buffer;
  rb->size = buffer_size;
  rb->entry_size = sizeof(rb_entry_t) + data_size;
  rb->num_entries = buffer_size / rb->entry_size;
  rb->write_index = 0;

  // Check if we have at least one entry
  if (rb->num_entries == 0) {
    return RB_INVALID_PARAM;
  }

  // Clear the buffer
  memset(buffer, 0, buffer_size);

  return RB_OK;
}

rb_result_t rb_write(ringbuffer_t *rb, const void *data, uint32_t data_size) {
  if (!rb || !data || data_size == 0) {
    return RB_INVALID_PARAM;
  }

  // Check if data fits in entry
  if (data_size > (rb->entry_size - sizeof(rb_entry_t))) {
    return RB_INVALID_PARAM;
  }

  // Calculate buffer position
  uint32_t pos = (rb->write_index % rb->num_entries) * rb->entry_size;
  rb_entry_t *entry = (rb_entry_t *)(rb->buffer + pos);

  // Write entry header
  entry->write_index = rb->write_index;

  // Write data
  memcpy(entry->data, data, data_size);

  // Increment write index
  rb->write_index++;

  return RB_OK;
}

rb_result_t rb_read(ringbuffer_t *rb, uint32_t reader_index, void *data,
                    uint32_t data_size, uint32_t *actual_index) {
  if (!rb || !data || data_size == 0) {
    return RB_INVALID_PARAM;
  }

  // Check if data buffer is large enough
  if (data_size > (rb->entry_size - sizeof(rb_entry_t))) {
    return RB_INVALID_PARAM;
  }

  // Calculate buffer position
  uint32_t pos = (reader_index % rb->num_entries) * rb->entry_size;
  rb_entry_t *entry = (rb_entry_t *)(rb->buffer + pos);

  // Store actual index if requested
  if (actual_index) {
    *actual_index = entry->write_index;
  }

  // Check for overrun by comparing stored write index with expected
  if (entry->write_index != reader_index) {
    // Copy data anyway (it might still be valid)
    memcpy(data, entry->data, data_size);
    return RB_OVERRUN;
  }

  // Copy data
  memcpy(data, entry->data, data_size);

  return RB_OK;
}

uint32_t rb_get_write_index(const ringbuffer_t *rb) {
  if (!rb) {
    return 0;
  }
  return rb->write_index;
}

uint32_t rb_available(const ringbuffer_t *rb, uint32_t reader_index) {
  if (!rb) {
    return 0;
  }

  if (rb->write_index >= reader_index) {
    uint32_t available = rb->write_index - reader_index;
    // Limit to buffer capacity
    return (available > rb->num_entries) ? rb->num_entries : available;
  }

  return 0;
}

bool rb_is_valid(const ringbuffer_t *rb) {
  if (!rb) {
    return false;
  }

  if (rb->buffer == NULL || rb->size == 0 || rb->entry_size == 0 ||
      rb->num_entries == 0) {
    return false;
  }

  return true;
}

bool rb_is_overrun(const ringbuffer_t *rb, uint32_t reader_index) {
  if (!rb) {
    return false;
  }

  // If reader is too far behind, it's definitely overrun
  if (rb->write_index >= reader_index + rb->num_entries) {
    return true;
  }

  // Check the actual entry at that position
  uint32_t pos = (reader_index % rb->num_entries) * rb->entry_size;
  rb_entry_t *entry = (rb_entry_t *)(rb->buffer + pos);

  return (entry->write_index != reader_index);
}