
#define BOOST_TEST_MODULE polymorphic test
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include "polymorphic.h"

struct A {
    virtual ~A() = default;
    virtual int f() const { return 3; }
};

struct B : A
{
    virtual int f() const override { return 17; }
};

struct C : A
{
    virtual int f() const override { return 26; }
};


BOOST_AUTO_TEST_CASE(polymorphism)
{
    polymorphic<A> a( A{} );
    polymorphic<A> b( B{} );

    BOOST_CHECK_EQUAL( typeid(a.get()).name(), typeid(A).name() );
    BOOST_CHECK_EQUAL( typeid(b.get()).name(), typeid(B).name() );

    BOOST_CHECK_EQUAL( a.get().f(),  3 );
    BOOST_CHECK_EQUAL( b.get().f(), 17 );
}



BOOST_AUTO_TEST_CASE(copy)
{
    polymorphic<A> a( A{} );
    polymorphic<A> b( B{} );

    auto a_copy = a;
    auto b_copy = b;

    BOOST_CHECK_EQUAL( a_copy.get().f(),  3 );
    BOOST_CHECK_EQUAL( b_copy.get().f(), 17 );
}



BOOST_AUTO_TEST_CASE(is_dynamic_castable)
{
    polymorphic<A> a( A{} );
    polymorphic<A> b( B{} );
    polymorphic<A> c( C{} );

    BOOST_CHECK( a.is_dynamic_castable_to_actual( A{} ) );
    BOOST_CHECK( a.is_dynamic_castable_to_actual( B{} ) );
    BOOST_CHECK( a.is_dynamic_castable_to_actual( C{} ) );

    BOOST_CHECK( not b.is_dynamic_castable_to_actual( A{} ) );
    BOOST_CHECK(     b.is_dynamic_castable_to_actual( B{} ) );
    BOOST_CHECK( not b.is_dynamic_castable_to_actual( C{} ) );

    BOOST_CHECK( not c.is_dynamic_castable_to_actual( A{} ) );
    BOOST_CHECK( not c.is_dynamic_castable_to_actual( B{} ) );
    BOOST_CHECK(     c.is_dynamic_castable_to_actual( C{} ) );
}

