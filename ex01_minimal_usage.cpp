
#include "Options.h"
#include <iostream>

struct OptNFrames : Option<int> {
    std::string name()        const override { return "n-frames,N"; }
    std::string description() const override { return "Number of frames to process"; }
};

int main( int argc, const char** argv )
{
    auto options = Options().declare<OptNFrames>()
                            .parse( argc, argv );

    auto n_frames = options.get_value<OptNFrames>();

    std::cout << "N Frames = " << n_frames << std::endl;

    return 0;
}
