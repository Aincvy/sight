//
// Created by Aincvy(aincvy@gmail.com) on 2021/8/16.
//

#pragma once

#include <cstddef>
#include "absl/container/flat_hash_set.h"

#include "sight_defines.h"

namespace sight {

    class ResetAble {
    public:
        virtual void reset() = 0;
    };

    /**
     * A auto expand array. NOT thread safe.
     * All element's address do not change.
     * @tparam T need has a empty constructor
     */
    template<class T>
    class SightArray {
    public:

        SightArray() : SightArray(MEDIUM_ARRAY_SIZE) {}

        explicit SightArray(size_t capacity) : capacityPerArray(capacity) {
            expand();
        }

        ~SightArray() {
            if (pointer) {
                for (int i = 0; i < arraySize; ++i) {
                    auto p = pointer[i];
                    delete[] p;
                }
                delete[] pointer;
                pointer = nullptr;
            }
        }


        /**
         *
         * @param t
         * @return
         */
        T* add(T const &t) {
            T *p = add();
            *p = t;
            return p;
        }

        /**
         * get a element pointer.
         * @return
         */
        T* add() {
            int index;
            if (!notUsed.empty()) {
                auto iter = notUsed.begin();
                index = * iter;
                notUsed.erase(iter);
            } else {
                index = current++;
            }

            expandIfFull();
            return obtainAddress(index);
        }

        [[nodiscard]] bool empty() const {
            return current == 0;
        }

        /**
         * If you add element, array will be auto expand.
         * @return
         */
        [[nodiscard]] bool full() const {
            return current >= capacityPerArray * arraySize;
        }

        /**
         * Remove element. If type is ResetAble, it will call ResetAble::reset().
         * @param p
         * @return true: delete successful, false: not find.
         */
        bool remove(T *p) {
            if (!p) {
                return false;
            }

            int arrayIndex, elementIndex;
            for (int i = 0; i < arraySize; ++i) {
                T *temp = this->pointer[i];
                if (p >= temp && p <= temp + capacityPerArray - 1) {
                    // here it is.
                    elementIndex = p - temp;
                    arrayIndex = i;
                    int tempIndex = arrayIndex * capacityPerArray + elementIndex;
                    if (tempIndex == current - 1) {
                        current--;
                    } else {
                        notUsed.insert(tempIndex);
                    }

                    // reset element
                    resetElement(p);

                    return true;
                }
            }

            return false;
        }

        void clear(){
            for (int i = 0; i < arraySize; ++i) {
                for (int j = 0; j < capacityPerArray; ++j) {
                    resetElement(& (pointer[i][j]));
                }
            }
            current = 0;
            notUsed.clear();
        }



        struct SightArrayConstIterator {
            SightArray const* arrayPointer;
            size_t current;

            SightArrayConstIterator(SightArray const* arrayPointer, size_t current) : arrayPointer(arrayPointer), current(current) {}

            T const & operator*() const { return *arrayPointer->obtainAddress(current); }
            T const * operator->() const {
                return arrayPointer->obtainAddress(current);
            }
            SightArrayConstIterator operator++() {
                do {
                    current++;
                } while (arrayPointer->notUsed.template contains(current));
                return *this;
            }
            SightArrayConstIterator operator++(int) { SightArrayConstIterator tmp = *this; ++(*this); return tmp; }

            friend bool operator== (const SightArrayConstIterator& a, const SightArrayConstIterator& b) {
                return a.current == b.current;
            };
            friend bool operator!= (const SightArrayConstIterator& a, const SightArrayConstIterator& b) {
                return a.current != b.current;
            };

        };

        struct SightArrayIterator : SightArrayConstIterator {
            SightArrayIterator(SightArray* arrayPointer, size_t current) : SightArrayConstIterator(arrayPointer, current) {}

            T & operator*() { return const_cast<T&>(SightArrayConstIterator::operator*()); }
            T * operator->() { return const_cast<T*>(SightArrayConstIterator::operator->()); }
        };

        SightArrayConstIterator begin() const{
            return SightArrayConstIterator(this, findFirstValidIndex());
        }

        SightArrayConstIterator end() const{
            return SightArrayConstIterator(this, current);
        }

        SightArrayIterator begin(){
            return SightArrayIterator(this, findFirstValidIndex());
        }

        SightArrayIterator end(){
            return SightArrayIterator(this, current);
        }

        bool erase(SightArrayConstIterator iterator) {
            return remove(obtainAddress(iterator.current));
        }

    private:
        T **pointer = nullptr;
//        std::vector<int> notUsed;
        absl::flat_hash_set<int> notUsed;

        size_t arraySize = 0;
        size_t current = 0;
        const size_t capacityPerArray;

        /**
         * Add an array.
         */
        void expand() {
            int lastArraySize = arraySize++;
            T **temp = new T *[arraySize];
            memcpy(temp, pointer, lastArraySize * sizeof(void *));
            temp[lastArraySize] = new T[capacityPerArray]();

            if (this->pointer) {
                delete[] this->pointer;
            }

            this->pointer = temp;
        }

        void expandIfFull() {
            if (full()) {
                expand();
            }
        }

        bool obtainIndex(int index, int *arrayIndex, int *elementIndex) const {
            *arrayIndex = index / capacityPerArray;
            *elementIndex = index % capacityPerArray;
            return true;
        }

        T *obtainAddress(int arrayIndex, int elementIndex) {
            T *p = this->pointer[arrayIndex];
            return &(p[elementIndex]);
        }

        T *obtainAddress(int index){
            if (index < capacityPerArray) {
                return obtainAddress(0, index);
            }

            int arrayIndex, elementIndex;
            obtainIndex(index, &arrayIndex, &elementIndex);
            return obtainAddress(arrayIndex, elementIndex);
        }

        T const* obtainAddress(int arrayIndex, int elementIndex) const{
            T *p = this->pointer[arrayIndex];
            return &(p[elementIndex]);
        }

        T const* obtainAddress(int index) const{
            if (index < capacityPerArray) {
                return obtainAddress(0, index);
            }

            int arrayIndex, elementIndex;
            obtainIndex(index, &arrayIndex, &elementIndex);
            return obtainAddress(arrayIndex, elementIndex);
        }

        void resetElement(T* p){
            if constexpr(std::is_same_v<T, ResetAble>) {
                ((ResetAble *) p)->reset();
            }

            *p = {};
        }

        [[nodiscard]] size_t findFirstValidIndex() const{
            int i = 0;
            while (notUsed.template contains(i)) {
                i++;
            }
            return i;
        }

    };


}