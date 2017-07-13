
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
    Argv() const
    { return _argv.data(); }

    int
    Argc() const
    { return _argv.size(); }
};



BOOST_AUTO_TEST_CASE(short_and_long_name)
{
    {
        struct OptNElectrons : public Option<int> {
            std::string name()        const override { return "n-electrons"; }
            std::string description() const override { return "Number of electrons to simulate."; }
        };

        BOOST_CHECK_EQUAL( OptNElectrons().name_short(), 0 );
        BOOST_CHECK_EQUAL( OptNElectrons().name_long(),  "n-electrons" );
    }

    {
        struct OptNElectrons : public Option<int> {
            std::string name()        const override { return "n-electrons,N"; }
            std::string description() const override { return "Number of electrons to simulate."; }
        };

        BOOST_CHECK_EQUAL( OptNElectrons().name_short(), 'N' );
        BOOST_CHECK_EQUAL( OptNElectrons().name_long(),  "n-electrons" );
    }

    {
        struct OptNElectrons : public Option<int> {
            std::string name()        const override { return ",N"; }
            std::string description() const override { return "Number of electrons to simulate."; }
        };

        try{
            OptNElectrons().name_short();
            BOOST_FAIL("Must throw because there is no long name.");
        } catch( std::logic_error& e) {}

        try{
            OptNElectrons().name_long();
            std::cout << "name short=" << OptNElectrons().name_short() <<std::endl;
            std::cout << "name long=" << OptNElectrons().name_long() <<std::endl;
            BOOST_FAIL("Must throw because there is no long name.");
        } catch( std::logic_error& e) {}
    }

    {
        struct OptNElectrons : public Option<int> {
            std::string name()        const override { return "n-electrons,"; }
            std::string description() const override { return "Number of electrons to simulate."; }
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
        std::string description() const override { return "Number of electrons to simulate."; }
    };

    {
        Arguments args( {"--n-electrons=33"} );
        BOOST_CHECK_EQUAL( Options().declare<OptNElectrons>().parse( args.Argc(), args.Argv() ).get_value<OptNElectrons>(), 33 );
    }

    {
        Arguments args( {"--n-electrons", "17"} );
        BOOST_CHECK_EQUAL( Options().declare<OptNElectrons>().parse( args.Argc(), args.Argv() ).get_value<OptNElectrons>(), 17 );
    }

    {
        Arguments args( {"-N", "118"} );
        BOOST_CHECK_EQUAL( Options().declare<OptNElectrons>().parse( args.Argc(), args.Argv() ).get_value<OptNElectrons>(), 118 );
    }

    {
        Arguments args( {"-N0"} );
        BOOST_CHECK_EQUAL( Options().declare<OptNElectrons>().parse( args.Argc(), args.Argv() ).get_value<OptNElectrons>(), 0 );
    }

    {
        Arguments args( {} );
        try{
            Options().declare<OptNElectrons>().parse( args.Argc(), args.Argv() ).get_value<OptNElectrons>();
            BOOST_FAIL("Must throw because the value was not specified.");
        } catch( std::logic_error& e) {}
    }

    {
        Arguments args( {"-n", "22"} );
        try{
            Options().declare<OptNElectrons>().parse( args.Argc(), args.Argv() ).get_value<OptNElectrons>();
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
            Arguments args( {"--min-e-momentum=1.5"} );
            BOOST_CHECK_EQUAL( Options().declare<OptMinElectronMomentum>().parse( args.Argc(), args.Argv() ).get_value<OptMinElectronMomentum>(), 1.5 );
        }

        {
            Arguments args( {} );
            try{
                BOOST_CHECK_EQUAL( Options().declare<OptMinElectronMomentum>().parse( args.Argc(), args.Argv() ).get_value<OptMinElectronMomentum>(), 0.1 );
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
        Arguments args( {"--batch"} );
        BOOST_CHECK_EQUAL( Options().declare<OptBatch>().parse( args.Argc(), args.Argv() ).get_value<OptBatch>(), true );
    }

    {
        Arguments args( {"-b"} );
        BOOST_CHECK_EQUAL( Options().declare<OptBatch>().parse( args.Argc(), args.Argv() ).get_value<OptBatch>(), true );
    }

    {
        Arguments args( {} );
        BOOST_CHECK_EQUAL( Options().declare<OptBatch>().parse( args.Argc(), args.Argv() ).get_value<OptBatch>(), false );
    }

    {
        Arguments args( {"-b0"} );
        BOOST_CHECK_EQUAL( Options().declare<OptBatch>().parse( args.Argc(), args.Argv() ).get_value<OptBatch>(), false );
    }

    {
        Arguments args( {"--batch=0"} );
        BOOST_CHECK_EQUAL( Options().declare<OptBatch>().parse( args.Argc(), args.Argv() ).get_value<OptBatch>(), false);
    }

    {
        Arguments args( {"-b1"} );
        BOOST_CHECK_EQUAL( Options().declare<OptBatch>().parse( args.Argc(), args.Argv() ).get_value<OptBatch>(), true );
    }

    {
        Arguments args( {"--batch=1"} );
        BOOST_CHECK_EQUAL( Options().declare<OptBatch>().parse( args.Argc(), args.Argv() ).get_value<OptBatch>(), true);
    }
}



BOOST_AUTO_TEST_CASE(declare_and_parse_tuple_of_options)
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

    using IOOptions          = std::tuple< OptInFile, OptOutFile >;
    using SimulationOptions  = std::tuple< OptNElectrons, OptMinElectronMomentum, IOOptions >;
    using ApplicationOptions = std::tuple< SimulationOptions, OptBatch >;

    {
        Arguments args( { "--in-file", "xxx.txt",
                          "--out-file", "yyy.txt",
                          "--min-e-momentum=3.62",
                          "-N", "160"    } );
        const auto opt = Options().declare<ApplicationOptions>().parse( args.Argc(), args.Argv() );
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



BOOST_AUTO_TEST_CASE(process_option_value)
{
    struct OptDataDir: public Option<std::string> {
        std::string name()        const override { return "data-dir"; }
        std::string description() const override { return "Path to the input file. Appended with trailing slash if was not specified."; }
        Optional    value()       const override { return raw_value().is_initialized() ? append_slash_if_missing( raw_value().get() ) : Optional(); }
        std::string append_slash_if_missing( std::string s ) const { return ( s.back() != '/' ) ? (s + '/') : s; }
    };

    {
        Arguments args( { "--data-dir=~/data/abc" } );
        BOOST_CHECK_EQUAL( Options().declare<OptDataDir>().parse( args.Argc(), args.Argv() ).get_value<OptDataDir>(), "~/data/abc/" );
    }

    {
        Arguments args( { "--data-dir=~/data/abc/" } );
        BOOST_CHECK_EQUAL( Options().declare<OptDataDir>().parse( args.Argc(), args.Argv() ).get_value<OptDataDir>(), "~/data/abc/" );
    }
}



BOOST_AUTO_TEST_CASE(use_value_of_other_option)
{
    struct OptDataDir: public Option<std::string> {
        std::string name()        const override { return "data-dir"; }
        std::string description() const override { return "Path to the input file. Appended with trailing slash if was not specified."; }
        std::string append_slash_if_missing( std::string s ) const { return ( s.back() != '/' ) ? (s + '/') : s; }
        Optional    value()       const override { return raw_value().is_initialized() ? append_slash_if_missing( raw_value().get() ) : Optional(); }
    };

    struct OptInFile: public Option<std::string> {
        std::string name()        const override { return "in-file"; }
        std::string description() const override { return "Input file name. If no path is specified, then the path specified --data-dir path is prepended."; }
        std::string prepend_path_if_none( std::string name, std::string path ) const { return ( name.find('/') == std::string::npos ) ? path + name : name; };
        Optional    value()       const override { return raw_value().is_initialized() ? prepend_path_if_none( raw_value().get(), get_options()->get_value_or<OptDataDir>("") ) : Optional(); }
    };

    {
        Arguments args( { "--in-file=trololo.txt"  } );
        BOOST_CHECK_EQUAL( Options().declare<OptDataDir>().declare<OptInFile>().parse( args.Argc(), args.Argv()).get_value<OptInFile>(), "trololo.txt" );
    }

    {
        Arguments args( { "--data-dir=~/data/abc",
                          "--in-file=trololo.txt"  } );
        BOOST_CHECK_EQUAL( Options().declare<OptDataDir>().declare<OptInFile>().parse( args.Argc(), args.Argv()).get_value<OptInFile>(), "~/data/abc/trololo.txt" );
    }

    {
        Arguments args( { "--data-dir=~/data/abc",
                          "--in-file=./trololo.txt"  } );
        BOOST_CHECK_EQUAL( Options().declare<OptDataDir>().declare<OptInFile>().parse( args.Argc(), args.Argv()).get_value<OptInFile>(), "./trololo.txt" );
    }
}



BOOST_AUTO_TEST_CASE(derived_option)
{
    struct OptMinElectronMomentum : public Option<double> {
        std::string name()        const override   { return "min-e-momentum"; }
        std::string description() const override   { return "Minimal electron momentum [MeV/c]."; }
        Optional    default_value() const override { return 0.1; }
    };

    struct OptMinElectronMomentumConstrained : public OptMinElectronMomentum {
        Optional    value() const override {
            if( raw_value().is_initialized()
                    and ( raw_value().get() < 0
                            or raw_value().get() > 100 ) ) {
                throw std::invalid_argument( "Minimal electron momentum must be within between 0 and 100 MeV/c");
            }
            return raw_value();
        }
    };

    {
        try {
            Options().declare<OptMinElectronMomentum>().declare<OptMinElectronMomentumConstrained>();
            BOOST_FAIL("Must throw if derived option with the same names is declared after the base one.");
        } catch (const std::logic_error&) {}
    }

    {
        Options options;
        try {
            options.declare<OptMinElectronMomentumConstrained>().declare<OptMinElectronMomentum>();
        } catch (const std::logic_error&) {
            BOOST_FAIL("Must not throw if declaring base option after the derived");
        }

        BOOST_CHECK( options.is_declared<OptMinElectronMomentum>() );
        BOOST_CHECK( options.is_declared<OptMinElectronMomentumConstrained>() );
        BOOST_CHECK( nullptr != dynamic_cast<OptMinElectronMomentumConstrained*>( &(options.get<OptMinElectronMomentum>() ) ) );

        Arguments args( { "--min-e-momentum", "-1.2"} );
        options.parse( args.Argc(), args.Argv() );
        try{
            options.get_value<OptMinElectronMomentum>();
            BOOST_FAIL("Must throw because the specified min-e-momentum is invalid.");
        } catch (const std::invalid_argument&) {}
    }
}








