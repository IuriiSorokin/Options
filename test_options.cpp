
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
        struct OptNFrames : public Option<int> {
            std::string name()        const override { return "n-frames"; }
            std::string description() const override { return "Number of frames to process"; }
        };
        BOOST_CHECK_EQUAL( OptNFrames().name_short(), 0 );
        BOOST_CHECK_EQUAL( OptNFrames().name_long(),  "n-frames" );
    }

    {
        struct OptNFrames : public Option<int> {
            std::string name()        const override { return "N,n-frames"; }
            std::string description() const override { return "Number of frames to process"; }
        };
        BOOST_CHECK_EQUAL( OptNFrames().name_short(), 'N' );
        BOOST_CHECK_EQUAL( OptNFrames().name_long(),  "n-frames" );
    }
}



BOOST_AUTO_TEST_CASE(declare_and_parse_one_option)
{
    struct OptNFrames : public Option<int> {
        std::string name()        const override { return "n-frames"; }
        std::string description() const override { return "Number of frames to process"; }
    };

    {
        Arguments arguments( { "--n-frames=10" } );

        const auto options =
                Options()
                .declare<OptNFrames>()
                .parse( arguments.Argc(), arguments.Argv() );

        BOOST_CHECK_EQUAL( options.get_value<OptNFrames>(), 10 );
    }

    {
        Arguments arguments( {} );

        const auto options =
                Options()
                .declare<OptNFrames>()
                .parse( arguments.Argc(), arguments.Argv() );

        bool exception_caught = false;
        try {
            options.get_value<OptNFrames>();
        } catch (const std::exception&) {
            exception_caught = true;
        }
        BOOST_CHECK(exception_caught);
    }

    {
        Arguments arguments( {"--n-frames=10"} );

        bool exception_caught = false;
        try {
            Options().parse( arguments.Argc(), arguments.Argv() );
        } catch (const std::exception&) {
            exception_caught = true;
        }
        BOOST_CHECK(exception_caught);
    }
}



BOOST_AUTO_TEST_CASE(declare_and_parse_tuple_of_options)
{
    struct OptNFrames : public Option<int> {
        std::string name()        const override { return "n-frames"; }
        std::string description() const override { return "Number of frames to process"; }
    };

    struct OptInFile: public Option<std::string> {
        std::string name()        const override { return "in-file"; }
        std::string description() const override { return "Input file name"; }
    };

    Arguments arguments( { "--n-frames=17",
                           "--in-file=some_input_file.txt" } );

    using MyOptions = std::tuple<OptNFrames, OptInFile>;

    const auto options =
            Options()
            .declare<MyOptions>()
            .parse( arguments.Argc(), arguments.Argv() );

    BOOST_CHECK_EQUAL( options.get_value<OptNFrames>(), 17 );
    BOOST_CHECK_EQUAL( options.get_value<OptInFile>(), "some_input_file.txt" );
}





BOOST_AUTO_TEST_CASE(default_value)
{
    struct OptNFrames : public Option<int> {
        std::string name()          const override { return "n-frames"; }
        std::string description()   const override { return "Number of frames to process"; }
        Optional    default_value() const override { return {100}; }
    };

    {
        Arguments arguments( { "" } );
        const auto options = Options().declare<OptNFrames>()
                                      .parse( arguments.Argc(), arguments.Argv() );
        BOOST_CHECK_EQUAL( options.get_value<OptNFrames>(), 100 );
    }

    {
        Arguments arguments( { "--n-frames", "83" } );

        const auto options =
                Options()
                .declare<OptNFrames>()
                .parse( arguments.Argc(), arguments.Argv() );

        BOOST_CHECK_EQUAL( options.get_value<OptNFrames>(), 83 );
    }
}



BOOST_AUTO_TEST_CASE(default_value_override_by_specified)
{
    struct OptNFrames : public Option<int> {
        std::string name()          const override { return "n-frames"; }
        std::string description()   const override { return "Number of frames to process"; }
        Optional    default_value() const override { return {100}; }
    };

    Arguments arguments( { "--n-frames", "83" } );
    const auto options = Options().declare<OptNFrames>()
                                  .parse( arguments.Argc(), arguments.Argv() );
    BOOST_CHECK_EQUAL( options.get_value<OptNFrames>(), 83 );
}



BOOST_AUTO_TEST_CASE(process_option_value)
{
    struct OptDataDir: public Option<std::string> {
        std::string name()        const override { return "data-dir"; }
        std::string description() const override { return "Path to the input file. Appended with trailing slash if was not specified."; }
        Optional    value()       const override {
            if( raw_value().is_initialized() and raw_value().get().back() != '/' ) {
                return raw_value().get() + '/';
            }
            return raw_value();
        }
    };

    {
        Arguments arguments( { "--data-dir=~/data/abc" } );
        const auto options = Options().declare<OptDataDir>()
                                      .parse( arguments.Argc(), arguments.Argv() );
        BOOST_CHECK_EQUAL( options.get_value<OptDataDir>(), "~/data/abc/" );
    }

    {
        Arguments arguments( { "--data-dir=~/data/abc/" } );
        const auto options = Options().declare<OptDataDir>()
                                      .parse( arguments.Argc(), arguments.Argv() );
        BOOST_CHECK_EQUAL( options.get_value<OptDataDir>(), "~/data/abc/" );
    }
}



BOOST_AUTO_TEST_CASE(use_value_of_other_option)
{
    struct OptDataDir: public Option<std::string> {
        std::string name()        const override { return "data-dir"; }
        std::string description() const override { return "Path to the input file. Appended with a trailing slash if it was missing."; }
        Optional    value()       const override {
            if( raw_value().is_initialized() and raw_value().get().back() != '/' ) {
                return raw_value().get() + '/';
            }
            return raw_value();
        }
    };

    struct OptInFile: public Option<std::string> {
        std::string name()        const override { return "in-file"; }
        std::string description() const override { return "Input file name. If no path is specified, then the path specified --data-dir path is prepended."; }
        Optional    value()       const override {
            if( not raw_value().is_initialized() ) {
                return Optional();
            }

            const bool contains_path = raw_value().get().find('/') != std::string::npos;

            if( not contains_path ) {
                const auto path = get_options()->get_value_or<OptDataDir>("");
                return path + raw_value().get();
            }

            return raw_value();
        }
    };

    using MyOptions = std::tuple<OptDataDir, OptInFile>;

    {
        Arguments arguments( { "--in-file=trololo.txt"  } );
        const auto options = Options().declare<MyOptions>()
                                      .parse( arguments.Argc(), arguments.Argv() );
        BOOST_CHECK_EQUAL( options.get_value<OptInFile>(), "trololo.txt" );
    }

    {
        Arguments arguments( { "--data-dir=~/data/abc",
                               "--in-file=trololo.txt"  } );
        const auto options = Options().declare<MyOptions>()
                                      .parse( arguments.Argc(), arguments.Argv() );
        BOOST_CHECK_EQUAL( options.get_value<OptInFile>(), "~/data/abc/trololo.txt" );
    }

    {
        Arguments arguments( { "--data-dir=~/data/abc",
                               "--in-file=./trololo.txt"  } );
        const auto options = Options().declare<MyOptions>()
                                      .parse( arguments.Argc(), arguments.Argv() );
        BOOST_CHECK_EQUAL( options.get_value<OptInFile>(), "./trololo.txt" );
    }
}



BOOST_AUTO_TEST_CASE(option_switch)
{
    struct OptBatch : public OptionSwitch {
        std::string name()        const override { return "b,batch"; }
        std::string description() const override { return "Run in batch mode"; }
        Optional    default_value() const override { return false; }
    };

    Arguments arguments( { "-b"} );
    // Arguments arguments( { "--batch"} );

    Options options;
    options.declare<OptBatch>();

    bool exception_caught = false;
    bool batch_mode       = false;

    try {
        options.parse( arguments.Argc(), arguments.Argv() );
        batch_mode = options.get_value<OptBatch>();
    } catch( const std::exception& ) {
        exception_caught = true;
    }

    BOOST_CHECK( batch_mode );
    BOOST_CHECK( not exception_caught );
}



BOOST_AUTO_TEST_CASE(derived_option)
{
    struct OptNFrames : public Option<int> {
        std::string name()          const override { return "n-frames"; }
        std::string description()   const override { return "Number of frames to process"; }
        Optional    default_value() const override { return 1000; }
    };

    struct OptNFramesConstrained : public OptNFrames {
        std::string name()          const override { return "n-frames"; }
        std::string description()   const override { return "Number of frames to process"; }
        Optional    default_value() const override { return 10; }
        Optional    value()         const override { return raw_value() > 100 ? 100 : raw_value(); }
    };

    {
        try {
            const auto options = Options()
                    .declare<OptNFrames>()
                    .declare<OptNFramesConstrained>();
            BOOST_FAIL("Must throw if derived option with the same names is declared after the base one.");
        } catch (const std::logic_error&) {}
    }

    {
        Options options;
        options.declare<OptNFramesConstrained>();
        try {
            options.declare<OptNFrames>();
        } catch (const std::logic_error&) {
            BOOST_FAIL("Must not throw if attempting to declare base option after the derived");
        }
        BOOST_CHECK( options.is_declared<OptNFrames>() );
        BOOST_CHECK( options.is_declared<OptNFramesConstrained>() );
        BOOST_CHECK_NE( dynamic_cast<OptNFramesConstrained*>( &(options.get<OptNFrames>() ) ), static_cast<OptNFramesConstrained*>(nullptr) );

        Arguments arguments( { "--n-frames=300"} );
        options.parse( arguments.Argc(), arguments.Argv() );
        BOOST_CHECK_EQUAL( options.get_value<OptNFrames>(), 100 );
        BOOST_CHECK_EQUAL( options.get_value<OptNFramesConstrained>(), 100 );
    }
}








