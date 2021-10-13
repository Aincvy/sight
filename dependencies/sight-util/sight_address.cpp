//
// Created by Aincvy(aincvy@gmail.com) on 2021/8/6.
//

#include <cstdlib>

#include "sight_address.h"

#include "absl/strings/str_split.h"
#include "absl/strings/numbers.h"
#include "string"

const char* sight::addressPrefix  = "sight://";
const int sight::addressPrefixLen = static_cast<int>(strlen(sight::addressPrefix));

namespace sight {

    float Address::asFloat(float fallback) const {
        float f = 0;
        if (absl::SimpleAtof(this->part, &f)) {
            return f;
        }
        return fallback;
    }

    int Address::asInt(int fallback) const {
        int i = 0;
        if (absl::SimpleAtoi(this->part, &i)) {
            return i;
        }

        return fallback;
    }

    double Address::asDouble(double fallback) const {
        double d = 0;
        if (absl::SimpleAtod(this->part, &d)) {
            return d;
        }
        return fallback;
    }

    bool Address::asBool(bool fallback) const {
        bool b = false;
        if (absl::SimpleAtob(this->part, &b)) {
            return b;
        }
        return fallback;
    }

    Address *Address::last() {
        auto c = this;
        while (c->next) {
            c = c->next;
        }
        return c;
    }

    int Address::length() {
        auto c = this;
        auto count = 0;
        while (c) {
            count++;
            c = c->next;
        }

        return count;
    }

    void freeAddress(struct Address*& first) {
        free(first);
        first = nullptr;
    }

    Address* resolveAddress(const std::string &fullAddress) {
        auto & str = fullAddress.find(addressPrefix) == 0 ? fullAddress.substr(addressPrefixLen) : fullAddress;
        std::vector<std::string> list = absl::StrSplit(str, "/");
        auto size = list.size();
        auto first = (Address*) calloc(1, sizeof(struct Address) * size);
        auto c = first;

        for (int i = 0; i < size; ++i) {
            sprintf(c->part, "%s", list[i].c_str());

            if (i + 1 < size) {
                // has next.
                c->next = first + (i + 1);
                c = c->next;
            }
        }


        return first;
    }

}