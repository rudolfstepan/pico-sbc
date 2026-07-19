#include "calculator_storage.h"

#include "hardware/flash.h"
#include "hardware/regs/addressmap.h"
#include "hardware/sync.h"

#include <string.h>

#define STORAGE_SLOT_COUNT 2u
#define STORAGE_SLOT_SIZE CALCULATOR_PERSISTENCE_RECORD_CAPACITY
#define STORAGE_BASE_OFFSET \
    (PICO_FLASH_SIZE_BYTES - STORAGE_SLOT_COUNT * STORAGE_SLOT_SIZE)
#define LEGACY_SLOT_SIZE FLASH_SECTOR_SIZE
#define LEGACY_BASE_OFFSET \
    (PICO_FLASH_SIZE_BYTES - STORAGE_SLOT_COUNT * LEGACY_SLOT_SIZE)

_Static_assert(CALCULATOR_PERSISTENCE_RECORD_CAPACITY % FLASH_SECTOR_SIZE == 0,
               "persistence record must contain complete flash sectors");
_Static_assert(CALCULATOR_PERSISTENCE_RECORD_CAPACITY % FLASH_PAGE_SIZE == 0,
               "persistence record must contain complete flash pages");

static int active_slot = -1;
static uint32_t active_sequence;
_Alignas(4) static uint8_t
    record_buffer[CALCULATOR_PERSISTENCE_RECORD_CAPACITY];
static calculator_persisted_state_t verified_state;

static const uint8_t *slot_data(size_t slot) {
    uintptr_t address = XIP_BASE + STORAGE_BASE_OFFSET +
                        slot * STORAGE_SLOT_SIZE;
    return (const uint8_t *)address;
}

static const uint8_t *legacy_slot_data(size_t slot) {
    uintptr_t address = XIP_BASE + LEGACY_BASE_OFFSET +
                        slot * LEGACY_SLOT_SIZE;
    return (const uint8_t *)address;
}

static uint16_t record_version(const uint8_t *record) {
    return (uint16_t)record[4] | (uint16_t)record[5] << 8;
}

static bool load_legacy_state(calculator_persisted_state_t *state) {
    size_t selected_slot = 0;
    calculator_persistence_status_t status = calculator_persistence_select(
        legacy_slot_data(0), LEGACY_SLOT_SIZE,
        legacy_slot_data(1), LEGACY_SLOT_SIZE,
        state, &active_sequence, &selected_slot);
    if (status != CALCULATOR_PERSISTENCE_VALID) return false;

    /* Write the first format-5 record into the non-overlapping new slot. */
    active_slot = 1;
    return true;
}

calculator_storage_load_status_t calculator_storage_load(
    calculator_persisted_state_t *state) {
    size_t selected_slot = 0;
    calculator_persistence_status_t status = calculator_persistence_select(
        slot_data(0), STORAGE_SLOT_SIZE, slot_data(1), STORAGE_SLOT_SIZE,
        state, &active_sequence, &selected_slot);
    if (status == CALCULATOR_PERSISTENCE_VALID) {
        if (record_version(slot_data(selected_slot)) <
                CALCULATOR_PERSISTENCE_VERSION &&
            load_legacy_state(state)) {
            return CALCULATOR_STORAGE_LOADED;
        }
        active_slot = (int)selected_slot;
        return CALCULATOR_STORAGE_LOADED;
    }
    if (load_legacy_state(state)) return CALCULATOR_STORAGE_LOADED;

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
                             (uint32_t)target_slot * STORAGE_SLOT_SIZE;

    uint32_t interrupts = save_and_disable_interrupts();
    flash_range_erase(target_offset, STORAGE_SLOT_SIZE);
    flash_range_program(target_offset, record_buffer,
                        CALCULATOR_PERSISTENCE_RECORD_CAPACITY);
    restore_interrupts(interrupts);

    uint32_t verified_sequence = 0;
    if (calculator_persistence_decode(slot_data(target_slot), STORAGE_SLOT_SIZE,
                                      &verified_state, &verified_sequence) !=
            CALCULATOR_PERSISTENCE_VALID ||
        verified_sequence != next_sequence) {
        return CALCULATOR_STORAGE_ERROR;
    }
    active_slot = (int)target_slot;
    active_sequence = next_sequence;
    return CALCULATOR_STORAGE_WRITTEN;
}
