# Options #
This is a wrapper around [```boost::program_options```](http://www.boost.org/doc/libs/1_63_0/doc/html/program_options.html), designed with the aim to make it easy to manage and reuse any sizeable number of options.

## Main features
* Every option is a class so it is defined in one place, and can be re-used in several executables.
* Options are referred to by the class name, and not by a string literal.
* Options can be listed as ```using MyOptions = std::tuple<Option1, Option2>;``` and even as ```using MyOptionsExtended = std::tuple< MyOptions, Option3 >;``` (where ```MyOptions``` is a tuple), and then declared with a single ```options.declare<MyOptinsExtende>()``` call.
* Validity check can be implemented. One can also easily get access to the other option values, so one can check for inconsistent combinations of options. 
* Option value can be post-processed. Suppose you have a ```DataDirectory``` option, then you can implement automatic appending of the trailing "\", if it is missing. Then a ```fileName``` can be safely appended without caring about possible missing "\". One can do more complex post-processing using values of other options. E.g. if you have options ```DataDirectory```, ```InputFile```, and ```OutputFile```, and you can automatically prepend ```DataDirectory``` to the ```InputFile``` and ```OutputFile``` (example 13).
* The validity check and the post-processing stays together with the definition of the option, and is not scattered around the user code.


## Requirements
* c++11
* boost


## Integrating in your project
#### 1. 
Add ```Options``` as a git submodule, or as CMake ExternalProject, or simply copy ```Options.h``` and ```polymorphic.h``` in.

#### 2. In your CMakeLists.txt: 
* add ```find_package( Boost REQUIRED program_options )``` 
* add ```include_directories( ${Boost_INCLUDE_DIRS} )```
* link with ```${Boost_LIBRARIES}``` like: 
```cmake
target_link_libraries( <MyAnalysis> <MyLib1> <MyLib2> ... ${Boost_LIBRARIES} )
                                                          ^^^^^^^^^^^^^^^^^^  
```
#### 3. In your c++ project:
```c++
#include "Options.h"
```
