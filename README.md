# Options #
This project is wrapper around [```boost::program_options```](http://www.boost.org/doc/libs/1_63_0/doc/html/program_options.html), designed with the aim to make it easy to manage and reuse any sizeable number of options.

## Main features
Every option is a class, and this brings several advantages:

* Every option is defined in one place, and can be re-used in several executables.
* Options are referred to by the class name, and not by a string literal.
* Option value can be post-processed using other options. E.g. you can have options ```DataDirectory```, ```InputFile```, and ```OutputFile```, and you can make ```InputFile``` and ```OutputFile``` to automatically prepend the specified ```DataDirectory```.
* The post-processing logic (including validity checking) can be encapsulated. The implementation where the option is defined, and not where the option is used.
* Options can be combined in groups like: ```using IOOptions = std::tuple< DataDirectory, InputFile, OutputFile >;```, 
  also in a nested manner, like: ```using MyAnalysisOptions = std::tuple< IOOptions, BeamEnergy >;```



### Define an option
```c++
class OptNFrames : public Option<int> {
public:
    std::string  name()          const { return "n-frames"; }
    std::string  description()   const { return "Number of frames to process"; }
    Optional     default_value() const { return 1000; }
};
```


```minimal_example.cpp``` shows how to define and 




### Minimal example

```c++
#include "Options.h"
#include <ostream>

class OptNFrames : public Option<int> {
public:
    std::string  name()          const { return "n-frames"; }
    std::string  description()   const { return "Number of frames to process"; }
    Optional     default_value() const { return 1000; }
};

int main( int argc, const char** argv )
{
    Options opt;
    opt.declare<OptNFrames>();
    opt.parse( argc, argv );
    int nFrames = opt.get<OptNFrames>();

    std::cout << "Processing " << nFrames << " frames" << std::endl;

    return 0;
}
```

```sh
$ ./analysis
Processing 1000 frames
```
```sh
$ ./analysis --n-frames=35
Processing 35 frames
```

#### Several options
```c++
#include "Options.h"
#include <ostream>

class OptNFrames : public Option<int> {
public:
    std::string  name()          const override { return "n-frames"; }
    std::string  description()   const override { return "Number of frames to process"; }
    Optional     default_value() const override { return 1000; }
};

class OptMinElectronPt : public Option<double> {
public:
    std::string  name()          const override { return "min-e-pt"; }
    std::string  description()   const override { return "Electron pt cut [MeV]"; }
    Optional     default_value() const override { return 12.7; }
};

class OptOutFileName : public Option<std::string> {
public:
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
```

```bash
$ ./minimal_example
Processing       1000 frames
Electron Pt cut: 12.7
Output file:     hists.root
```
```bash
$ ./minimal_example --out-file=hists_pt8.2.root --min-e-pt=8.2 --n-frames=500
Processing       500 frames
Electron Pt cut: 8.2
Output file:     hists_pt8.2.root
```



#### Chained notation
```c++
Options().declare<OptNFrames>().parse( argc, argv )
```
This is useful when options need to be passed to a function:

```c++
void analysis( const Options& opt )
{
    std::cout << "Processing       " << opt.get<OptNFrames>()       << " frames" << std::endl;
    std::cout << "Electron Pt cut: " << opt.get<OptMinElectronPt>() << std::endl;
    std::cout << "Output file:     " << opt.get<OptOutFileName>()   << std::endl;
}

int main( int argc, const char** argv )
{
    analysis( Options().declare<OptNFrames>()
                       .declare<OptMinElectronPt>()
                       .declare<OptOutFileName>()
                       .parse( argc, argv ) );
    return 0;
}
```

especially in the chain of constructor calls: