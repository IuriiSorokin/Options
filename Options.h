
// todo:
//   comments
//   sort the options by name

#ifndef OPTIONS_H_
#define OPTIONS_H_

#include <typeinfo>
#include <boost/program_options.hpp>
#include <boost/optional.hpp>
#include <memory>
#include <utility>
#include <tuple>
#include <ostream>
#include <fstream>
#include <stdexcept>
#include "polymorphic.h"


/** Base class for any option. */
template< typename ValueType >
class Option;

/** Collection of option objects. Through this class
 *  parsing is performed, and the option values are
 *  obtained. */
class Options;

/** Helper to distinguish between Option and OptionList
 *  in Options::declare_impl<T>() */
namespace detail_Options {
class OptionListBase {};
}

/** List of options. Can be used to specify a group of options
 *  that always need to be defined and used together. The options
 *  are stored as template arguments. */
template< typename... T >
class OptionList : private detail_Options::OptionListBase {};



namespace detail_Options {

/** Helper class enabling to store objects of different Option<ValueType> implementations
 *  in a single collection, and providing the necessary common interface and functionality. */
class OptionBase
{
    friend class ::Options;

private:
    const Options* _options = nullptr;

protected:
    OptionBase() = default;

    /** To avoid forcing the user to define, or explicitly inherit, a non-trivial constructor. */
    template<typename OptionType,
             typename std::enable_if< std::is_base_of< OptionBase, OptionType >::value, bool >::type = true >
    static OptionType
    construct( const Options* options );

public:
    virtual
    ~OptionBase() = default;

    /** Option name, as to be specified in the command line, but without the leading "--" or "-".
     *  E.g.: "help" for "--help".
     *  Short notation may be specified:
     *  E.g. "h,help" for "-h" or "--help".
     *  To be implemented by the user. */
    virtual std::string
    name() const = 0;

    /** Without the leading "--". */
    char
    name_short() const;

    /** Without the leading "-". */
    std::string
    name_long() const;

    /** With the leading "--". */
    std::string
    name_long_prefixed() const;

    /** Description of the option, that will be printed in the help.
     *  To be implemented by the user. */
    virtual std::string
    description() const
    { return ""; }

    /** Print the value (not the raw_value). */
    virtual std::ostream&
    print( std::ostream& os ) const = 0;

    /** Value to string. (not the raw_value). */
    std::string
    to_string() const;

    friend std::ostream&
    operator<<( std::ostream& os, const OptionBase& option );

protected:
    /** Owning Options object.
     *  To be used to get the values of other Option's */
    const Options*
    get_options() const;

private:
    void
    set_options( const Options* options );

    virtual void
    declare( boost::program_options::options_description& description ) const = 0;

    virtual void
    set_from_vm( const boost::program_options::variables_map& vm ) = 0;

    std::tuple<char, std::string>
    split_name( std::string name ) const;
};



inline std::tuple<char, std::string>
OptionBase::split_name( std::string name ) const
{
    char short_name = 0;
    std::string long_name;
    if( name.size() >= 2 and name.at(name.size()-2) == ',' ) {
        short_name = name.back();
        long_name  = name.substr( 0, name.size()-2 );
    } else {
        short_name = 0;
        long_name  = name;
    }

    if( short_name != 0 and not std::isalpha( short_name ) ) {
        throw std::invalid_argument( std::string("Short option name '") + short_name + "' is not a letter.");
    }

    if( long_name.size() == 0  ) {
        throw std::invalid_argument("Long option name was not specified.");
    }

    for( const char l : long_name ) {
        if( l == ',' ) {
            throw std::logic_error( "Long option name contains disallowed ',' character." );
        }
    }

    return std::make_tuple(short_name, long_name);
}



inline char
OptionBase::name_short() const
{
    return std::get<0>( split_name( name() ) );
}



inline std::string
OptionBase::name_long() const
{
    return std::get<1>( split_name( name() ) );
}



inline std::string
OptionBase::to_string() const
{
    std::ostringstream s;
    print( s );
    return s.str();
}



template<typename OptionT,
         typename std::enable_if< std::is_base_of< OptionBase, OptionT >::value, bool >::type >
OptionT
OptionBase::construct( const Options* options )
{
    auto option = OptionT();
    dynamic_cast<OptionBase*>(&option)->set_options( options );
    return option;
}



inline std::string
OptionBase::name_long_prefixed() const
{
    return std::string("--") + name();
}



inline void
OptionBase::set_options( const Options* options )
{
    _options = options;
}



inline const Options*
OptionBase::get_options() const
{
    return _options;
}


} // namespace detail_Options




inline std::ostream&
operator<<( std::ostream& os, const detail_Options::OptionBase& option )
{
    return option.print( os );
}



/**
 *  Base class for option definition.
 */
template< typename ValueType >
class Option : public detail_Options::OptionBase {
public:
    using value_type = ValueType;
    using Optional   = boost::optional<value_type>;

protected:
    Optional _specified_value;

public:
    virtual
    ~Option() = default;

    /** To be overridden by the user.
     *  Leave uninitialized for no default value. */
    virtual Optional
    default_value() const
    { return Optional(); }

    /** What was specified in the command line or in the configuration file. */
    Optional
    specified_value() const
    { return _specified_value; }

    /** specified_value(), or if wasn't specified, then default_value(). */
    Optional
    raw_value() const;

    /** To be overridden by the user if any post-processing or validity check is necessary.
     *  Hint: use get_options() to access the values of other options. */
    virtual Optional
    value() const
    { return raw_value(); }

    /** Specify the value in the code. */
    void
    set( const value_type& value )
    { _specified_value = value; }

    /** Print the value (not the raw_value). */
    virtual std::ostream &
    print( std::ostream& os ) const override;

protected:
    virtual void
    declare( boost::program_options::options_description& opt_descr ) const override;

    virtual void
    set_from_vm( const boost::program_options::variables_map& vm ) override final;
};



template< typename ValueType >
inline auto
Option<ValueType>::raw_value() const -> Optional
{
    return specified_value().is_initialized() ? specified_value() : default_value();
}



/**
 *  Allows to specify boolean options as e.g.:
 *      ./my_program --help       or
 *      ./my_program -h
 *  instead of
 *      ./my_program --help=1     or
 *      ./my_program -h 1
 */
class OptionSwitch : public Option<bool>
{
protected:
    Optional
    default_value() const override
    { return false; }

    virtual void
    declare( boost::program_options::options_description& opt_descr ) const override;
};



inline void
OptionSwitch::declare( boost::program_options::options_description& opt_descr ) const
{
    auto value = boost::program_options::value<value_type>();

    if( default_value().is_initialized() ) {
        value->default_value( default_value().get() );
    }

    value->implicit_value( true );

    opt_descr.add_options()( name().c_str(), value, description().c_str() );
}



/**
 * Collection of Option<ValueType> objects.
 * Can parse the command line arguments or a configuration file,
 * and set the values of the contained Option<ValueType> objects.
 */
class Options {
    using options_description = boost::program_options::options_description;
    using variables_map = boost::program_options::variables_map;

private:
    std::string _caption;    // of the help message
    unsigned    _line_length; // of the help message
    unsigned    _min_description_length; // in the help message
    std::vector< polymorphic< detail_Options::OptionBase > > _options;

public:
    Options()
    : Options( "Available options", 120, 80 ) {}

    Options( std::string caption,                 // caption of the help message
             unsigned    line_length,              // width of the help message
             unsigned    min_description_length );  // min width of option description in the help message

    Options( const Options& options );

    Options( Options&& options );

    Options&
    operator=( const Options& other );

    Options&
    operator=( Options&& other );

    /** non-virtual ! */
    ~Options() = default;

    /** Declare a single option or option list */
    template<typename... OptionsOrOptionListsT>
    Options &
    declare();

//    /** Un-declare an option of type OptionType or any derived type.
//     *  If the option was not declared, or if there are more than one option found
//     *  (which normally not be the case), then exception is thrown. */
//    template<typename OptionType,
//    typename std::enable_if< std::is_base_of< detail_Options::OptionBase, OptionType >::value, bool >::type = true >
//    Options &
//    renounce();

    /** Declare and set in one call. */
    template<typename OptionType>
    Options &
    declare_and_set( typename OptionType::value_type value );

    /** If the option OptionType or a more derived one was already declared.
     *  No error is produces if there are more than one options derived from
     *  OptionType declared. */
    template<typename OptionType>
    bool
    is_declared() const;

    /** Parse the command line arguments, and, if provided, the options_file.
     *  Values in the command line have priority.
     *  Previously set values are overwritten without warnings.
     *  Throws if non-declared option is encountered, or if a parsing error occurs. */
    Options &
    parse( int argc, const char * const argv[], std::string options_file = "" );

    /** Get the option object.
     *  Throws if the option was not declared. */
    template<typename OptionType>
    const OptionType&
    get() const;

    template<typename OptionType>
    OptionType&
    get();

    /** Get the value of the option.
     *  Throws if the option was not declared.
     *  Throws if the option has no value */
    template<typename OptionType>
    typename OptionType::value_type
    get_value() const;

    /** Returns the option value, if available, or fallback otherwise.
     *  Throws if the option was not declared. */
    template<typename OptionType>
    typename OptionType::value_type
    get_value_or( typename OptionType::value_type fallback ) const;

    /** Set the option value.
     *  Throws if the option was not declared. */
    template<typename OptionType>
    Options&
    set_value( typename OptionType::value_type value);

    /** If the option has a value. */
    template<typename OptionType>
    bool
    is_set() const;

    /** Help is generated by boost::program options. Caption, line length, and the width of the description
     *  column are set in the constructor:
     *  Options::Options( const std::string& caption, unsigned lineLength, unsigned minDescriptionLength ); */
    void
    print_help( std::ostream& os = std::cout ) const;

    /** Print a table with option values */
    const Options&
    print( std::ostream& os = std::cout ) const;

    /** Calls func(*this).
     *  Func return value is ignored.
     *  Useful in e.g.:
     *      MyClass::MyClass( Options options )
     *      : _options( options.declare<OptionA>()
     *                         .declare<OptionB>()
     *                         .call( [&]( Options& o ) {
     *                                    if( o.is_declared<OptionC>() ) {
     *                                        o.renounce<OptionC>();
     *                                        o.declare<OptionCNew>();
     *                                    }
     *                                } )
     *                         .declare<OptionZ>()
     *                         .parse( argc, argv ) );    */
    template<typename Func>
    Options&
    call( Func func );

protected:
//    /** Throws if OptionType has the same long or short name as any other already declared one. */
//    template<typename OptionType>
//    void
//    check_no_name_collisions() const;

    /** Returns iterator to option of type OptionType or a more derived one.
     *  If there there are more than one options of type OptionType or derived,
     *  then std::logic_error is thrown.
     *  Returns  _options::cend() if the option was not found (not declared) */
    template<typename OptionType>
    decltype(_options)::const_iterator
    find_option_const() const;

    /** Same as find_option_const(), but non-const. */
    template<typename OptionType>
    decltype(_options)::iterator
    find_option();

    options_description
    make_options_description() const;

    /** throws if \p optionsFile contain an non-declared option */
    void
    parse_from_file( const options_description & optionsDescription,
                     std::string optionsFile,
                     variables_map & parsedOptions );

    /** throws if \p argv contain an non-declared option */
    void
    parse_from_command_line( const options_description & opt_descr,
                             int argc,
                             const char * const argv[],
                             variables_map & parsed_options );

    /** Set the values of all options in \p _options from the \p vm*/
    void
    set_from_vm( const variables_map & vm );

    template< typename OptionOrOptionListT,
              typename... OptionsOrOptionListsT,
              std::enable_if_t< sizeof...(OptionsOrOptionListsT), int > = 0 >
    Options&
    declare_impl();

    /** Declare a single option. */
    template< typename OptionT,
              std::enable_if_t< std::is_base_of< detail_Options::OptionBase, OptionT >::value, int > = 0 >
    Options &
    declare_impl();

    /** Declare an OptionList. */
    template< typename OptionListT,
              std::enable_if_t< std::is_base_of< detail_Options::OptionListBase, OptionListT >::value, int > = 0 >
    Options &
    declare_impl();

    /** Declare an OptionList. */
    template< typename... OptionsOrOptionListsT >
    Options &
    declare_impl_unpack_list( OptionList<OptionsOrOptionListsT...> option_list );
};




template< typename ValueType >
std::ostream &
Option<ValueType>::print( std::ostream& os ) const
{
    if( value().is_initialized() ) {
        os << value().get();
    }
    return os;
}



template< typename ValueType >
void
Option<ValueType>::declare( boost::program_options::options_description& opt_descr ) const
{
    auto value = boost::program_options::value<value_type>();

    if( default_value().is_initialized() ) {
        value->default_value( default_value().get() );
    }

    opt_descr.add_options()( name().c_str(), value, description().c_str() );
}



template< typename ValueType >
void
Option<ValueType>::set_from_vm( const boost::program_options::variables_map& vm )
{
    using boost::program_options::variable_value;

    if( vm.count( name_long() ) ) {
        const variable_value& variableValue = vm[ name_long() ];
        set( variableValue.as<ValueType>() );
    }
}



inline
Options::Options( std::string caption,
                  unsigned    line_length,
                  unsigned    min_description_length )
: _caption( caption )
, _line_length( line_length )
, _min_description_length( min_description_length )
{}



inline
Options::Options( const Options& options )
: _caption( options._caption )
, _line_length( options._line_length )
, _min_description_length( options._min_description_length )
, _options( options._options )
{
    for( auto& option: _options ) {
        option.get().set_options( this );
    }
}



inline
Options::Options( Options&& options )
: _caption( std::move(options._caption) )
, _line_length( options._line_length )
, _min_description_length( options._min_description_length )
, _options( std::move( options._options ) )
{
    for( auto& option: _options ) {
        option.get().set_options( this );
    }
}



inline Options&
Options::operator=( const Options& other )
{
    _caption                = other._caption;
    _line_length            = other._line_length;
    _min_description_length = other._min_description_length;
    _options                = other._options;

    for( auto& option: _options ) {
        option.get().set_options( this );
    }

    return *this;
}



inline Options&
Options::operator=( Options&& other )
{
    _caption                = std::move( other._caption );
    _line_length            = std::move( other._line_length );
    _min_description_length = std::move( other._min_description_length );
    _options                = std::move( other._options );

    for( auto& option: _options ) {
        option.get().set_options( this );
    }

    return *this;
}



template<typename... OptionsOrOptionListsT>
Options&
Options::declare()
{
    declare_impl<OptionsOrOptionListsT...>();
    return *this;
}



template< typename OptionOrOptionListT,
          typename... OptionsOrOptionListsT,
          std::enable_if_t< sizeof...(OptionsOrOptionListsT), int > >
Options&
Options::declare_impl()
{
    declare_impl<OptionOrOptionListT>();
    declare_impl<OptionsOrOptionListsT...>();
    return *this;
}



template< typename OptionT,
          std::enable_if_t< std::is_base_of< detail_Options::OptionBase, OptionT >::value, int > >
Options &
Options::declare_impl()
{
    if( is_declared<OptionT>() ) {
        return *this;
    }

    auto to_replace = _options.end();

    for( auto option_iter = _options.begin(); option_iter < _options.end(); ++option_iter ) {
        const bool already_declared_is_same = typeid(option_iter->get()).name() == typeid(OptionT).name();
        const bool already_declared_is_parent = option_iter->is_dynamic_castable_to_actual( OptionT() ) and not already_declared_is_same;
        const bool already_declared_is_child = dynamic_cast<const OptionT*>(&(option_iter->get())) and not already_declared_is_same;
        // fourth possible case is that the already declared option is not related to OptionT

        const bool same_long_name  = option_iter->get().name_long() == OptionT().name_long();
        const bool same_short_name = option_iter->get().name_short() == OptionT().name_short();
        const bool same_nonempty_short_name = same_short_name and ( option_iter->get().name_short() != 0 );

        if( already_declared_is_parent ) {
            if( (not same_long_name) or (not same_short_name) ) {
                throw std::logic_error( std::string() + "Attempting to declare option of type " + typeid(OptionT).name() + ". "
                                        "Found parent option " + typeid(option_iter->get()).name() + " but the name is different. "
                                        "Replacing an option by one with different name is not allowed." );
            }
            if( to_replace != _options.end() ) {
                throw std::logic_error( std::string() + "Attempting to declare option of type " + typeid(OptionT).name() + ". "
                                        "Found more than one parent options with the same name. Don't know which to replace." );
            }
            to_replace = option_iter;
        }
        else if( already_declared_is_same or already_declared_is_child ) {
            if( (not same_long_name) or (not same_short_name) ) {
                throw std::logic_error( std::string() + "Attempting to declare option of type " + typeid(OptionT).name() + ". "
                                        "Found option of the same or child type " + typeid(option_iter->get()).name() +
                                        " but the name is different. This should never happen. All instances of any option are "
                                        "obliged to return the same name." );
            }
        }
        else { // declared is not related to OptionT
            if( same_long_name or same_nonempty_short_name ) {
                throw std::logic_error( std::string() + "Can't declare option of type " + typeid(OptionT).name() +
                                        " because of name collision with option " + typeid(option_iter->get()).name() + "." );
            }
        }
    }
    
    if( to_replace != _options.end() ) {
        _options.erase( to_replace );
    }

    _options.push_back( polymorphic<detail_Options::OptionBase>( detail_Options::OptionBase::construct<OptionT>( this ) ) );

    return *this;
}



/** Declare an OptionList. */
template< typename OptionListT,
          std::enable_if_t< std::is_base_of< detail_Options::OptionListBase, OptionListT >::value, int > >
Options &
Options::declare_impl()
{
    declare_impl_unpack_list( OptionListT() );
    return *this;
}



template< typename... OptionsOrOptionListsT >
Options &
Options::declare_impl_unpack_list( OptionList<OptionsOrOptionListsT...> option_list )
{
    declare_impl<OptionsOrOptionListsT...>();
    return *this;
}



//template<typename OptionType,
//typename std::enable_if< std::is_base_of< detail_Options::OptionBase, OptionType >::value, bool >::type >
//Options &
//Options::renounce()
//{
//    auto opt_iter = find_option<OptionType>();
//
//    if( opt_iter == _options.end() ) {
//        throw std::logic_error("Option not found");
//    }
//
//    _options.erase( opt_iter );
//
//    return *this;
//}



template<typename OptionType>
Options &
Options::declare_and_set( typename OptionType::value_type value )
{
    declare<OptionType>();
    get<OptionType>().set( value );
    return *this;
}



template<typename OptionType>
bool
Options::is_declared() const
{
    for( const auto& option : _options ) {
        if( dynamic_cast< const OptionType* >( &(option.get()) ) ) {
            return true;
        }
    }
    return false;
}



template<typename OptionType>
auto
Options::find_option_const() const -> decltype(_options)::const_iterator
{
    auto found = _options.end();

    for( auto iOption = _options.begin(); iOption < _options.end(); ++iOption ) {
        if( dynamic_cast< const OptionType* >( &(iOption->get()) ) ) {
            if( found != _options.end() ) {
                throw std::logic_error( std::string() + "More than one option of type " + typeid(OptionType).name() + " is found." );
            }
            found = iOption;
        }
    }

    return found;
}



template<typename OptionType>
auto
Options::find_option() -> decltype(_options)::iterator
{
    return _options.begin() + std::distance( _options.cbegin(), find_option_const<OptionType>() );
}



template<typename OptionType>
const OptionType&
Options::get() const
{
    auto iter = find_option_const<OptionType>();
    if( iter == _options.cend() ) {
        throw std::logic_error( std::string("Option ") + OptionType().name_long() + " was not declared." );
    }
    return dynamic_cast< const OptionType&>( (*iter).get() );
}



template<typename OptionType>
OptionType&
Options::get()
{
    auto iter = find_option<OptionType>();
    if( iter == _options.end() ) {
        throw std::logic_error( std::string("Option ") + OptionType().name_long() + " was not declared." );
    }
    return dynamic_cast<OptionType&>( (*iter).get() );
}



inline Options::options_description
Options::make_options_description() const
{
    auto opt_descr = options_description( _caption, _line_length, _min_description_length );

    for( const auto& option: _options ) {
        option.get().declare( opt_descr );
    }

    return opt_descr;
}



inline Options&
Options::parse( int argc, const char * const argv[], std::string optionsFile )
{
    auto vm = boost::program_options::variables_map();
    auto opt_descr = make_options_description();

    if( optionsFile.size() ) {
        parse_from_file( opt_descr, optionsFile, vm );
    }

    parse_from_command_line( opt_descr, argc, argv, vm );

    set_from_vm( vm );

    return *this;
}



template<typename OptionType>
typename OptionType::value_type
Options::get_value() const
{
    const auto& optional_value = get<OptionType>().value();
    if( not optional_value.is_initialized() ) {
        throw std::logic_error("Not initialized");
    }
    return optional_value.get();
}



template<typename OptionType>
typename OptionType::value_type
Options::get_value_or( typename OptionType::value_type fallback ) const
{
    const auto& optional_value = get<OptionType>().value();
    return optional_value.is_initialized() ? optional_value.get() : fallback;
}



template<typename OptionType>
Options&
Options::set_value( typename OptionType::value_type value)
{
    return get<OptionType>()->set( value );
}



template<typename OptionType>
bool
Options::is_set() const
{
    return get<OptionType>().raw_value().is_initialized();
}



inline void
Options::parse_from_file( const options_description& optionsDescription,
                               std::string optionsFile,
                               variables_map& parsedOptions  )
{
    std::ifstream file( optionsFile.c_str() );
    boost::program_options::store(
            boost::program_options::parse_config_file( file, optionsDescription ),
            parsedOptions
            );
    boost::program_options::notify( parsedOptions );
}



inline void
Options::parse_from_command_line( const options_description & opt_descr,
                                       int argc,
                                       const char * const argv[],
                                       variables_map & parsed_options  )
{
    boost::program_options::store( boost::program_options::command_line_parser( argc, argv ).options( opt_descr ).run(), parsed_options );
    boost::program_options::notify( parsed_options );
}



inline void
Options::set_from_vm( const variables_map & vm )
{
    for( auto & option : _options ) {
        option.get().set_from_vm( vm );
    }
}



inline void
Options::print_help( std::ostream& os ) const
{
    os << make_options_description() << std::endl;
}



inline const Options&
Options::print( std::ostream& os ) const
{
    size_t maxNameLength = 0;
    for( const auto& option : _options ) {
        maxNameLength = std::max( maxNameLength, option.get().name_long().size() );
    }

    for( const auto& option : _options ) {
        std::string name_with_spaces;
        name_with_spaces = option.get().name_long();
        name_with_spaces.append( maxNameLength - name_with_spaces.size(), ' ' );
        os << name_with_spaces << "  : ";
        option.get().print( os );
        os << std::endl;
    }

    return *this;
}



template<typename Func>
Options&
Options::call( Func func )
{
    func( *this );
    return *this;
}



//template<typename OptionType>
//void
//Options::check_no_name_collisions() const
//{
//    for( const auto& option : _options ) {
//        const bool same_short_names =
//                option.get().name_short() == OptionType().name_short()
//                and option.get().name_short() != 0 ;
//        const bool same_long_names  = option.get().name_long()  == OptionType().name_long();
//
//        if( same_short_names or same_long_names ) {
//            throw std::logic_error( std::string("Option ") + typeid(OptionType).name()
//                    + " has name conflict with the already declared " + typeid(option.get()).name() );
//        }
//    }
//}



#endif /* BASE_OPTIONT_H_ */
