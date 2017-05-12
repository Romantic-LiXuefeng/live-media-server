#ifndef DSHAREDPTR_HPP
#define DSHAREDPTR_HPP

#include <stdlib.h>

/**
 * 使用原子操作实现的线程安全的sharedptr，摘自boost
 */

template<typename T>
class DSharedPtr
{
public:
    explicit DSharedPtr(T *p = 0)
        : px(p)
    {
        pn = new int(1);
    }

    template<typename Y>
    DSharedPtr(Y *p)
    {
        pn = new int(1);
        px = p;
    }

    DSharedPtr(const DSharedPtr &r)
        : px(r.px)
    {
        __sync_add_and_fetch(r.pn, 1);
        pn = r.pn;
    }

    template<typename Y>
    DSharedPtr(const DSharedPtr<Y> &r)
    {
        px = r.px;
        __sync_add_and_fetch(r.pn, 1);
        pn = r.pn;
    }

    ~DSharedPtr()
    {
        dispose();
    }

public:
    DSharedPtr& operator=(const DSharedPtr &r)
    {
        if(this== &r) {
            return *this;
        }
        dispose();
        px = r.px;
        __sync_add_and_fetch(r.pn, 1);
        pn = r.pn;

        return *this;
    }

    template<typename Y>
    DSharedPtr& operator=(const DSharedPtr<Y> &r)
    {
        dispose();
        px = r.px;
        __sync_add_and_fetch(r.pn, 1);
        pn = r.pn;
        return *this;
    }

    T operator*()const
    {
        return *px;
    }

    T* operator->()const
    {
        return px;
    }

    T* get() const
    {
        return px;
    }

private:
    void dispose()
    {
        if (!__sync_sub_and_fetch(pn, 1)) {
            delete px;
            delete pn;
        }
    }

private:
    T *px;
    int *pn;
};

template<typename A,typename B>
inline bool operator==(DSharedPtr<A> const &l, DSharedPtr<B> const &r)
{
    return l.get() == r.get();
}

template<typename A,typename B>
inline bool operator!=(DSharedPtr<A> const &l, DSharedPtr<B> const &r)
{
    return l.get() != r.get();
}

#endif // DSHAREDPTR_HPP
