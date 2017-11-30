/**
 *   ex02a_option_list.cpp
 */

#include "Options.h"
#include <ostream>

class BasicAnalysis
{
public:
    struct OptNFrames : Option<int> {
        std::string  name()          const override { return "n-frames"; }
        std::string  description()   const override { return "Number of frames to process"; }
        Optional     default_value() const override { return 1000; }
    };

    struct OptMinElectronPt : Option<double> {
        std::string  name()          const override { return "min-e-pt"; }
        std::string  description()   const override { return "Electron pt cut [MeV]"; }
        Optional     default_value() const override { return 12.7; }
    };

    using required_options = OptionList< OptNFrames, OptMinElectronPt >;

    BasicAnalysis( const Options& options )
    {
        std::cout << "Constructing BasicAnalysis:" << std::endl;
        std::cout << "  N Frames to process = " << options.get_value<OptNFrames>()       << std::endl;
        std::cout << "  Electron Pt cut     = " << options.get_value<OptMinElectronPt>() << std::endl;
    }
};



int main( int argc, const char** argv )
{
    BasicAnalysis( Options().declare<BasicAnalysis::required_options>().parse( argc, argv ) );
    return 0;
}
