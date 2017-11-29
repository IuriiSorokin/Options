
#include "Options.h"
#include <ostream>

struct OptNFrames : Option<int> {
    std::string  name()          const override { return "n-frames,N"; }
    std::string  description()   const override { return "Number of frames to process"; }
    Optional     default_value() const override { return 1000; }
};

struct OptMinElectronPt : Option<double> {
    std::string  name()          const override { return "min-e-pt"; }
    std::string  description()   const override { return "Cut on the electron transverse momentum [MeV]"; }
};

struct OptOutFileName : Option<std::string> {
    std::string  name()          const override { return "out-file,o"; }
    std::string  description()   const override { return "Output file name"; }
    Optional     default_value() const override { return std::string("results.root"); }
};

using AnalysisOptions = OptionList< OptNFrames, OptMinElectronPt, OptOutFileName >;

int main( int argc, const char** argv )
{
    auto options = Options().declare<AnalysisOptions>()
                            .parse( argc, argv );

    std::cout << "Processing       " << options.get_value<OptNFrames>() << " frames" << std::endl;

    std::cout << "Electron pt cut: " << ( options.is_set<OptMinElectronPt>() ? std::to_string( options.get_value<OptMinElectronPt>() )
                                                                             : std::string("<none>") ) << std::endl;

    std::cout << "Output file:     " << options.get_value<OptOutFileName>() << std::endl;

    return 0;
}
