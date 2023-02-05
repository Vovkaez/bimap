#pragma once
#include <cassert>
#include <functional>
#include <random>

template <typename, typename, typename, typename>
struct bimap;

namespace details {

struct tree_element_base {
  tree_element_base() noexcept = default;

  tree_element_base* prev() const noexcept;
  tree_element_base* next() const noexcept;

  tree_element_base* minimum() noexcept;
  tree_element_base* maximum() noexcept;

  void swap(tree_element_base& other) noexcept;

  void set_left(tree_element_base* new_left) noexcept;
  void set_right(tree_element_base* new_right) noexcept;

  template <typename, typename, typename, typename>
  friend struct intrusive_bst;

private:
  tree_element_base* parent{nullptr};
  tree_element_base* left{nullptr};
  tree_element_base* right{nullptr};
  unsigned heap_key = 0;

  void fix_links() noexcept;
};

template <typename Tag>
struct tree_element : tree_element_base {};

template <typename T, typename Compare, typename Get, typename Tag>
struct intrusive_bst : Compare {
  using node_t = tree_element<Tag>;

  intrusive_bst(node_t* sentinel, Compare compare)
      : Compare(std::move(compare)), sentinel(sentinel), eng(42),
        distrib(std::numeric_limits<unsigned>::min(),
                std::numeric_limits<unsigned>::max()) {}

  node_t* lower_bound(T const& val) const noexcept {
    if (sentinel->left == nullptr) {
      return nullptr;
    }

    tree_element_base* cur = sentinel->left;
    while (true) {
      T& root_val = Get::get_value(cur);
      tree_element_base* next = compare(root_val, val) ? cur->right : cur->left;
      if (next == nullptr) {
        break;
      }
      cur = next;
    }
    return downcast((compare(Get::get_value(cur), val)) ? cur->next() : cur);
  }

  void insert(node_t* cur) noexcept {
    cur->heap_key = distrib(eng);
    if (sentinel->left == nullptr) {
      sentinel->set_left(cur);
      return;
    }
    tree_element_base* root = sentinel->left;
    auto [left, right] = split(root, Get::get_value(cur));
    sentinel->set_left(merge(left, merge(cur, right)));
  }

  void erase(node_t* cur) noexcept {
    tree_element_base*& parent_ptr =
        (cur->parent->left == cur) ? cur->parent->left : cur->parent->right;
    parent_ptr = merge(cur->left, cur->right);
    cur->parent->fix_links();
  }

private:
  tree_element_base* sentinel;
  std::default_random_engine eng;
  std::uniform_int_distribution<unsigned> distrib;

  static node_t* downcast(tree_element_base* ptr) noexcept {
    return static_cast<node_t*>(ptr);
  }

  Compare& comparator() {
    return *this;
  }

  bool compare(T const& x, T const& y) const {
    return Compare::operator()(x, y);
  }

  tree_element_base* merge(tree_element_base* left,
                           tree_element_base* right) noexcept {
    if (left == nullptr) {
      return right;
    }
    if (right == nullptr) {
      return left;
    }
    if (left->heap_key > right->heap_key) {
      left->set_right(merge(left->right, right));
      return left;
    } else {
      right->set_left(merge(left, right->left));
      return right;
    }
  }

  std::pair<tree_element_base*, tree_element_base*>
  split(tree_element_base* node, T const& val) {
    if (node == nullptr) {
      return {nullptr, nullptr};
    }
    if (compare(Get::get_value(node), val)) {
      auto split_right = split(node->right, val);
      node->set_right(split_right.first);
      return {node, split_right.second};
    } else {
      auto split_left = split(node->left, val);
      node->set_left(split_left.second);
      return {split_left.first, node};
    }
  }

  template <typename, typename, typename, typename>
  friend struct ::bimap;
};
} // namespace details
