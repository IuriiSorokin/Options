
#include "Options.h"
#include <ostream>

struct OptNFrames : public Option<int> {
    std::string name()          const override { return "n-frames"; }
    std::string description()   const override { return "Number of frames to process"; }
    Optional    default_value() const override { return 1000; }
};

int main( int argc, const char** argv )
{
    Options opt;
    opt.declare<OptNFrames>();
    opt.parse( argc, argv );
    std::cout << "Processing " << opt.get<OptNFrames>() << " frames" << std::endl;
    return 0;
}