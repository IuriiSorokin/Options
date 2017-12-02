# `Options`
The project is a wrapper around [```boost::program_options```](http://www.boost.org/doc/libs/1_63_0/doc/html/program_options.html), designed to simplify defining, managing, and reusing any considerable number of command line options.

## Main features

* Every option is represented by a class, so all the related logic is localized. 

* Option value has an explicit type (not type-erased).

* Options can be combined in a list: `using MyOptions = OptionList<OptionA, OptionB, OptionC>;`. An `OptionList` may contain another `OptionList`:  `using MyOptionsExtended = OptionList< MyOptions, OptionX >;`. 

* Validity check and post-processing can be integrated in the option definition class (again, this logic will not penetrate into the option-user code). 

* Each option can access the values of other options within the same `Options` object. Thus, e.g. in the validity check one can check for inconsistencies.   

* It is possible to override any option property, such as e.g. the default value, or the validity checking logic (except the value type).    

## Requirements
* c++14
* boost


## Integrating `Options` in a project
#### 1. In the source tree:
Add `Options` as a git submodule, or as CMake ExternalProject, or simply copy ```Options.h``` and ```polymorphic.h``` in.

#### 2. In the CMakeLists.txt: 
* add `find_package( Boost REQUIRED program_options )` 
* add `include_directories( ${Boost_INCLUDE_DIRS} )`
* link with `${Boost_LIBRARIES}` like: 
```cmake
target_link_libraries( <MyTarget> <MyLib1> <MyLib2> ... ${Boost_LIBRARIES} )
                                                        ^^^^^^^^^^^^^^^^^^  
```

#### 3. In the source code:
```c++
#include "Options.h"
```

# Tutorial
### Define an option
An option is defined by implementing the class template `Option<T>`:

```c++
/*  Option type is specified    Option long name without the   Short option name.    */
/*  as the template parameter   leading '--'. In the command   In the command line   */
/*                        \     line it will be '--n-frames'   it will be '-N'.      */
/*                         \                           \           /                 */
struct OptNFrames : Option<int> {   /*                  \        /                   */
    std::string name()        const override { return "n-frames,N"; }
    std::string description() const override { return "Number of frames to process"; }
};                                    /*                 /                */ 
                                      /*     Description, that will       */
                                      /*     appear in the help.          */
```
Short option name must consist of only a signle letter.   
Short option name may be omitted.   
The long name is mandatory.

### Parse the command line: 
Options are managed by the `Options` class. Options need to be declared, 
and then the `Options` object can parse the command line:
```c++
auto options = Options().declare<OptNFrames>()  
                        .parse( argc, argv );   
```
Both `Options::declare` and `Options::parse` return `Options&`, so the calls 
can be daisy-chained. 

### Get the option value:

```c++
options.get_value<OptNFrames>(); // returns int
```
An attempt to `get_value` of an option that was not specified results in exception.

### Check if option value was specified:
``` c++
options.is_set<OptNFrames>(); // returns bool
```

### Check if option was declared
``` c++
options.is_declared<OptNFrames>(); // returns bool
```
### Default value
Can be specified by overriding the respective function:
```c++                      
struct OptNFrames : Option<int> {
    std::string name()          const override { return "n-frames"; }
    std::string description()   const override { return "Number of frames to process"; }
    Optional    default_value() const override { return 1000; }
};
```
Here `Optional` is an [`alias`](http://en.cppreference.com/w/cpp/language/type_alias) 
to `boost::optional<int>`. If `default_value` were not overridden, it would return 
an empty `optional`, meaning "no default value".   

### Minimal complete example
```c++
/**
 *   ex01_minimal_usage.cpp
 */

#include "Options.h"
#include <iostream>

struct OptNFrames : Option<int> {
    std::string name()          const override { return "n-frames,N"; }
    std::string description()   const override { return "Number of frames to process"; }
    Optional    default_value() const override { return 1000; }
};

int main( int argc, const char** argv )
{
    auto options = Options().declare<OptNFrames>()
                            .parse( argc, argv );

    auto n_frames = options.get_value<OptNFrames>();

    std::cout << "Processing " << n_frames << " frames"<< std::endl;

    return 0;
}
```

```sh
$ ./ex01_minimal_usage --n-frames=33
Processing 33 frames
```

```sh
$ ./ex01_minimal_usage -N 77
Processing 77 frames
```

```sh
$ ./ex01_minimal_usage
Processing 1000 frames
```

If no default was provided (see [`ex01a_minimal_usage_no_default.cpp`](ex01a_minimal_usage_no_default.cpp)), 
an attempt to access an unspecified option results in exception:
```c++
$ ./ex01a_minimal_usage_no_default      
terminate called after throwing an instance of 'std::logic_error'
  what():  Not initialized
```

### List of options
Several options can be combined in an `OptionList`, and declared at once. This 
is handy to define the list of options required by some class:

```c++
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
```

```sh
$ ./ex02b_option_list 
Constructing BasicAnalysis:
  N Frames to process = 1000
  Electron Pt cut     = 12.7
```


An `OptionList` may contain another `OptionList`. This is convenient to 
extend an existing `OptionList`:

```c++
struct ExtendedAnalysis : public BasicAnalysis
{
    struct OptOutFileName : Option<std::string> {
        std::string  name()          const override { return "out-file"; }
        std::string  description()   const override { return "Output file name"; }
        Optional     default_value() const override { return std::string("results.root"); }
    };

    using required_options = OptionList< BasicAnalysis::required_options, OptOutFileName >;

    ExtendedAnalysis( const Options& options )
    : BasicAnalysis( options )
    {
        std::cout << "Constructing ExtendedAnalysis:" << std::endl;
        std::cout << "  N Frames to process = " << options.get_value<OptNFrames>()       << std::endl;
        std::cout << "  Electron Pt cut     = " << options.get_value<OptMinElectronPt>() << std::endl;
        std::cout << "  Output file         = " << options.get_value<OptOutFileName>()   << std::endl;
    }
};
```

```c++
int main( int argc, const char** argv )
{
    ExtendedAnalysis( Options().declare<ExtendedAnalysis::required_options>().parse( argc, argv ) );
    return 0;
}
```

```sh
$ ./ex02b_option_list   
Constructing BasicAnalysis:
  N Frames to process = 1000
  Electron Pt cut     = 12.7
Constructing ExtendedAnalysis:
  N Frames to process = 1000
  Electron Pt cut     = 12.7
  Output file         = results.root
```


### Override option attribute
Suppose in `ExtendedAnalysis` you want to override the default value for `OptMinElectronPt`.
This can be done in the following way:

```c++
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
```

```sh
 % ./ex03_override_attribute 
Constructing BasicAnalysis:
  N Frames to process = 1000
  Electron Pt cut     = 25.4
Constructing ExtendedAnalysis:
  N Frames to process = 1000
  Electron Pt cut     = 25.4
  Output file         = results.root
```

The more derived option (`ExtendedAnalysis::OptMinElectronPt`) will replace the 
base one (`BasicAnalysis::OptMinElectronPt`), 
and will be used instead of it everywhere. E.g. a call:  
```c++
get_value<BasicAnalysis::OptMinElectronPt>();
```
will return the value of `ExtendedAnalysis::OptMinElectronPt`.

### Priority in option declaration
1) More derived option will replace its base. An attempt to declare an already 
   declared option, or a base of an already declared one, will result in no action.
   In each case it is ensured that the base and the derived options have the same name, 
   otherwise an exception is thrown.  
2) It is allowed to declare two options with a common base (can be any other option), 
   but they must have different names.

### Validity check
Validity check can be introduced by overriding the `Option<T>::value()` function.
At this point it is important to mention that `Option<T>` has 
four `...value` functions:    

`virtual Optional Option<T>::default_value() const`    
(non-virtual) `Optional Option<T>::specified_value() const`         
(non-virtual) `Optional Option<T>::raw_value() const`         
`virtual Optional Option<T>::value() const`    
    
As was already mentioned above, `Optional` is 
an [`alias`](http://en.cppreference.com/w/cpp/language/type_alias) 
to `boost::optional<T>`, and it has nothing to do with the command line options. 
So each `...value` function can return either a value or nothing.       

`default_value()` was already discussed above. As defined in `Option<T>`, it returns
an empty `Optional`, but the user may override it.
 
`specified_value()` returns what was obtained from parsing the command line arguments.   
   
`raw_value()` chooses between the `specified_value()` and `default_value()`:   
	if `specified_value()` is non-empty, it returns `specified_value()`;    
	otherwise, if `default_value()` is non-empty, it returns `default_value()`;    
	otherwise it returns an empty `Optional`.   
	
`value()`, as defined in `Option<T>` returns `raw_value()`. Here the user may hook in 
any custom logic, including the validity check.


```c++
/**
 *   ex04_validity_check.cpp
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
        Optional     value()         const override;
    };

    using required_options = OptionList< OptNFrames, OptMinElectronPt >;

    BasicAnalysis( const Options& options )
    {
        std::cout << "Constructing BasicAnalysis:" << std::endl;
        std::cout << "  N Frames to process = " << options.get_value<OptNFrames>()       << std::endl;
        std::cout << "  Electron Pt cut     = " << options.get_value<OptMinElectronPt>() << std::endl;
    }
};

auto
BasicAnalysis::OptMinElectronPt::value() const -> Optional
{
    if( raw_value().get() < 0 ) {
        throw std::invalid_argument( OptMinElectronPt().name_long_prefixed() +
                                     " is negative" );
        return 0.;
    }
    return raw_value();
}


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
```


```sh
$ ./ex04_validity_check
Constructing BasicAnalysis:
  N Frames to process = 1000
  Electron Pt cut     = 25.4
Constructing ExtendedAnalysis:
  N Frames to process = 1000
  Electron Pt cut     = 25.4
  Output file         = results.root
```

```sh
$ ./ex04_validity_check --min-e-pt -11.6
Constructing BasicAnalysis:
  N Frames to process = 1000
terminate called after throwing an instance of 'std::invalid_argument'
  what():  --min-e-pt is negative
```

### Accessing other options
Suppose an analysis requires several input and output files, which are usually 
in the same directory. It is then convenient to be able to specify 
only the input directory, and use the default file names. This is implemented in
the further example.


```c++
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
    std::string  name() const override
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
```

Use the specified directory for all files:
```sh
$ ./ex05_accessing_other_options -d /data/simulations/new_geometry
Constructing BasicAnalysis:
  Data dir            = "/data/simulations/new_geometry"
  Input data file     = "/data/simulations/new_geometry/raw_data.root"
  Geometry file       = "/data/simulations/new_geometry/geometry.json"
  Output file         = "/data/simulations/new_geometry/results.json"
  N Frames to process = 1000
  Electron Pt cut     = 12.5
```

Use a different geometry file from the same directory:
```sh
 % ./ex05_accessing_other_options -d /data/simulations/new_geometry --geo-file  misaligned_geometry.json
Constructing BasicAnalysis:
  Data dir            = "/data/simulations/new_geometry"
  Input data file     = "/data/simulations/new_geometry/raw_data.root"
  Geometry file       = "/data/simulations/new_geometry/misaligned_geometry.json"
  Output file         = "/data/simulations/new_geometry/results.json"
  N Frames to process = 1000
  Electron Pt cut     = 12.5
```

Don't overwrite the results in the data directory, but store them in the current dir:
```sh
$ ./ex05_accessing_other_options -d /data/simulations/new_geometry --out-file ./results.root                                                                      
Constructing BasicAnalysis:
  Data dir            = "/data/simulations/new_geometry"
  Input data file     = "/data/simulations/new_geometry/raw_data.root"
  Geometry file       = "/data/simulations/new_geometry/geometry.json"
  Output file         = "./results.root"
  N Frames to process = 1000
  Electron Pt cut     = 12.5
```

### Print help
Can be done with:
```c++
options.print_help( std::cout );
```

It is convenient to define a help option `OptHelp`, as 
in the previous example. Notice how `OptHelp::handle()` was called:
```c++
    BasicAnalysis( Options().declare<BasicAnalysis::required_options>()
                            .parse( argc, argv )
                            .call( []( const Options& options ){ options.get<OptHelp>().handle(); } ) );
```
`Options::declare<T...>()` and `Options::parse()` 
return the reference to itself (`Options&`). `Options::call( FuncT )` 
invokes the provided lambda  passesing itself (`const Options&`) 
as an argument. Inside the lambda `options.get<OptHelp>()` 
returns the reference `OptHelp&`, for which  `handle()` is 
finally called. `Options::call(FuncT)` also returns the reference 
to itself.

