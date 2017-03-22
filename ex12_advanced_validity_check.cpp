
#include "Options.h"
#include <ostream>
#include "OptHelp.h"

struct OptFieldMap : public Option<std::string> {
    std::string  name()          const override { return "field-map"; }
    std::string  description()   const override { return "Field map file"; }
    bool         is_valid( std::string& error_message ) const override;
};



struct OptUniformField: public Option<double> {
    std::string  name()          const override { return "uniform-field"; }
    std::string  description()   const override { return "Assume uniform field with the given strength [T], directed along z-axis"; }
    bool         is_valid( std::string& error_message ) const override;
};



inline bool OptFieldMap::is_valid( std::string& error_message ) const
{
    if( get_options().is_declared_and_set<OptUniformField>() ) {
        error_message = "Option " + name() + " may not be specified together with " + OptUniformField().name();
        return false;
    }
    return true;
}



inline bool OptUniformField::is_valid( std::string& error_message ) const
{
    if( get_options().is_declared_and_set<OptFieldMap>() ) {
        error_message = "Option " + name() + " may not be specified together with " + OptFieldMap().name();
        return false;
    }
    return true;
}



using FieldOptions = std::tuple<OptFieldMap, OptUniformField>;



int main( int argc, const char** argv )
{
    auto options = Options().declare<FieldOptions,OptHelp>().parse( argc, argv );

    options.get_option<OptHelp>().print_and_exit_if_set();

    if( options.is_set<OptFieldMap>() ) {
        std::cout << "Reading magnetic field from " << options.get<OptFieldMap>() << std::endl;
    }
    else {
        std::cout << "Assuming uniform field of strength " << options.get<OptUniformField>() << std::endl;;
    }

    return 0;
}



