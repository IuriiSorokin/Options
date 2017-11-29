# Options
The project is a wrapper around [```boost::program_options```](http://www.boost.org/doc/libs/1_63_0/doc/html/program_options.html), designed to simplify defining, managing, and reusing any considerable number of command line options.

## Main features

* Every option is a class, so all the knowledge and logic, related the option, is concentrated in one place. 

* Option value has an explicit type (not type-erased).

* Options can be combined in a list: `using MyOptions = OptionList<OptionA, OptionB, OptionC>;`. An `OptionList` may contain another `OptionList`:  `using MyOptionsExtended = OptionList< MyOptions, OptionX >;`. 

* Validity check and post-processing can be integrated in the option definition class (again, this logic will not penetrate into the option-user code). 

* Each option can access the values of other options within the same `Options` object. Thus, e.g. in the validity check one can check for inconsistencies.   

* It is possible to override any option property, such as e.g. the default value, or the validity checking logic (except the value type).    

## Requirements
* c++14
* boost


## Integrating in your project
#### 1. In the source tree:
Add ```Options``` as a git submodule, or as CMake ExternalProject, or simply copy ```Options.h``` and ```polymorphic.h``` in.

#### 2. In the CMakeLists.txt: 
* add ```find_package( Boost REQUIRED program_options )``` 
* add ```include_directories( ${Boost_INCLUDE_DIRS} )```
* link with ```${Boost_LIBRARIES}``` like: 
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
struct OptNFrames : Option<int> {
    std::string name()        const override { return "n-frames"; }
    std::string description() const override { return "Number of frames to process"; }
};
```
The value type is specified as the template argument, whereas the name and description --- by overriding the respective functions. The name should be specified without the leading `--`, whereas in the command line the `--` need to be prepended (see below). The description string will be shown in help (see following sections). `OptNFrames` can now be used as follows:

```c++
// ex01_minimal_usage.cpp
int main( int argc, const char** argv )
{
    auto options = Options().declare<OptNFrames>()
                            .parse( argc, argv );
    auto n_frames = options.get_value<OptNFrames>();
    std::cout << "N Frames = " << n_frames << std::endl;
    return 0;
}

```


```
$ ./ex01_minimal_usage --n-frames=10
N Frames = 10
```
If the option is not specified, an exeption is thrown:
```
$ ./ex01_minimal_usage --n-frames=10
...
``` 

### Default value
A default value can be set by overriding the respective function:

```c++
struct OptNFrames : Option<int> {
    std::string name()          const override { return "n-frames,N"; }
    std::string description()   const override { return "Number of frames to process"; }
    Optional    default_value() const override { return 1000; }
};
```

```
$ ./ex02_default_value
N Frames = 1000
```

```
$ ./ex02_default_value --n-frames=10
N Frames = 10
``` 




