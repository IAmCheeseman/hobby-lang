#include "table.h"

#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"

#define TABLE_MAX_LOAD 0.75

void initTable(struct Table* table) {
  table->count = 0;
  table->capacity = 0;
  table->entries = NULL;
}

void freeTable(struct hl_State* H, struct Table* table) {
  FREE_ARRAY(H, struct TableEntry, table->entries, table->capacity);
  initTable(table);
}

static struct TableEntry* findEntry(
    struct TableEntry* entries, s32 capacity, struct GcString* key) {
  u32 index = key->hash & (capacity - 1);
  struct TableEntry* tombstone = NULL;

  while (true) {
    struct TableEntry* entry = &entries[index];

    if (entry->key == NULL) {
      if (IS_NIL(entry->value)) {
        return tombstone != NULL ? tombstone : entry;
      } else {
        if (tombstone == NULL) {
          tombstone = entry;
        }
      }
    } else if (entry->key == key) {
      return entry;
    }

    index = (index + 1) & (capacity - 1);
  }
}

static void adjustCapacity(struct hl_State* H, struct Table* table, s32 capacity) {
  struct TableEntry* entries = ALLOCATE(H, struct TableEntry, capacity);
  for (s32 i = 0; i < capacity; i++) {
    entries[i].key = NULL;
    entries[i].value = NEW_NIL;
  }

  table->count = 0;
  for (s32 i = 0; i < table->capacity; i++) {
    struct TableEntry* entry = &table->entries[i];
    if (entry->key == NULL) {
      continue;
    }

    struct TableEntry* dst = findEntry(entries, capacity, entry->key);
    dst->key = entry->key;
    dst->value = entry->value;
    table->count++;
  }

  FREE_ARRAY(H, struct TableEntry, table->entries, table->capacity);

  table->entries = entries;
  table->capacity = capacity;
}

bool tableSet(
    struct hl_State* H, struct Table* table, struct GcString* key, Value value) {
  if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
    s32 capacity = GROW_CAPACITY(table->capacity);
    adjustCapacity(H, table, capacity);
  }

  struct TableEntry* entry = findEntry(table->entries, table->capacity, key);
  bool isNew = entry->key == NULL;
  if (isNew && IS_NIL(entry->value)) {
    table->count++;
  }

  entry->key = key;
  entry->value = value;
  return isNew;
}

bool tableGet(
    struct Table* table, struct GcString* key, Value* outValue) {
  if (table->count == 0) {
    return false;
  }

  struct TableEntry* entry = findEntry(table->entries, table->capacity, key);
  if (entry->key == NULL) {
    return false;
  }

  *outValue = entry->value;
  return true;
}

bool tableDelete(struct Table* table, struct GcString* key) {
  if (table->count == 0) {
    return false;
  }

  struct TableEntry* entry = findEntry(table->entries, table->capacity, key);
  if (entry->key == NULL) {
    return false;
  }

  entry->key = NULL;
  entry->value = NEW_BOOL(true);
  return true;
}

struct GcString* tableFindString(
    struct Table* table, const char* chars, s32 length, u32 hash) {
  if (table->count == 0) {
    return NULL;
  }

  u32 index = hash & (table->capacity - 1);
  while (true) {
    struct TableEntry* entry = &table->entries[index];
    if (entry->key == NULL) {
      if (IS_NIL(entry->value)) {
        return NULL;
      }
    } else if (entry->key->length == length
        && entry->key->hash == hash
        && memcmp(entry->key->chars, chars, length) == 0) {
      return entry->key;
    }

    index = (index + 1) & (table->capacity - 1);
  }
}

void tableRemoveUnmarked(struct Table* table) {
  for (s32 i = 0; i < table->capacity; i++) {
    struct TableEntry* entry = &table->entries[i];
    if (entry->key != NULL && !entry->key->obj.isMarked) {
      tableDelete(table, entry->key);
    }
  }
}

void markTable(struct hl_State* H, struct Table* table) {
  for (s32 i = 0; i < table->capacity; i++) {
    struct TableEntry* entry = &table->entries[i];
    markObject(H, (struct GcObj*)entry->key);
    markValue(H, entry->value);
  }
}

void copyTable(struct hl_State* H, struct Table* dest, struct Table* src) {
  for (s32 i = 0; i < src->capacity; i++) {
    struct TableEntry* entry = &src->entries[i];
    if (entry->key != NULL) {
      tableSet(H, dest, entry->key, entry->value);
    }
  }
}
