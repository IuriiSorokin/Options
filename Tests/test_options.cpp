
#define BOOST_TEST_MODULE Options test
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include "Options.h"

class Arguments
{
    std::string _name = "executable";
    std::vector< std::string > _arguments;
    std::vector< const char* > _argv;

public:
    Arguments( std::vector< std::string > arguments )
: _arguments( arguments )
{
        _argv.reserve( arguments.size() + 1 );
        _argv.push_back(_name.data());
        for( const auto& arg : _arguments ) {
            _argv.push_back( arg.data() );
        }
}
    Arguments( const Arguments& ) = delete;
    Arguments( Arguments&& ) = delete;

    const char* const *
    argv() const
    { return _argv.data(); }

    int
    argc() const
    { return _argv.size(); }
};



BOOST_AUTO_TEST_CASE(short_and_long_name)
{
    {
        struct OptNElectrons : public Option<int> {
            std::string name()        const override { return "n-electrons"; }
        };

        BOOST_CHECK_EQUAL( OptNElectrons().name_short(), 0 );
        BOOST_CHECK_EQUAL( OptNElectrons().name_long(),  "n-electrons" );
    }

    {
        struct OptNElectrons : public Option<int> {
            std::string name()        const override { return "n-electrons,N"; }
        };

        BOOST_CHECK_EQUAL( OptNElectrons().name_short(), 'N' );
        BOOST_CHECK_EQUAL( OptNElectrons().name_long(),  "n-electrons" );
    }

    {
        struct OptNElectrons : public Option<int> {
            std::string name()        const override { return ",N"; }
        };

        try{
            OptNElectrons().name_short();
            BOOST_FAIL("Must throw because there is no long name.");
        } catch( std::logic_error& e) {}

        try{
            OptNElectrons().name_long();
            BOOST_FAIL("Must throw because there is no long name.");
        } catch( std::logic_error& e) {}
    }

    {
        struct OptNElectrons : public Option<int> {
            std::string name()        const override { return "n-electrons,"; } // ! notice the trailing comma in the name
        };

        try{
            OptNElectrons().name_short();
            BOOST_FAIL("Must throw because there is comma, but no short name.");
        } catch( std::logic_error& e) {}

        try{
            OptNElectrons().name_long();
            BOOST_FAIL("Must throw because there is comma, but no short name.");
        } catch( std::logic_error& e) {}
    }
}



BOOST_AUTO_TEST_CASE(declare_and_parse_one_option)
{

    struct OptNElectrons : public Option<int> {
        std::string name()        const override { return "n-electrons,N"; }
    };

    {
        Arguments a( {"--n-electrons=33"} );
        BOOST_CHECK_EQUAL( Options().declare<OptNElectrons>().parse( a.argc(), a.argv() ).get_value<OptNElectrons>(), 33 );
    }

    {
        Arguments a( {"--n-electrons", "17"} );
        BOOST_CHECK_EQUAL( Options().declare<OptNElectrons>().parse( a.argc(), a.argv() ).get_value<OptNElectrons>(), 17 );
    }

    {
        Arguments a( {"-N", "118"} );
        BOOST_CHECK_EQUAL( Options().declare<OptNElectrons>().parse( a.argc(), a.argv() ).get_value<OptNElectrons>(), 118 );
    }

    {
        Arguments a( {"-N0"} );
        BOOST_CHECK_EQUAL( Options().declare<OptNElectrons>().parse( a.argc(), a.argv() ).get_value<OptNElectrons>(), 0 );
    }

    {
        Arguments a( {} );
        try{
            Options().declare<OptNElectrons>().parse( a.argc(), a.argv() ).get_value<OptNElectrons>();
            BOOST_FAIL("Must throw because the value was not specified.");
        } catch( std::logic_error& e) {}
    }

    {
        Arguments a( {"-n", "22"} );
        try{
            Options().declare<OptNElectrons>().parse( a.argc(), a.argv() ).get_value<OptNElectrons>();
            BOOST_FAIL("Must throw because there is no option '-n'.");
        } catch( std::logic_error& e) {}
    }
}





BOOST_AUTO_TEST_CASE(default_value)
{
    {
        struct OptMinElectronMomentum : public Option<double> {
            std::string name()        const override   { return "min-e-momentum"; }
            std::string description() const override   { return "Minimal electron momentum [MeV/c]."; }
            Optional    default_value() const override { return 0.1; }
        };

        {
            Arguments a( {"--min-e-momentum=1.5"} );
            BOOST_CHECK_EQUAL( Options().declare<OptMinElectronMomentum>().parse( a.argc(), a.argv() ).get_value<OptMinElectronMomentum>(), 1.5 );
        }

        {
            Arguments a( {} );
            try{
                BOOST_CHECK_EQUAL( Options().declare<OptMinElectronMomentum>().parse( a.argc(), a.argv() ).get_value<OptMinElectronMomentum>(), 0.1 );
            } catch( std::logic_error& e) {
                BOOST_FAIL("Must not throw as the default value was specified.");
            }
        }
    }
}



BOOST_AUTO_TEST_CASE(option_switch)
{
    struct OptBatch : public OptionSwitch {
        std::string name()        const override { return "batch,b"; }
        std::string description() const override { return "Run in batch mode"; }
    };

    {
        Arguments a( {"--batch"} );
        BOOST_CHECK_EQUAL( Options().declare<OptBatch>().parse( a.argc(), a.argv() ).get_value<OptBatch>(), true );
    }

    {
        Arguments a( {"-b"} );
        BOOST_CHECK_EQUAL( Options().declare<OptBatch>().parse( a.argc(), a.argv() ).get_value<OptBatch>(), true );
    }

    {
        Arguments a( {} );
        BOOST_CHECK_EQUAL( Options().declare<OptBatch>().parse( a.argc(), a.argv() ).get_value<OptBatch>(), false );
    }

    {
        Arguments a( {"-b0"} );
        BOOST_CHECK_EQUAL( Options().declare<OptBatch>().parse( a.argc(), a.argv() ).get_value<OptBatch>(), false );
    }

    {
        Arguments a( {"--batch=0"} );
        BOOST_CHECK_EQUAL( Options().declare<OptBatch>().parse( a.argc(), a.argv() ).get_value<OptBatch>(), false);
    }

    {
        Arguments a( {"-b1"} );
        BOOST_CHECK_EQUAL( Options().declare<OptBatch>().parse( a.argc(), a.argv() ).get_value<OptBatch>(), true );
    }

    {
        Arguments a( {"--batch=1"} );
        BOOST_CHECK_EQUAL( Options().declare<OptBatch>().parse( a.argc(), a.argv() ).get_value<OptBatch>(), true);
    }
}



BOOST_AUTO_TEST_CASE(declare_and_parse_list_of_options)
{
    struct OptNElectrons : public Option<int> {
        std::string name()        const override { return "n-electrons,N"; }
        std::string description() const override { return "Number of electrons to simulate."; }
    };

    struct OptMinElectronMomentum : public Option<double> {
        std::string name()        const override   { return "min-e-momentum"; }
        std::string description() const override   { return "Minimal electron momentum [MeV/c]."; }
        Optional    default_value() const override { return 0.1; }
    };

    struct OptInFile: public Option<std::string> {
        std::string name()        const override { return "in-file"; }
        std::string description() const override { return "Input file name."; }
    };

    struct OptOutFile: public Option<std::string> {
        std::string name()        const override { return "out-file"; }
        std::string description() const override { return "Output file name."; }
    };

    struct OptBatch : public OptionSwitch {
        std::string name()        const override { return "batch,b"; }
        std::string description() const override { return "Run in batch mode"; }
    };

    using IOOptions          = OptionList< OptInFile, OptOutFile >;
    using SimulationOptions  = OptionList< OptNElectrons, OptMinElectronMomentum, IOOptions >;
    using ApplicationOptions = OptionList< SimulationOptions, OptBatch >;

    {
        Arguments a( { "--in-file", "xxx.txt",
                          "--out-file", "yyy.txt",
                          "--min-e-momentum=3.62",
                          "-N", "160"    } );
        const auto opt = Options().declare<ApplicationOptions>().parse( a.argc(), a.argv() );
        BOOST_CHECK( opt.is_declared<OptNElectrons>() );
        BOOST_CHECK( opt.is_declared<OptMinElectronMomentum>() );
        BOOST_CHECK( opt.is_declared<OptInFile>() );
        BOOST_CHECK( opt.is_declared<OptOutFile>() );
        BOOST_CHECK( opt.is_declared<OptBatch>() );
        BOOST_CHECK_EQUAL( opt.get_value<OptNElectrons>(), 160 );
        BOOST_CHECK_EQUAL( opt.get_value<OptMinElectronMomentum>(), 3.62 );
        BOOST_CHECK_EQUAL( opt.get_value<OptBatch>(), false );
        BOOST_CHECK_EQUAL( opt.get_value<OptInFile>(),  "xxx.txt" );
        BOOST_CHECK_EQUAL( opt.get_value<OptOutFile>(), "yyy.txt" );
    }
}



BOOST_AUTO_TEST_CASE(use_value_of_other_option)
{
    struct OptA: public Option<int> {
        std::string name()        const override { return "A"; }
    };

    struct OptSameAsAByDefault: public Option<int> {
        std::string name()        const override { return "B"; }
        Optional    value()       const override { return raw_value().is_initialized() ? raw_value() : get_options()->get<OptA>().value(); }
    };

    {
        Arguments a( { "--A=12"  } );
        auto options = Options().declare<OptA, OptSameAsAByDefault>().parse( a.argc(), a.argv());
        BOOST_CHECK_EQUAL( options.get_value<OptSameAsAByDefault>(), 12 );
    }

    {
        Arguments a( { "--A=12", "--B=3"  } );
        auto options = Options().declare<OptA, OptSameAsAByDefault>().parse( a.argc(), a.argv());
        BOOST_CHECK_EQUAL( options.get_value<OptSameAsAByDefault>(), 3 );
    }
}



BOOST_AUTO_TEST_CASE(derived_replaces_base_independent_of_declaration_order)
{
    struct OptBase : Option<double> {
        std::string name() const override { return "base"; }
    };

    struct OptDerived : OptBase {
    };

    auto options_base_first    = Options().declare<OptBase>().declare<OptDerived>();
    auto options_derived_first = Options().declare<OptDerived>().declare<OptBase>();
    BOOST_CHECK( options_base_first.is_declared<OptDerived>() );
    BOOST_CHECK( options_derived_first.is_declared<OptDerived>() );
    BOOST_CHECK_EQUAL( typeid(options_base_first.get<OptBase>()).name(),    typeid(OptDerived).name() );
    BOOST_CHECK_EQUAL( typeid(options_derived_first.get<OptBase>()).name(), typeid(OptDerived).name() );
}



BOOST_AUTO_TEST_CASE(two_options_with_same_name_not_allowed)
{
    struct OptBase : Option<double> {
        std::string name() const override { return "base"; }
    };

    struct OptDerived1 : OptBase {
    };

    struct OptDerived2 : OptBase {
    };

    try {
        Options().declare<OptBase, OptDerived1, OptDerived2>();
        BOOST_FAIL("Must throw as two options with same name are not allowed");
    }
    catch (std::logic_error& ){
    }
}



BOOST_AUTO_TEST_CASE(two_options_with_different_names_are_allowed)
{
    struct OptBase : Option<double> {
        std::string name() const override { return "base"; }
    };

    struct OptDerived1 : OptBase {
        std::string name() const override { return "derived1"; }
    };

    struct OptDerived2 : OptBase {
        std::string name() const override { return "derived2"; }
    };

    Options().declare<OptDerived1, OptDerived2>();
}



BOOST_AUTO_TEST_CASE(replacing_by_option_with_different_name_not_allowed)
{
    struct OptBase : Option<double> {
        std::string name() const override { return "base"; }
    };

    struct OptDerived : OptBase {
        std::string name() const override { return "derived"; }
    };

    try {
        Options().declare<OptBase, OptDerived>();
        BOOST_FAIL("Must throw when declaring a derived option with overridden name.");
    }
    catch (std::logic_error& ){
    }
}



