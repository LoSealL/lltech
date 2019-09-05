/********************************************************************
Copyright 2017 Intel Corp. All Rights Reserved.
Description : A simple memory pool
Author      : Wenyi Tang
Email       : wenyi.tang@intel.com
Created     : Feb. 3rd, 2017
Mod         : Date      Author

changelog
********************************************************************/
#ifndef LL_CODEC_MFXVR_UTILITY_SIMPLEPOOL_H_
#define LL_CODEC_MFXVR_UTILITY_SIMPLEPOOL_H_

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <list>
#include <mutex>


class SimplePool {
 public:
  /**
   *  \brief create a memory pool with associated size
   *
   *  \param [in] cap the reserved memory size
   */
  explicit SimplePool(size_t cap) : m_capacity(cap) {
    m_pool = malloc(cap);
    if (!m_pool) throw std::overflow_error("malloc failed!");
    m_table.clear();
  }

  virtual ~SimplePool() {
    if (m_pool) free(m_pool);
    m_pool = nullptr;
    m_table.clear();
  }

  /**
   * \brief find an available space for user.
   * \param [in] len: requested space in bytes
   * \return a valid pointer if found enough space, otherwise returns null.
   */
  template <class T = void *>
  T Alloc(size_t len) {
    std::lock_guard<std::mutex> locker(m_lock);
    table _table;
    _table.len = len;
    if (m_table.empty()) {
      // the pool is empty
      _table.head = m_pool;
      _table.available = false;
      m_table.emplace_back(_table);
      return static_cast<T>(m_pool);
    }
    auto _it = m_table.begin();
    if ((size_t)_it->head - (size_t)m_pool > len) {
      _table.head = m_pool;
      _table.available = false;
      m_table.emplace_front(_table);
      return static_cast<T>(m_pool);
    } else {
      auto cur_pos = _it;
      while (++_it != m_table.end()) {
        // check if there is enough space between two blocks
        if ((size_t)_it->head - (size_t)cur_pos->head - cur_pos->len >= len) {
          _table.head = static_cast<char *>(cur_pos->head) + cur_pos->len;
          _table.available = false;
          m_table.emplace(_it, _table);
          return static_cast<T>(_table.head);
        }
        cur_pos = _it;
      }
      // check the last remaining space in the pool
      if ((size_t)m_pool + m_capacity <
          len + (size_t)cur_pos->head + cur_pos->len) {
        // not enough space
        return nullptr;
      } else {
        _table.head = static_cast<char *>(cur_pos->head) + cur_pos->len;
        _table.available = false;
        m_table.emplace_back(_table);
        return static_cast<T>(_table.head);
      }
    }
    return nullptr;
  }

  /**
   * \brief delete the alloced memory in the table, release to the pool.
   * \param [in] pos: the pointer to the alloced memory space to delete
   */
  void Dealloc(void *pos) {
    std::lock_guard<std::mutex> locker(m_lock);
    if (!pos) return;
    for (auto _t = m_table.begin(); _t != m_table.end(); ++_t) {
      if (_t->head == pos) {
        m_table.erase(_t);
        break;
      }
    }
  }

  // Clear the mem-map table
  void Reset() { m_table.clear(); }

 private:
  void *m_pool;       ///< the raw memory section
  size_t m_capacity;  ///< the pre-allocated size for pool
  struct table {
    void *head;
    size_t len;
    bool available;
  };
  std::list<table> m_table;  ///< a memory map table
  std::mutex m_lock;
};

#endif  // LL_CODEC_MFXVR_UTILITY_SIMPLEPOOL_H_
