#ifndef PROGRESS_ITERATOR_H
#define PROGRESS_ITERATOR_H

#include "thirdparty/boost/boost_disable_warnings.h"
#include "boost/iterator/iterator_adaptor.hpp"
#include "thirdparty/boost/boost_spirit_qstring_adapter.h"

#include <functional>

template<typename BaseItType>
class progress_iterator :
        public boost::iterator_adaptor<progress_iterator<BaseItType>, BaseItType>
{
private:
    std::function<void(size_t position)> _onPositionChangedFn;
    std::function<bool()> _cancelledFn;

    size_t _position = 0;

public:
    progress_iterator() : progress_iterator<BaseItType>::iterator_adaptor_() {}

    progress_iterator(const BaseItType& iterator) :
        progress_iterator<BaseItType>::iterator_adaptor_(iterator)
    {}

    template<typename OnPositionChangedFn>
    void onPositionChanged(OnPositionChangedFn&& onPositionChangedFn)
    {
        _onPositionChangedFn = onPositionChangedFn;
    }

    template<typename CancelledFn>
    void setCancelledFn(CancelledFn&& cancelledFn)
    {
        _cancelledFn = cancelledFn;
    }

    struct cancelled_exception {};

private:
    friend class boost::iterator_core_access;

    void increment()
    {
        this->base_reference()++;
        _position++;

        if(_onPositionChangedFn != nullptr)
            _onPositionChangedFn(_position);

        if(_cancelledFn != nullptr && _cancelledFn())
            throw cancelled_exception();
    }
};

#endif // PROGRESS_ITERATOR_H
