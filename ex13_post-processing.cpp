
#include "Options.h"
#include <ostream>
#include "OptHelp.h"

namespace detail {

inline
bool is_with_absolute_path( std::string pathName )
{
    return pathName.substr( 0, 1 ) == "/"   ||
            pathName.substr( 0, 2 ) == "~/" ||
            pathName.substr( 0, 2 ) == "./";
}

//inline
//bool is_with_path( std::string pathName ) {
//    return std::string::npos != pathName.find('/');
//}


}



struct OptDataDir : public Option<std::string> {
    std::string  name()          const override { return "data-dir"; }
    std::string  description()   const override { return "Input and output directory"; }
    std::string  with_trailing_slash() const;
};



struct OptInFile : public Option<std::string> {
    std::string  name()          const override { return "in-file"; }
    std::string  description()   const override { return "Input file"; }
    Optional     default_value() const override { return std::string("p2sim_m2_p00.root"); }
    bool         is_valid( std::string& error_message ) const override;
    std::string  with_data_dir() const;
};



struct OptOutFile : public Option<std::string> {
    std::string  name()          const override { return "out-file"; }
    std::string  description()   const override { return "Output file"; }
    Optional     default_value() const override { return std::string("hists.root"); }
    bool         is_valid( std::string& error_message ) const override;
    std::string  with_data_dir() const;
};



inline std::string
OptDataDir::with_trailing_slash() const
{
    const char last_char = *(value().rbegin());

    if( last_char == '/' ) {
        return value();
    }

    return value() + '/';
}



inline std::string
OptInFile::with_data_dir() const
{
    if( detail::is_with_absolute_path( value() ) ) {
        return value();
    }
    else {
        return get<OptDataDir>().with_trailing_slash() + value();
    }
}



inline std::string
OptOutFile::with_data_dir() const
{
    if( detail::is_with_absolute_path( value() ) ) {
        return value();
    }
    else {
        return get<OptDataDir>().with_trailing_slash() + value();
    }
}



bool OptInFile::is_valid( std::string& error_message ) const
{
    if( detail::is_with_absolute_path( value() ) && get_options().is_set<OptDataDir>() ) {
        error_message = OptInFile().name() + " Must not contain absolute path if " + OptDataDir().name() + " is specified";
        return false;
    }
    return true;
}



bool OptOutFile::is_valid( std::string& error_message ) const
{
    if( detail::is_with_absolute_path( value() ) && get_options().is_set<OptDataDir>() ) {
        error_message = OptOutFile().name() + " Must not contain absolute path if " + OptDataDir().name() + " is specified";
        return false;
    }
    return true;
}



using IOOptions = std::tuple< OptDataDir, OptInFile, OptOutFile >;



int main( int argc, const char** argv )
{
    auto options = Options().declare<IOOptions,OptHelp>().parse( argc, argv );
    options.get<OptHelp>().handle();
    options.print_values();
    return 0;
}



