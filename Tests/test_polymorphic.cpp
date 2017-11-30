
#define BOOST_TEST_MODULE polymorphic test
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include "polymorphic.h"

BOOST_AUTO_TEST_CASE(preserve_type)
{
    struct A {
        virtual ~A() = default;
    };

    struct B : A {};

    B b;

    polymorphic<A> pa( b );

    BOOST_CHECK_EQUAL( typeid(pa.get()).name(), typeid(B).name() );
}



BOOST_AUTO_TEST_CASE(preserve_state)
{
    struct A {
        virtual ~A() = default;
        int v = 0;
    };

    struct B : A {};

    B b;
    b.v = 123;

    polymorphic<A> pa( b );
    pa.get().v = -22;

    BOOST_CHECK_EQUAL( pa.get().v, -22);
    BOOST_CHECK_EQUAL( b.v, 123);
}
