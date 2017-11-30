/*
 * clonetest.cpp
 *
 *  Created on: Mar 20, 2017
 *      Author: sorokin
 */

#include "polymorphic.h"

#include <iostream>
#include <typeinfo>
#include <vector>

struct Base
{
    virtual ~Base() = default;
};

struct DerivedA : public Base {};

struct DerivedB : public Base {};

struct ReimplA : public DerivedA {};


int main( int, const char**)
{
    auto p = polymorphic<Base>( DerivedA() );

    std::cout << "p contains " << typeid( p.get() ).name() << std::endl;

    p.set( DerivedB() );
    std::cout << "p contains " << typeid( p.get() ).name() << std::endl;

    p.set( ReimplA() );
    std::cout << "p contains " << typeid( p.get() ).name() << std::endl;


    auto p2 = polymorphic<Base>( p );

    std::cout << "p2 contains " << typeid( p2.get() ).name() << std::endl;
    std::cout << "p  contains " << typeid( p.get()  ).name() << std::endl;

    auto p3 = polymorphic<Base>( std::move(p2) );

    std::cout << "p3 contains " << typeid( p3.get() ).name() << std::endl;

    std::vector< polymorphic<Base> > v;
    v.push_back( polymorphic<Base>( Base() ) );
    v.push_back( polymorphic<Base>( DerivedA() ) );
    v.push_back( polymorphic<Base>( DerivedB() ) );
    v.push_back( polymorphic<Base>( ReimplA() ) );

    for( const auto& poly : v ) {
        std::cout << "poly contains " << typeid( poly.get() ).name() << std::endl;
    }

    const auto v2( v );

    for( const auto& poly : v2 ) {
        std::cout << "poly contains " << typeid( poly.get() ).name() << std::endl;
    }


}



