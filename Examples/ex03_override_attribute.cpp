/**
 *   ex03_override_attribute.cpp
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



struct ExtendedAnalysis : public BasicAnalysis
{
    struct OptOutFileName : Option<std::string> {
        std::string  name()          const override { return "out-file"; }
        std::string  description()   const override { return "Output file name"; }
        Optional     default_value() const override { return std::string("results.root"); }
    };

    struct OptMinElectronPt : BasicAnalysis::OptMinElectronPt {
        Optional     default_value() const override { return 25.4; }
    };

    using required_options = OptionList< OptMinElectronPt, // <-- this is ExtendedAnalysis::OptMinElectronPt
                                         BasicAnalysis::required_options,
                                         OptOutFileName >;

    ExtendedAnalysis( const Options& options )
    : BasicAnalysis( options )
    {
        std::cout << "Constructing ExtendedAnalysis:" << std::endl;
        std::cout << "  N Frames to process = " << options.get_value<OptNFrames>()       << std::endl;
        std::cout << "  Electron Pt cut     = " << options.get_value<OptMinElectronPt>() << std::endl;
        std::cout << "  Output file         = " << options.get_value<OptOutFileName>()   << std::endl;
    }
};


int main( int argc, const char** argv )
{
    ExtendedAnalysis( Options().declare<ExtendedAnalysis::required_options>().parse( argc, argv ) );
    return 0;
}
