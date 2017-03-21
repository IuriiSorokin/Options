
#include "Options.h"
#include <ostream>

struct OptNFrames : public Option<int> {
    std::string  name()          const override { return "n-frames"; }
    std::string  description()   const override { return "Number of frames to process"; }
    Optional     default_value() const override { return 1000; }
};

struct OptMinElectronPt : public Option<double> {
    std::string  name()          const override { return "min-e-pt"; }
    std::string  description()   const override { return "Electron pt cut [MeV]"; }
    Optional     default_value() const override { return 12.7; }
};

struct OptOutFileName : public Option<std::string> {
    std::string  name()          const override { return "out-file"; }
    std::string  description()   const override { return "Output file name"; }
    Optional     default_value() const override { return std::string("hists.root"); }
};


int main( int argc, const char** argv )
{
    Options opt;
    opt.declare<OptNFrames>();
    opt.declare<OptMinElectronPt>();
    opt.declare<OptOutFileName>();
    opt.parse( argc, argv );

    std::cout << "Processing       " << opt.get<OptNFrames>()       << " frames" << std::endl;
    std::cout << "Electron Pt cut: " << opt.get<OptMinElectronPt>() << std::endl;
    std::cout << "Output file:     " << opt.get<OptOutFileName>()   << std::endl;

    return 0;
}
