
#include "Options.h"
#include <ostream>

struct OptNFrames : public Option<int> {
    std::string  name()          const override { return "n-frames"; }
    std::string  description()   const override { return "Number of frames to process"; }
    Optional     default_value() const override { return 1000; }

    bool is_valid( std::string& error_message ) const override
    {
        if( raw_value() < 0 ) {
            error_message = "Number of frames must be non-negative. Specified value is " + std::to_string( raw_value() );
            return false;
        }
        return true;
    }
};




int main( int argc, const char** argv )
{
    auto options = Options().declare<OptNFrames>().parse( argc, argv );
    std::cout << "Processing " << options.get<OptNFrames>() << " frames" << std::endl;
    return 0;
}
