#include "calculator_storage.h"

#include "hardware/flash.h"
#include "hardware/regs/addressmap.h"
#include "hardware/sync.h"

#include <string.h>

#define STORAGE_SLOT_COUNT 2u
#define STORAGE_BASE_OFFSET \
    (PICO_FLASH_SIZE_BYTES - STORAGE_SLOT_COUNT * FLASH_SECTOR_SIZE)

_Static_assert(CALCULATOR_PERSISTENCE_RECORD_CAPACITY <= FLASH_SECTOR_SIZE,
               "persistence record must fit in one flash sector");
_Static_assert(CALCULATOR_PERSISTENCE_RECORD_CAPACITY % FLASH_PAGE_SIZE == 0,
               "persistence record must contain complete flash pages");

static int active_slot = -1;
static uint32_t active_sequence;
_Alignas(4) static uint8_t
    record_buffer[CALCULATOR_PERSISTENCE_RECORD_CAPACITY];

static const uint8_t *slot_data(size_t slot) {
    uintptr_t address = XIP_BASE + STORAGE_BASE_OFFSET +
                        slot * FLASH_SECTOR_SIZE;
    return (const uint8_t *)address;
}

calculator_storage_load_status_t calculator_storage_load(
    calculator_persisted_state_t *state) {
    size_t selected_slot = 0;
    calculator_persistence_status_t status = calculator_persistence_select(
        slot_data(0), FLASH_SECTOR_SIZE, slot_data(1), FLASH_SECTOR_SIZE,
        state, &active_sequence, &selected_slot);
    if (status == CALCULATOR_PERSISTENCE_VALID) {
        active_slot = (int)selected_slot;
        return CALCULATOR_STORAGE_LOADED;
    }

    active_slot = -1;
    active_sequence = 0;
    calculator_persistence_defaults(state);
    return status == CALCULATOR_PERSISTENCE_EMPTY
        ? CALCULATOR_STORAGE_DEFAULTS : CALCULATOR_STORAGE_RECOVERED;
}

calculator_storage_save_status_t calculator_storage_save(
    const calculator_persisted_state_t *state) {
    size_t record_size = 0;
    memset(record_buffer, 0xff, sizeof record_buffer);
    if (!calculator_persistence_encode(state, active_sequence, record_buffer,
                                       sizeof record_buffer, &record_size)) {
        return CALCULATOR_STORAGE_ERROR;
    }
    if (active_slot >= 0 &&
        memcmp(record_buffer, slot_data((size_t)active_slot), record_size) == 0) {
        return CALCULATOR_STORAGE_UNCHANGED;
    }

    uint32_t next_sequence = active_sequence + 1u;
    memset(record_buffer, 0xff, sizeof record_buffer);
    if (!calculator_persistence_encode(state, next_sequence, record_buffer,
                                       sizeof record_buffer, &record_size)) {
        return CALCULATOR_STORAGE_ERROR;
    }
    size_t target_slot = active_slot < 0 ? 0u : 1u - (size_t)active_slot;
    uint32_t target_offset = STORAGE_BASE_OFFSET +
                             (uint32_t)target_slot * FLASH_SECTOR_SIZE;

    uint32_t interrupts = save_and_disable_interrupts();
    flash_range_erase(target_offset, FLASH_SECTOR_SIZE);
    flash_range_program(target_offset, record_buffer,
                        CALCULATOR_PERSISTENCE_RECORD_CAPACITY);
    restore_interrupts(interrupts);

    calculator_persisted_state_t verified;
    uint32_t verified_sequence = 0;
    if (calculator_persistence_decode(slot_data(target_slot), FLASH_SECTOR_SIZE,
                                      &verified, &verified_sequence) !=
            CALCULATOR_PERSISTENCE_VALID ||
        verified_sequence != next_sequence) {
        return CALCULATOR_STORAGE_ERROR;
    }
    active_slot = (int)target_slot;
    active_sequence = next_sequence;
    return CALCULATOR_STORAGE_WRITTEN;
}
