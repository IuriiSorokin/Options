/**
 *   ex05_accessing_other_options.cpp
 */

#include "Options.h"
#include "OptHelp.h"
#include <ostream>
#include <boost/filesystem.hpp>

struct OptDataDir : Option< boost::filesystem::path >
{
    std::string  name()        const override { return "data-dir,d"; }
    std::string  description() const override { return "Data directory"; }
};



struct OptFileWithImplicitDataDir : Option< boost::filesystem::path >
{
    std::string name() const override
    {
        return "data-file";
    }

    std::string description() const override
    {
        return std::string() + "If specified without absolute or relative path, "
                " then the " + OptDataDir().name_long_prefixed() + " is prepended.";
    }

    Optional value() const override
    {
        if( raw_value().get().has_parent_path() ) {
            return raw_value();
        }
        return get_options()->get_value<OptDataDir>() / raw_value().get();
    }
};



class BasicAnalysis
{
public:
    struct OptDataFile : OptFileWithImplicitDataDir
    {
        std::string name()         const override  { return "data-file"; }
        std::string description()  const override  { return std::string("Data file. ")
                                                            + OptFileWithImplicitDataDir().description(); }
        Optional    default_value() const override { return boost::filesystem::path("raw_data.root"); }
    };

    struct OptGeoFile : OptFileWithImplicitDataDir
    {
        std::string name()         const override  { return "geo-file"; }
        std::string description()  const override  { return std::string("Geometry file. ")
                                                            + OptFileWithImplicitDataDir().description(); }
        Optional    default_value() const override { return boost::filesystem::path("geometry.json"); }
    };

    struct OptOutFile : OptFileWithImplicitDataDir
    {
        std::string name()          const override { return "out-file"; }
        std::string description()   const override { return std::string("Output file. ")
                                                           + OptFileWithImplicitDataDir().description(); }
        Optional    default_value() const override { return boost::filesystem::path("results.json"); }
    };

    struct OptNFrames : Option<int> {
        std::string name()          const override { return "n-frames"; }
        std::string description()   const override { return "Number of frames to process"; }
        Optional    default_value() const override { return 1000; }
    };

    struct OptMinElectronPt : Option<double> {
        std::string name()          const override { return "min-e-pt"; }
        std::string description()   const override { return "Electron pt cut [MeV]"; }
        Optional    default_value() const override { return 12.5; }
    };

    using required_options = OptionList< OptDataDir,
                                         OptDataFile,
                                         OptGeoFile,
                                         OptOutFile,
                                         OptNFrames,
                                         OptMinElectronPt,
                                         OptHelp>;

    BasicAnalysis( const Options& options )
    {
        std::cout << "Constructing BasicAnalysis:" << std::endl;
        std::cout << "  Data dir            = " << options.get_value<OptDataDir>()       << std::endl;
        std::cout << "  Input data file     = " << options.get_value<OptDataFile>()      << std::endl;
        std::cout << "  Geometry file       = " << options.get_value<OptGeoFile>()       << std::endl;
        std::cout << "  Output file         = " << options.get_value<OptOutFile>()       << std::endl;
        std::cout << "  N Frames to process = " << options.get_value<OptNFrames>()       << std::endl;
        std::cout << "  Electron Pt cut     = " << options.get_value<OptMinElectronPt>() << std::endl;
    }
};



int main( int argc, const char** argv )
{
    BasicAnalysis( Options().declare<BasicAnalysis::required_options>()
                            .parse( argc, argv )
                            .call( []( const Options& options ){ options.get<OptHelp>().handle(); } ) );
    return 0;
}
