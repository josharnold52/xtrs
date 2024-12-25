//
// Created by arnold on 12/22/24.
//

#ifndef XTRS_DPMHW_IMPL_H
#define XTRS_DPMHW_IMPL_H


#include <cstddef>

#define INLINE_PAUSE  { __asm__ __volatile__ ("pause"); }

namespace dpmhw {


    /**
     * A template wrapper around qsort for sorting pointers to a type T.  Null pointers
     * always sort to the end of the list.  The "run" method does the actual sort
     * @tparam T the type of objects to sort
     * @tparam cf functopn that compares the 2 objects
     */
    template<class T, int (*cf)(const T &t1, const T &t2)> class tqsort {
    private:
        static int vcf(const void *e1, const void *e2) {
            const T *pt1 = *(static_cast<const T * const *>(e1));
            const T *pt2 = *(static_cast<const T * const *>(e2));
            if (!(pt1 && pt2)) {
                return pt1 ? -1 : (pt2 ? 1 : 0);
            }
            return cf(*pt1, *pt2);
        }
    public:
        tqsort() = delete;
        static void run(const T **base, size_t arrlen) {
            qsort(base, arrlen, sizeof(T*), vcf);
        }
    };



    template<unsigned int sz> class bitset {
        private:
        unsigned int bits[(sz + 31)>>5]{};

        public:
        void add(unsigned int x) {
            if (x < sz) {
                bits[(x>>5)] |= 1u << (x & 31);
            }
        }

        void addAll(const bitset<sz> &rhs) {
            const unsigned int arrsz = (sz + 31)>>5;
            for(unsigned int i=0;i<arrsz;i++) {
                bits[i] |= rhs.bits[i];
            }
        }
        [[nodiscard]] bool contains(unsigned int x) const {
            if (x < sz)
                return (bits[(x>>5) & 7] & (1u << (x & 31))) != 0;
            else
                return false;
        }

        [[nodiscard]] unsigned int not_found() const {
            return sz;
        }

        /** Returns sz (or "not_found()" if not found */
        [[nodiscard]] unsigned int findNext(unsigned int startAt) const {
            //Can optimize this...
            for(unsigned int i=startAt; i < sz; i++) {
                if (contains(i)) {
                    return i;
                }
            }
            return sz;
        }
    };


};



#endif //XTRS_DPMHW_IMPL_H
