#ifndef SJTU_DEQUE_HPP
#define SJTU_DEQUE_HPP

#include "exceptions.hpp"
#include "utility.hpp"

#include <cstddef>
#include <memory>
#include <iostream>

namespace sjtu {
    template<class T>
    class deque {
    private:
        template<class A>
        class Vector {
            friend deque;
            A *buffer;          
            std::allocator<A> a;
            int _size, _capacity;
            bool full() { return _size == _capacity; }            

            void _resize(int new_capacity) {
                A *new_buffer = a.allocate(new_capacity);
                memcpy(new_buffer, buffer, sizeof(A) * _size);
                a.deallocate(buffer, _capacity);
                _capacity = new_capacity;
                buffer = new_buffer;
            }

            void _shrink() {
                if (_capacity >= 256 && (_size << 2) < _capacity) _resize(_capacity >> 2);
            }

            void _expand() {
                if (!full()) return;
                _resize(_capacity << 1);
            }

           
            static int change(int _mincapacity) {
                int s = 64;
                while (s < _mincapacity) s <<= 1;
                return s;
            }

             A *get_buffer() {
                return buffer;
            }
        public:
            Vector(int capacity = 64) : _size(0), _capacity(capacity) {
                buffer = a.allocate(_capacity);
            }

            Vector(const Vector &that) : _size(that._size), _capacity(that._capacity) {
                buffer = a.allocate(_capacity);
                for (int i = 0; i < that._size; i++) a.construct(buffer + i, that[i]);
            }

            Vector &operator=(const Vector &that) {
                if (this == &that) return *this;
                for (int i = 0; i < _size; i++) a.destroy(buffer + i);
                a.deallocate(buffer, _capacity);
                _capacity = that._capacity;
                _size = that._size;
                buffer = a.allocate(_capacity);
                for (int i = 0; i < that._size; i++) a.construct(buffer + i, that[i]);
                return *this;
            }

            int size() const { return _size; }

            void insert(int pos, const A &x) {
                _expand();
                if (pos != _size) memmove(buffer + pos + 1, buffer + pos, (_size - pos) * sizeof(A));
                a.construct(buffer + pos, x);
                ++_size;
            }

            void erase(int pos) {
                a.destroy(buffer + pos);
                if (pos != _size - 1) memmove(buffer + pos, buffer + pos + 1, (_size - pos - 1) * sizeof(A));
                --_size;
                _shrink();
            }

            void clear() {
                for (int i = 0; i < _size; i++) a.destroy(buffer + i);
                _size = 0;
            }

            ~Vector() {
                for (int i = 0; i < _size; i++) a.destroy(buffer + i);
                a.deallocate(buffer, _capacity);
            }

            A &operator[](int pos) { return buffer[pos]; }

            const A &operator[](int pos) const { return buffer[pos]; }
        };

        int _size;
        Vector<Vector<T> > x;
    private:
        template<typename Tx, typename Ty>
        class _iterator {
        private:
            friend deque;
            Ty *y;
            mutable Tx *addition;
            int pos;

            _iterator(Ty *y, const int &pos) : y(y), pos(pos), addition(NULL) {}    

            void check_valid(Ty *y) const { if (y != this->y) throw invalid_iterator(); }

            

            _iterator &construct() {
                addition = nullptr;
                 if (!this->y) throw invalid_iterator();
                this->y->throw_if_out_of_bound2(this->pos);
                return *this;
            }

            const _iterator &construct() const {
                addition = nullptr;
                if (!this->y) throw invalid_iterator();
                this->y->throw_if_out_of_bound2(this->pos);
                return *this;
            }

        public:
            _iterator(const _iterator &that) = default;

            _iterator() : _iterator(nullptr, 0) {}

            /**
             * return a new iterator which pointer n-next elements
             *   even if there are not enough elements, the behaviour is **undefined**.
             * as well as operator-
             */
            _iterator operator+(const int &n) const { return _iterator(y, pos + n).construct(); }

            _iterator operator-(const int &n) const { return _iterator(y, pos - n).construct(); }

            // return th distance between two iterator,
            // if these two iterators points to different vectors, throw invaild_iterator.
            int operator-(const _iterator &rhs) const {
                construct();
                check_valid(rhs.y);
                return pos - rhs.pos;
            }

            _iterator &operator+=(const int &n) { return *this = _iterator(y, pos + n).construct(); }

            _iterator &operator-=(const int &n) { return *this = _iterator(y, pos - n).construct(); }

            _iterator operator++(int) {
                auto it = *this;
                ++(*this);
                return it;
            }

            _iterator &operator++() {
                ++pos;
                return construct();
            }

            _iterator operator--(int) {
                auto it = *this;
                --(*this);
                return it;
            }

            _iterator &operator--() {
                --pos;
                return construct();
            }

            Tx &operator*() const {
                return  y->search(pos);
                
            }

            Tx *operator->() const  {
                return &(y->search(pos));
                
            }

            bool operator==(const _iterator &rhs) const { return rhs.y == y && rhs.pos == pos; }

            bool operator!=(const _iterator &rhs) const { return !(*this == rhs); }
        };

    private:
        void start() {
            _size = 0;
            x.insert(0, Vector<T>());
        }

        void throw_if_out_of_bound1(int pos) const {
            if (pos < 0 || pos >= size()) throw index_out_of_bound();
        }

        void throw_if_out_of_bound2(int pos) const {
            if (pos < 0 || pos > size()) throw index_out_of_bound();
        }
        template<typename Tt, typename Tx>
        static Tx &search(Tt *ths, int pos) {
            ths->throw_if_out_of_bound1(pos);
            int i = ths->_find1(pos);
            return ths->x[i][pos];
        }

        T &search(int pos) { return search<deque, T>(this, pos); }

        const T &search(int pos) const { return search<const deque, const T>(this, pos); }

        bool check_split(int chksize) { return chksize >= 16 && chksize * chksize > (_size << 2); }

        bool check_merge(int chksize) { return chksize * chksize  <= (_size >> 2);}

        void _split(int chunk) {
            int split_size = x[chunk].size() >> 1;
            x.insert(chunk, Vector<T>(Vector<T>::change(x[chunk]._size)));
            auto &chk_a = x[chunk];
            auto &chk_b = x[chunk + 1];
            memcpy(chk_a.buffer, chk_b.buffer, sizeof(T) * split_size);
            chk_a._size = split_size;
            memmove(chk_b.buffer, chk_b.buffer + split_size, sizeof(T) * (chk_b._size - split_size));
            chk_b._size -= split_size;
        }

        void _merge(int chunk) {
            auto &chk_a = x[chunk];
            auto &chk_b = x[chunk + 1];
            chk_a._resize(Vector<T>::change(chk_a._size + chk_b._size));
            memcpy(chk_a.buffer + chk_a._size, chk_b.buffer, sizeof(T) * chk_b._size);
            chk_a._size += chk_b._size;
            chk_b._size = 0;
            x.erase(chunk + 1);
        }

       
        int _find1(int &pos) const {
            int  i=0,_pos = pos;
            if (_pos <= _size >> 1) {
                while (_pos >= x[i]._size) {                   
                    _pos -= x[i]._size;
                    ++i;
                }
            } else {
                 i = x._size - 1;
                _pos = _size - _pos;
                while (_pos >  x[i]._size) {                   
                    _pos -= x[i]._size;
                    --i;
                }
                _pos = x[i]._size - _pos;
            }
            pos = _pos;
            return i;
        }

        int _find2(int &pos) const {
            int i = 0, _pos = pos;
            if (_pos <= _size >> 1) {
                while (_pos > x[i].size()) {
                    _pos -= x[i].size();
                    ++i;
                }
            } else {
                i = x.size() - 1;
                _pos = _size - _pos;
                while (i != 0 && _pos >=  x[i].size()) {
                    _pos -= x[i].size();
                    --i;
                }
                _pos = x[i].size() - _pos;
            }
            pos = _pos;
            return i;
        }

        int _insert(int pos, const T &value) {
            throw_if_out_of_bound2(pos);
            int __pos = pos;
            int i = _find2(pos);
            x[i].insert(pos, value);
            ++_size;
            if (check_split(x[i].size())) _split(i);
             if(rand()<9999) rebuild();
            return __pos;
        }
        int _remove(int pos) {
            throw_if_out_of_bound1(pos);
            int __pos = pos;
            int i = _find1(pos);
            x[i].erase(pos);
            --_size;
            if (i != x.size() - 1) {
                if (check_merge(x[i].size() + x[i + 1].size())) _merge(i);
            } else {
                if (x.size() > 1 && x[i].size() == 0) x.erase(i);
            }
             if(rand()<9999) rebuild();
            return __pos;
        }

    public:
        void rebuild() {
            if(x.size()>1){
             for (int i = 0; i < x.size() - 1; i++) {
                if (x[i].size() == 0) {
                    x.erase(i);
                }
            }}
            for (int i = 0; i < x.size(); i++) {
                if (check_split(x[i].size())) {
                    _split(i);
                    ++i;
                }
            }
            for (int i = 0; i < x.size() - 1; i++) {
                if (check_merge(x[i].size() + x[i + 1].size())) {
                    _merge(i);
                    --i;
                }
            }
        }

 

        
        
    public:
        typedef _iterator<T, deque> iterator;
        typedef _iterator<const T, const deque> const_iterator;

        /**
         * Constructors
         */
        deque() {
            _size = 0;
            start();
        }

        /**
         * Deconstructor
         */
        ~deque() {}

        /**
         * search specified element with bounds checking
         * throw index_out_of_bound if out of bound.
         */
        T &at(const size_t &pos) { return search(pos); }

        const T &at(const size_t &pos) const { return search(pos); }

        T &operator[](const size_t &pos) { return search(pos); }

        const T &operator[](const size_t &pos) const { return search(pos); }

        /**
         * search the first element
         * throw container_is_empty when the container is empty.
         */
        const T &front() const { return search(0); }

        /**
         * search the last element
         * throw container_is_empty when the container is empty.
         */
        const T &back() const { return search(size() - 1); }

        /**
         * returns an iterator to the beginning.
         */
        iterator begin() { return iterator(this, 0); }

        const_iterator cbegin() const { return const_iterator(this, 0); }

        /**
         * returns an iterator to the end.
         */
        iterator end() { return iterator(this, size()); }

        const_iterator cend() const { return const_iterator(this, size()); }

        /**
         * checks whether the container is empty.
         */
        bool empty() const { return _size == 0; }

        /**
         * returns the number of elements
         */
        size_t size() const { return _size; }

        /**
         * clears the contents
         */
        void clear() {
            x.clear();
            start();
        }

        /**
         * inserts elements at the specified locat on in the container.
         * inserts value before pos
         * returns an iterator pointing to the inserted value
         *     throw if the iterator is invalid or it point to a wrong place.
         */
        iterator insert(iterator pos, const T &value) {
            pos.check_valid(this);
            return iterator(this, _insert(pos.pos, value));
        }

        /**
         * removes specified element at pos.
         * removes the element at pos.
         * returns an iterator pointing to the following element, if pos pointing to the last element, end() will be returned.
         * throw if the container is empty, the iterator is invalid or it points to a wrong place.
         */
        iterator erase(iterator pos) {
            pos.check_valid(this);
            return iterator(this, _remove(pos.pos));
        }

        /**
         * adds an element to the end
         */
        void push_back(const T &value) { _insert(size(), value); }

        /**
         * removes the last element
         *     throw when the container is empty.
         */
        void pop_back() { _remove(size() - 1); }

        /**
         * inserts an element to the beginning.
         */
        void push_front(const T &value) { _insert(0, value); }

        /**
         * removes the first element.
         *     throw when the container is empty.
         */
        void pop_front() { _remove(0); }

        
    };

}

#endif