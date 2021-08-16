//
// Created by Aincvy(aincvy@gmail.com) on 2021/8/16.
//

#pragma once

#include <cstddef>
#include "queue"

#include "sight_defines.h"

namespace sight {

    class ResetAble {
    public:
        virtual void reset() = 0;
    };

    template <class T>
    class SightArray {
    public:

        SightArray() : SightArray(MEDIUM_ARRAY_SIZE) {}
        SightArray(size_t capacity) : capacity(capacity) {
            pointer = new T[capacity]();
        }
        ~SightArray(){
            if (pointer) {
                delete pointer;
                pointer = nullptr;
            }
        }

        /**
         *
         * @param t
         * @return
         */
        T* add(T const& t) {
            if (current >= capacity) {
                return nullptr;
            }

            auto c = current++;
            this->pointer[c] = t;
            return this->pointer + c;
        }

        /**
         * get a element pointer.
         * @return
         */
        T* add(){
            if (!notUsed.empty()) {
                int index = notUsed.front();
                notUsed.pop();
                return this->pointer + index;
            }
            return this->pointer + current++;
        }

        bool empty() const{
            return current == 0;
        }

        bool full() const{
            return current >= capacity;
        }

        /**
         * Remove element. If type is ResetAble, it will call ResetAble::reset().
         * @param p
         * @return
         */
        bool remove(T *p){
            if (p >= this->pointer && p <= this->pointer + capacity - 1) {
                auto index = p - this->pointer;
                // mark index is not used.
                notUsed.push(index);

                if constexpr(std::is_same_v<T, ResetAble>) {
                    ((ResetAble*)p)->reset();
                }
            }
        }

    private:
        T* pointer = nullptr;
        std::queue<int> notUsed;

        size_t current = 0;
        size_t capacity = 0;
    };

}