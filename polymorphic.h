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

namespace detail {

template< typename BaseType >
class PolymorphicStorageBase
{
public:
    using base_type = BaseType;

    virtual
    ~PolymorphicStorageBase() = default;

    virtual BaseType&
    get() = 0;

    virtual const BaseType&
    get() const = 0;

    virtual std::unique_ptr<PolymorphicStorageBase<BaseType>>
    clone() const = 0;
};



template< typename BaseType, typename ActualType >
class PolymorphicStorageImpl : public PolymorphicStorageBase< BaseType >
{
private:
    ActualType _object;

public:
    PolymorphicStorageImpl( ActualType object )
    : _object( object )
    {}

    virtual
    ~PolymorphicStorageImpl() = default;

    virtual BaseType&
    get() override
    {
        return *( dynamic_cast<BaseType*>(&_object) );
    }

    virtual const BaseType&
    get() const override
    {
        return *( dynamic_cast<const BaseType*>(&_object) );
    }

    virtual std::unique_ptr<PolymorphicStorageBase<BaseType>>
    clone() const override
    {
        return std::unique_ptr<PolymorphicStorageBase<BaseType>>( new PolymorphicStorageImpl<BaseType,ActualType>(*this) );
    }

};

}



template< typename BaseType >
class polymorphic
{
    std::unique_ptr< detail::PolymorphicStorageBase<BaseType> > _storage;

public:
    polymorphic()
    : _storage( nullptr )
    {}

    template<
        typename ActualType,
        typename std::enable_if< std::is_base_of< BaseType, ActualType >::value, bool >::type = true >
    polymorphic( ActualType&& val )
    :  _storage( new detail::PolymorphicStorageImpl<BaseType,ActualType>( val ) )
    {}

    polymorphic( const polymorphic<BaseType>& poly )
    : _storage( poly._storage->clone() )
    {}

    polymorphic( polymorphic<BaseType>&& poly )
    : _storage( std::move(poly._storage) )
    {}

    polymorphic<BaseType>&
    operator=( const polymorphic<BaseType>& other )
    {
        _storage = other._storage->clone();
        return *this;
    }

    polymorphic<BaseType>&
    operator=( polymorphic<BaseType>&& other )
    {
        _storage = std::move( other._storage );
        return *this;
    }

    BaseType&
    get()
    {
        assert( _storage && "Value is set" );
        return _storage->get();
    }

    const BaseType&
    get() const
    {
        assert( _storage && "Value is set" );
        return _storage->get();
    }

    template<typename ActualType>
    void
    set( ActualType val )
    {
        _storage.reset( new detail::PolymorphicStorageImpl<BaseType,ActualType>( val ) );
    }
};




#endif /* OPTIONS_POLYMORPHIC_H_ */
