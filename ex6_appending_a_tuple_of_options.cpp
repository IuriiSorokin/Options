
#include "Options.h"
#include <ostream>
#include <tuple>

struct BasicAnalysis
{
    struct OptNFrames : public Option<int> {
    public:
        std::string  name()          const override { return "n-frames"; }
        std::string  description()   const override { return "Number of frames to process"; }
        Optional     default_value() const override { return 1000; }
    };

    struct OptOutFileName : public Option<std::string> {
    public:
        std::string  name()          const override { return "out-file"; }
        std::string  description()   const override { return "Output file name"; }
        Optional     default_value() const override { return std::string("hists.root"); }
    };

    using RequiredOptions = std::tuple< OptNFrames, OptOutFileName >;

    BasicAnalysis( const Options& options )
    {
        std::cout << "Constructing Analysis:" << std::endl;
        std::cout << "  N Frames to process = " << options.get<OptNFrames>()       << std::endl;
        std::cout << "  Output file         = " << options.get<OptOutFileName>()   << std::endl;
    }
};



struct ExtendedAnalysis : public BasicAnalysis
{
    struct OptMinElectronPt : public Option<double> {
        std::string  name()          const override { return "min-e-pt"; }
        std::string  description()   const override { return "Electron pt cut [MeV]"; }
        Optional     default_value() const override { return 12.7; }
    };

    using RequiredOptions = std::tuple< BasicAnalysis::RequiredOptions, OptMinElectronPt >;

    ExtendedAnalysis( const Options& options )
    : BasicAnalysis( options )
    {
        std::cout << "  Electron Pt cut     = " << options.get<OptMinElectronPt>() << std::endl;
    }
};


int main( int argc, const char** argv )
{
   ExtendedAnalysis( Options().declare<ExtendedAnalysis::RequiredOptions>().parse( argc, argv ) );
   return 0;
}
