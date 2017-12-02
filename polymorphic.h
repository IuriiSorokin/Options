/*
 * polymorphic.h
 *
 *  Created on: Mar 21, 2017
 *      Author: sorokin
 */

#ifndef POLYMORPHIC_H_
#define POLYMORPHIC_H_

#include <memory>
#include <assert.h>
#include <ostream>
#include <iostream>

namespace detail {

template< typename BaseT >
class wrapper_base
{
public:
    using base_type = BaseT;

    virtual
    ~wrapper_base() = default;

    virtual BaseT&
    get() = 0;

    virtual const BaseT&
    get() const = 0;

    virtual std::unique_ptr<wrapper_base<BaseT>>
    clone() const = 0;

    virtual bool
    is_dynamic_castable_to_actual( const BaseT& other ) const = 0;
};



template< typename BaseT, typename ActualT >
class wrapper_impl : public wrapper_base< BaseT >
{
private:
    ActualT _object;

public:
    wrapper_impl( ActualT&& object )
    : _object( std::move(object ) )
    {}

    virtual
    ~wrapper_impl() = default;

    virtual BaseT&
    get() override
    {
        return static_cast<BaseT&>(_object);
    }

    virtual const BaseT&
    get() const override
    {
        return static_cast<const BaseT&>(_object);
    }

    virtual std::unique_ptr<wrapper_base<BaseT>>
    clone() const override
    {
        return std::unique_ptr<wrapper_base<BaseT>>( new wrapper_impl<BaseT,ActualT>(*this) );
    }

    virtual bool
    is_dynamic_castable_to_actual( const BaseT& other ) const override
    {
        return nullptr != dynamic_cast<const ActualT*>( &other );
    }
};

}



template< typename BaseT >
class polymorphic
{
    std::unique_ptr< detail::wrapper_base<BaseT> > _object;

public:
    template< typename ActualT,
              std::enable_if_t< std::is_base_of<BaseT, ActualT>::value, int > = 0 >
    polymorphic( ActualT&& val )
    :  _object( new detail::wrapper_impl<BaseT,ActualT>( std::move(val) ) )
    {}

    polymorphic( const polymorphic<BaseT>& poly )
    : _object( poly._object->clone() )
    {}

    polymorphic( polymorphic<BaseT>&& poly )
    : _object( std::move(poly._object) )
    {}

    polymorphic<BaseT>&
    operator=( const polymorphic<BaseT>& other )
    {
        _object = other._object->clone();
        return *this;
    }

    polymorphic<BaseT>&
    operator=( polymorphic<BaseT>&& other )
    {
        _object = std::move( other._object );
        return *this;
    }

    BaseT&
    get()
    {
        assert( _object && "Value is set" );
        return _object->get();
    }

    const BaseT&
    get() const
    {
        assert( _object && "Value is set" );
        return _object->get();
    }

    BaseT*
    operator->()
    {
        assert( _object && "Value is set" );
        return &(_object->get());
    }

    const BaseT*
    operator->() const
    {
        assert( _object && "Value is set" );
        return &(_object->get());
    }

    template<typename ActualT>
    void
    set( ActualT&& val )
    {
        _object.reset( new detail::wrapper_impl<BaseT,ActualT>( val ) );
    }

    bool
    is_dynamic_castable_to_actual( const BaseT& other ) const
    {
        return _object->is_dynamic_castable_to_actual( other );
    }
};




#endif /* OPTIONS_POLYMORPHIC_H_ */
