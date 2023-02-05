#include "intrusive_bst.h"
#include <cassert>
#include <utility>

namespace details {

tree_element_base* tree_element_base::prev() const noexcept {
  if (left != nullptr) {
    return left->maximum();
  }
  tree_element_base const* low = this;
  tree_element_base* high = parent;
  while (high != nullptr && high->left == low) {
    low = high;
    high = high->parent;
  }
  return high;
}

tree_element_base* tree_element_base::next() const noexcept {
  if (right != nullptr) {
    return right->minimum();
  }
  tree_element_base const* low = this;
  tree_element_base* high = parent;
  while (high != nullptr && high->right == low) {
    low = high;
    high = high->parent;
  }
  return high;
}

tree_element_base* tree_element_base::minimum() noexcept {
  tree_element_base* cur = this;
  while (cur->left != nullptr) {
    cur = cur->left;
  }
  return cur;
}

tree_element_base* tree_element_base::maximum() noexcept {
  tree_element_base* cur = this;
  while (cur->right != nullptr) {
    cur = cur->right;
  }
  return cur;
}

void tree_element_base::set_left(
    details::tree_element_base* new_left) noexcept {
  left = new_left;
  if (left != nullptr) {
    left->parent = this;
  }
}

void tree_element_base::set_right(
    details::tree_element_base* new_right) noexcept {
  right = new_right;
  if (right != nullptr) {
    right->parent = this;
  }
}

void tree_element_base::swap(tree_element_base& other) noexcept {
  assert(parent == nullptr && other.parent == nullptr); // only swap sentinels
  std::swap(left, other.left);
  std::swap(right, other.right);
  fix_links();
  other.fix_links();
}

void tree_element_base::fix_links() noexcept {
  set_left(left);
  set_right(right);
}

} // namespace details
