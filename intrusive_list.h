#pragma once

#include <type_traits>
#include <iterator>
#include <cassert>
#include <memory>

namespace intrusive
{
    struct default_tag;

    template <typename Tag = default_tag>
    struct list_element
    {
    private:
        template <typename FT, typename FTag>
        friend struct list;
        list_element *next = nullptr;
        list_element *prev = nullptr;
    public:
        list_element* unlink()
        {
            if (next != nullptr)
                next->prev = prev;
            if (prev != nullptr)
                prev->next = next;
            auto ret = next;
            prev = next = nullptr;
            return ret;
        }

        list_element(void) {}

        ~list_element()
        {
            unlink();
        }

        list_element(list_element&& r)
        {
            next = r.next;
            prev = r.prev;
            if (next != nullptr)
                next->prev = this;
            if (prev != nullptr)
                prev->next = this;
            r->next = r->prev = nullptr;
        }
        list_element& operator=(list_element&& r)
        {
            unlink();
            next = r.next;
            prev = r.prev;
            if (next != nullptr)
                next->prev = this;
            if (prev != nullptr)
                prev->next = this;
            r.next = r.prev = nullptr;
            return *this;
        }
    };

    template <typename T, typename Tag = default_tag>
    class list
    {
    private:
        list_element<Tag> root;

        template<typename IT>
        class iterator_impl
        {
        public:
            using iterator_category = std::bidirectional_iterator_tag;
            using value_type = IT;
            using difference_type = std::ptrdiff_t;
            using pointer = value_type*;
            using reference	= value_type&;
        private:
            friend list;
            list_element<Tag> *me;
            explicit iterator_impl(decltype(me) to) noexcept
                    : me(to)
            {}
        public:
            iterator_impl(void) noexcept
                    : me(nullptr)
            {}
            pointer operator->(void) const noexcept
            {
                return static_cast<IT*>(me);
            }
            operator reference(void) const
            {
                return static_cast<IT&>(*me);
            }

            reference operator*(void) const noexcept
            {
                return static_cast<IT&>(*me);
            }

            iterator_impl& operator++(void) noexcept
            {
                me = me->next;
                return *this;
            }
            iterator_impl operator++(int) noexcept
            {
                auto copy = me;
                me = me->next;
                return iterator_impl(copy);
            }
            iterator_impl& operator--(void) noexcept
            {
                me = me->prev;
                return *this;
            }
            iterator_impl operator--(int) noexcept
            {
                auto copy = me;
                me = me->prev;
                return iterator_impl(copy);
            }

            template<typename T1>
            bool operator==(const iterator_impl<T1> &r) const noexcept
            {
                return me == r.me;
            }
            template<typename T1>
            bool operator!=(const iterator_impl<T1> &r) const noexcept
            {
                return !operator==(r);
            }

            operator iterator_impl<const value_type>() const noexcept
            {
                return iterator_impl<const value_type>(me);
            }

            bool valid() const noexcept
            {
                return me != nullptr;
            }
        };
        static list_element<Tag> & cast_el(T &r) noexcept
        {
            return static_cast<list_element<Tag>&>(r);
        }
    public:
        using iterator = iterator_impl<T>;
        using const_iterator = iterator_impl<const T>;

        static_assert(std::is_convertible_v<T&, list_element<Tag>&>,
                      "value type is not convertible to list_element");

        list() noexcept
        {
            root.next = root.prev = &root;
        }
        list(list const&) = delete;
        list(list&& r) noexcept
                : list()
        {
            operator=(std::move(r));
        }
        ~list()
        {
            clear();
        }

        list& operator=(list const&) = delete;
        list& operator=(list&& r) noexcept
        {
            clear();
            splice(end(), r, r.begin(), r.end());
            return *this;
        }

        void clear() noexcept
        {
            if (empty())
                return;
            // now root.next != root
            root.prev->next = nullptr;
            auto next = root.next;
            root.next = root.prev = &root;

            while (next->next != nullptr)
            {
                auto save = next->next;
                next->prev = next->next = nullptr;
                next = save;
            }
            next->prev = nullptr;
        }

        void push_back(T& u) noexcept
        {
            insert(end(), u);
        }
        void pop_back() noexcept
        {
            root.prev->unlink();
        }
        T& back() noexcept
        {
            return static_cast<T&>(*root.prev);
        }
        T const& back() const noexcept
        {
            return static_cast<const T&>(*root.prev);
        }

        void push_front(T& u) noexcept
        {
            insert(begin(), u);
        }
        void pop_front() noexcept
        {
            root.next->unlink();
        }
        T& front() noexcept
        {
            return static_cast<T&>(*root.next);
        }
        T const& front() const noexcept
        {
            return static_cast<T&>(*root.next);
        }

        bool empty() const noexcept
        {
            return root.next == &root;
        }

        iterator begin() noexcept
        {
            return iterator(root.next);
        }
        const_iterator begin() const noexcept
        {
            return const_iterator(root.next);
        }

        iterator end() noexcept
        {
            return iterator(&root);
        }
        const_iterator end() const noexcept
        {
            return const_iterator(const_cast<list_element<Tag>*>(&root));
        }

        iterator insert(const_iterator pos, T& u) noexcept
        {
            auto &v = static_cast<list_element<Tag>&>(u);
            pos.me->prev->next = &v;
            v.prev = pos.me->prev;
            v.next = pos.me;
            pos.me->prev = &v;
            return iterator(&v);
        }
        iterator erase(const_iterator pos) noexcept
        {
            iterator ret(pos.me->next);
            pos.me->unlink();
            return ret;
        }
        void splice(const_iterator pos, list&, const_iterator first, const_iterator last) noexcept
        {
            if (pos == first || first == last)
                return;
            auto *true_last = last.me->prev;
            first.me->prev->next = true_last->next;
            true_last->next->prev = first.me->prev;

            pos.me->prev->next = first.me;
            first.me->prev = pos.me->prev;

            pos.me->prev = true_last;
            true_last->next = pos.me;
        }
        static iterator to_iterator(list_element<Tag>& t)
        {
            return iterator(&t);
        }
    };
}
