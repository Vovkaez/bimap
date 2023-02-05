#pragma once

#include "intrusive_bst.h"
#include <cassert>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <type_traits>

namespace details {
struct left_tag;
struct right_tag;
} // namespace details

template <typename Left, typename Right, typename CompareLeft = std::less<Left>,
          typename CompareRight = std::less<Right>>
struct bimap {
  using left_t = Left;
  using right_t = Right;

private:
  using node_base = details::tree_element_base;
  using left_node = details::tree_element<details::left_tag>;
  using right_node = details::tree_element<details::right_tag>;

  struct bimap_element : left_node, right_node {};

  struct bimap_value_element : bimap_element {
    template <typename L, typename R>
    bimap_value_element(L&& left, R&& right)
        : left(std::forward<L>(left)), right(std::forward<R>(right)) {}

    Left left;
    Right right;
  };

  struct LeftDescriptor {
    using node_t = left_node;

    static left_t const& get_value(bimap_element const* p) noexcept {
      return static_cast<bimap_value_element const*>(p)->left;
    }

    static left_t& get_value(node_base* p) noexcept {
      return static_cast<bimap_value_element*>(static_cast<left_node*>(p))
          ->left;
    }

    static node_t const* get_node(bimap_element const* p) noexcept {
      return p;
    }
  };

  struct RightDescriptor {
    using node_t = right_node;

    static right_t const& get_value(bimap_element const* p) noexcept {
      return static_cast<bimap_value_element const*>(p)->right;
    }

    static right_t& get_value(node_base* p) noexcept {
      return static_cast<bimap_value_element*>(static_cast<right_node*>(p))
          ->right;
    }

    static node_t const* get_node(bimap_element const* p) noexcept {
      return p;
    }
  };

  template <typename Desc, typename OtherDesc>
  struct iterator {
    friend iterator<OtherDesc, Desc>;
    friend bimap;

    friend bool operator==(iterator const& a, iterator const& b) {
      return a.node == b.node;
    }

    friend bool operator!=(iterator const& a, iterator const& b) {
      return a.node != b.node;
    }

    auto& operator*() const {
      return Desc::get_value(node);
    }

    auto* operator->() const {
      return &operator*();
    }

    iterator& operator++() {
      node = downcast(Desc::get_node(node)->next());
      return *this;
    }

    iterator operator++(int) {
      iterator copy{*this};
      ++*this;
      return copy;
    }

    iterator& operator--() {
      node = downcast(Desc::get_node(node)->prev());
      return *this;
    }

    iterator operator--(int) {
      iterator copy{*this};
      --*this;
      return copy;
    }

    auto flip() const {
      return iterator<OtherDesc, Desc>(node);
    }

  private:
    explicit iterator(typename Desc::node_t const* node)
        : node(static_cast<bimap_element const*>(node)) {}

    explicit iterator(bimap_element const* node) : node(node) {}

    static bimap_element const* downcast(node_base const* ptr) noexcept {
      return static_cast<bimap_element const*>(
          static_cast<typename Desc::node_t const*>(ptr));
    }

    bimap_element const* node;
  };

public:
  using left_iterator = iterator<LeftDescriptor, RightDescriptor>;
  using right_iterator = iterator<RightDescriptor, LeftDescriptor>;

  // Создает bimap не содержащий ни одной пары.
  bimap(CompareLeft compare_left = CompareLeft(),
        CompareRight compare_right = CompareRight())
      : sentinel(), left_tree(&sentinel, std::move(compare_left)),
        right_tree(&sentinel, std::move(compare_right)), size_(0) {}

  // Конструкторы от других и присваивания
  bimap(bimap const& other) : bimap(other.left_tree, other.right_tree) {
    for (auto it = other.begin_left(); it != other.end_left(); ++it) {
      insert(*it, *it.flip());
    }
  }

  bimap(bimap&& other) noexcept = default;

  bimap& operator=(bimap const& other) {
    if (&other != this) {
      bimap copy{other};
      swap(copy);
    }
    return *this;
  }

  bimap& operator=(bimap&& other) noexcept {
    if (&other != this) {
      bimap copy{std::move(other)};
      swap(copy);
    }
    return *this;
  }

  // Деструктор. Вызывается при удалении объектов bimap.
  // Инвалидирует все итераторы ссылающиеся на элементы этого bimap
  // (включая итераторы ссылающиеся на элементы следующие за последними).
  ~bimap() {
    erase_left(begin_left(), end_left());
  }

  // Вставка пары (left, right), возвращает итератор на left.
  // Если такой left или такой right уже присутствуют в bimap, вставка не
  // производится и возвращается end_left().

  left_iterator insert(left_t const& left, right_t const& right) {
    return insert_impl(left, right);
  }

  left_iterator insert(left_t const& left, right_t&& right) {
    return insert_impl(left, std::move(right));
  }

  left_iterator insert(left_t&& left, right_t const& right) {
    return insert_impl(std::move(left), right);
  }

  left_iterator insert(left_t&& left, right_t&& right) {
    return insert_impl(std::move(left), std::move(right));
  }

  // Удаляет элемент и соответствующий ему парный.
  // erase невалидного итератора неопределен.
  // erase(end_left()) и erase(end_right()) неопределены.
  // Пусть it ссылается на некоторый элемент e.
  // erase инвалидирует все итераторы ссылающиеся на e и на элемент парный к e.
  left_iterator erase_left(left_iterator it) {
    left_iterator result{it};
    ++result;
    erase(const_cast<bimap_element*>(it.node));
    return result;
  }

  // Аналогично erase, но по ключу, удаляет элемент если он присутствует, иначе
  // не делает ничего Возвращает была ли пара удалена
  bool erase_left(left_t const& left) {
    left_node* p = find_left_node(left);
    if (p == nullptr) {
      return false;
    }
    erase(static_cast<bimap_element*>(p));
    return true;
  }

  right_iterator erase_right(right_iterator it) {
    right_iterator result{it};
    ++result;
    erase(const_cast<bimap_element*>(it.node));
    return result;
  }

  bool erase_right(right_t const& right) {
    right_node* p = find_right_node(right);
    if (p == nullptr) {
      return false;
    }
    erase(static_cast<bimap_element*>(p));
    return true;
  }

  // erase от ренжа, удаляет [first, last), возвращает итератор на последний
  // элемент за удаленной последовательностью
  left_iterator erase_left(left_iterator first, left_iterator last) {
    while (first != last) {
      erase_left(first++);
    }
    return last;
  }

  right_iterator erase_right(right_iterator first, right_iterator last) {
    while (first != last) {
      erase_right(first++);
    }
    return last;
  }

  // Возвращает итератор по элементу. Если не найден - соответствующий end()
  left_iterator find_left(left_t const& left) const {
    left_iterator it = lower_bound_left(left);
    return (*it == left) ? it : end_left();
  }

  right_iterator find_right(right_t const& right) const {
    right_iterator it = lower_bound_right(right);
    return (*it == right) ? it : end_right();
  }

  // Возвращает противоположный элемент по элементу
  // Если элемента не существует -- бросает std::out_of_range
  right_t const& at_left(left_t const& key) const {
    auto it = find_left(key);
    if (it == end_left()) {
      throw std::out_of_range("key not found");
    }
    return *it.flip();
  }

  left_t const& at_right(right_t const& key) const {
    auto it = find_right(key);
    if (it == end_right()) {
      throw std::out_of_range("key not found");
    }
    return *it.flip();
  }

  // Возвращает противоположный элемент по элементу
  // Если элемента не существует, добавляет его в bimap и на противоположную
  // сторону кладет дефолтный элемент, ссылку на который и возвращает
  // Если дефолтный элемент уже лежит в противоположной паре - должен поменять
  // соответствующий ему элемент на запрашиваемый (смотри тесты)

  template <typename T = right_t,
            std::enable_if_t<std::is_default_constructible_v<T>, bool> = true>
  right_t const& at_left_or_default(left_t const& key) {
    auto it = find_left(key);
    if (it == end_left()) {
      right_t def{};
      auto def_it = find_right(def);
      if (def_it != end_right()) {
        erase_right(def_it);
        erase_left(def);
      }
      return *insert(key, std::move(def)).flip();
    }
    return *it.flip();
  }

  template <typename T = left_t,
            std::enable_if_t<std::is_default_constructible_v<T>, bool> = true>
  left_t const& at_right_or_default(right_t const& key) {
    auto it = find_right(key);
    if (it == end_right()) {
      left_t def{};
      auto def_it = find_left(def);
      if (def_it != end_left()) {
        erase_left(def_it);
        erase_right(def);
      }
      return *insert(std::move(def), key);
    }
    return *it.flip();
  }

  // lower и upper bound'ы по каждой стороне
  // Возвращают итераторы на соответствующие элементы
  // Смотри std::lower_bound, std::upper_bound.
  left_iterator lower_bound_left(const left_t& left) const {
    return left_iterator(left_tree.lower_bound(left));
  }

  left_iterator upper_bound_left(const left_t& left) const {
    left_iterator it = lower_bound_left(left);
    if (it == end_left()) {
      return it;
    }
    return (*it == left) ? ++it : it;
  }

  right_iterator lower_bound_right(const right_t& right) const {
    return right_iterator(right_tree.lower_bound(right));
  }

  right_iterator upper_bound_right(const right_t& right) const {
    right_iterator it = lower_bound_right(right);
    if (it == end_right()) {
      return it;
    }
    return (*it == right) ? ++it : it;
  }

  // Возващает итератор на минимальный по порядку left.
  left_iterator begin_left() const {
    auto& mut_sentinel = const_cast<bimap_element&>(sentinel);
    return left_iterator(
        static_cast<left_node const*>(mut_sentinel.left_node::minimum()));
  }
  // Возващает итератор на следующий за последним по порядку left.
  left_iterator end_left() const {
    return left_iterator(&sentinel);
  }

  // Возващает итератор на минимальный по порядку right.
  right_iterator begin_right() const {
    auto& mut_sentinel = const_cast<bimap_element&>(sentinel);
    return right_iterator(
        static_cast<right_node const*>(mut_sentinel.right_node::minimum()));
  }
  // Возващает итератор на следующий за последним по порядку right.
  right_iterator end_right() const {
    return right_iterator(&sentinel);
  }

  // Проверка на пустоту
  bool empty() const {
    return size_ == 0;
  }

  // Возвращает размер бимапы (кол-во пар)
  std::size_t size() const {
    return size_;
  }

  // операторы сравнения
  bool operator==(bimap const& other) const {
    if (size() != other.size()) {
      return false;
    }

    auto left_not_eq = [this](left_t const& x, left_t const& y) {
      return left_tree.compare(x, y) || left_tree.compare(y, x);
    };

    auto right_not_eq = [this](right_t const& x, right_t const& y) {
      return right_tree.compare(x, y) || right_tree.compare(y, x);
    };

    auto it_1 = begin_left();
    auto it_2 = other.begin_left();
    for (; it_1 != end_left(); ++it_1, ++it_2) {
      if (left_not_eq(*it_1, *it_2) ||
          right_not_eq(*it_1.flip(), *it_2.flip())) {
        return false;
      }
    }

    return true;
  }

  friend bool operator!=(bimap const& a, bimap const& b) {
    return !(a == b);
  }

  void swap(bimap& other) {
    using std::swap;
    sentinel.left_node::swap(static_cast<left_node&>(other.sentinel));
    sentinel.right_node::swap(static_cast<right_node&>(other.sentinel));
    swap(left_tree.comparator(), other.left_tree.comparator());
    swap(right_tree.comparator(), other.right_tree.comparator());
    swap(size_, other.size_);
  }

private:
  bimap_element sentinel;
  details::intrusive_bst<left_t, CompareLeft, LeftDescriptor, details::left_tag>
      left_tree;
  details::intrusive_bst<right_t, CompareRight, RightDescriptor,
                         details::right_tag>
      right_tree;
  std::size_t size_;

  left_node* find_left_node(left_t const& left) const {
    left_node* p = left_tree.lower_bound(left);
    return (p == nullptr || p == &sentinel ||
            !(LeftDescriptor::get_value(p) == left))
             ? nullptr
             : p;
  }

  right_node* find_right_node(right_t const& right) const {
    right_node* p = right_tree.lower_bound(right);
    return (p == nullptr || p == &sentinel ||
            !(RightDescriptor::get_value(p) == right))
             ? nullptr
             : p;
  }

  template <typename L, typename R>
  left_iterator insert_impl(L&& left, R&& right) {
    if (find_left_node(left) != nullptr || find_right_node(right) != nullptr) {
      return end_left();
    }

    auto* element =
        new bimap_value_element(std::forward<L>(left), std::forward<R>(right));
    left_tree.insert(element);
    right_tree.insert(element);
    size_++;
    return left_iterator(element);
  }

  void erase(bimap_element* element) {
    size_--;
    left_tree.erase(element);
    right_tree.erase(element);
    delete static_cast<bimap_value_element*>(element);
  }
};
