/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_UTIL_HASH_TABLE_H_
#define TB_UTIL_HASH_TABLE_H_

#include <cassert>
#include <cstdint>

#include "tb/config.h"

namespace tb {
namespace util {

/** HashTable is a minimal hash table, for hashing anything using a uint32
* key. */
class HashTable {
 public:
  HashTable();
  virtual ~HashTable();

  /** Remove all items without deleting the content. */
  void RemoveAll() { RemoveAll(false); }

  /** Remove all items and delete the content.
  This requires HashTable to be subclassed and implementing
  DeleteContent.
  You would typically do this by using HashTableOf or
  HashTableAutoDeleteOf. */
  void DeleteAll() { RemoveAll(true); }

  /** Get the content for the given key, or nullptr if not found. */
  void* Get(uint32_t key) const;

  /** Add content with the given key.
  Returns false if out of memory. */
  void Add(uint32_t key, void* content);

  /** Remove the content with the given key. */
  void* Remove(uint32_t key);

  /** Delete the content with the given key. */
  void Delete(uint32_t key);

  /** Rehash the table so use the given number of buckets.
  Returns false if out of memory. */
  void Rehash(uint32_t num_buckets);

  /** Return true if the hashtable itself think it's time to rehash. */
  bool NeedRehash() const;

  /** Get the number of buckets the hashtable itself thinks is suitable for
  the current number of items. */
  uint32_t GetSuitableBucketsCount() const;

#ifdef TB_RUNTIME_DEBUG_INFO
  /** Print out some debug info about the hash table. */
  void Debug();
#endif

 protected:
  /** Delete the content of a item. This is called if calling DeleteAll, and
  must be
  implemented in a subclass that knows about the content type. */
  virtual void DeleteContent(void* content) {
    assert(!"You need to subclass and implement!");
  }

 private:
  friend class HashTableIterator;

  void RemoveAll(bool delete_content);

  struct ITEM {
    uint32_t key;
    ITEM* next;
    void* content;
  }** m_buckets = nullptr;
  uint32_t m_num_buckets = 0;
  uint32_t m_num_items = 0;
};

/** HashTableIterator is a iterator for stepping through all content stored in
* a HashTable. */
// FIX: make it safe in case the current item is removed from the hashtable
class HashTableIterator {
 public:
  HashTableIterator(HashTable* hash_table);
  void* GetNextContent();

 private:
  HashTable* m_hash_table;
  uint32_t m_current_bucket;
  HashTable::ITEM* m_current_item;
};

/** HashTableIteratorOf is a HashTableIterator which auto cast to the class
* type. */
template <class T>
class HashTableIteratorOf : private HashTableIterator {
 public:
  HashTableIteratorOf(HashTable* hash_table) : HashTableIterator(hash_table) {}
  T* GetNextContent() { return (T*)HashTableIterator::GetNextContent(); }
};

/** HashTableOf is a HashTable with the given class type as content. */
template <class T>
class HashTableOf : public HashTable {
  // FIX: Don't do public inheritance! Either inherit privately and forward, or
  // use a private member backend!
 public:
  T* Get(uint32_t key) const { return (T*)HashTable::Get(key); }

 protected:
  virtual void DeleteContent(void* content) { delete (T*)content; }
};

/** HashTableOf is a HashTable with the given class type as content.
It will delete all content automaticallt on destruction. */
template <class T>
class HashTableAutoDeleteOf : public HashTable {
 public:
  ~HashTableAutoDeleteOf() { DeleteAll(); }

  T* Get(uint32_t key) const { return (T*)HashTable::Get(key); }

 protected:
  virtual void DeleteContent(void* content) { delete (T*)content; }
};

}  // namespace util
}  // namespace tb

#endif  // TB_UTIL_HASH_TABLE_H_
