
#ifndef BASE_OPTIONT_H_
#define BASE_OPTIONT_H_

#include <typeinfo>
#include <boost/program_options.hpp>
#include <boost/optional.hpp>
#include <memory>
#include <utility>
#include <tuple>
#include <assert.h>
#include <ostream>
#include <fstream>
#include "polymorphic.h"

class OptionBase;

class Options;

template< typename ValueType >
class Option;


/**
 *  /brief Helper class enabling to store objects of different Option<ValueType> implementations
 *         in a single collection, and providing the necessary common interface and functionality.
 */
class OptionBase
{
    friend class Options;

private:
    const Options* _options = nullptr;

protected:
    OptionBase() = default;

    /** To avoid forcing the users to define (or explicitly inherit) a non-trivial constructor
     *  in their implementations of Option<ValueType> */
    template<typename OptionType,
             typename std::enable_if< std::is_base_of< OptionBase, OptionType >::value, bool >::type = true >
    static OptionType
    construct( const Options* options );

public:
    virtual
    ~OptionBase() = default;

    /** see Option<ValueType>::name() */
    virtual std::string
    name() const = 0;

    /** Short option name, as e.g. in " -h ", without the leading minus. */
    char
    name_short() const
    { assert(false); return 0; } // not yet implemented

    /** Long option name, as e.g. in " --help ", without the two leading minuses */
    std::string
    name_long() const
    { assert(false); return ""; } // not yet implemented

    /** return the long option name with the leading "--" */
    std::string
    name_long_prefixed() const;

    /** see Option<ValueType>::group() */
    virtual std::string
    group() const = 0;

    /** see Option<ValueType>::description() */
    virtual std::string
    description() const = 0;

    /** see Option<ValueType>::is_valid(...) */
    virtual bool
    is_valid( std::string& error_message ) const = 0;

    /** Calls is_valid(...) and throws std::invalid_argument if the value is invalid. */
    void
    check_valid() const;

    /** see Option<ValueType>::is_set(...) */
    virtual bool
    is_set() const = 0;

    /** see Option<ValueType>::print_value(...) */
    virtual std::ostream&
    print_value( std::ostream& os ) const = 0;

    virtual bool
    omit_when_printing() const = 0;

protected:
    /** see Option<ValueType>::declare(...) */
    virtual void
    declare( boost::program_options::options_description& description ) const = 0;

    /** see Option<ValueType>::set_from_vm(...) */
    virtual void
    set_from_vm( const boost::program_options::variables_map& vm ) = 0;

    /** Store the pointer to the corresponding Options object. This is necessary
     *  in order to be able to access the values of other options from
     *  Option<ValueType>::is_valid() and Option<ValueType>::value() functions. */
    void
    assign_to( const Options* options );

    /** Returns the corresponding Options object. \n
     *  Throws if the the option wan not assigned to any Options object \n
     *  See also assign_to(...) */
    const Options&
    get_options() const;

    /** Returns the specified option from the associated Options object. \n
     *  Shortcut for get_options().get<OptionType>() */
    template<typename OptionType,
             typename std::enable_if< std::is_base_of< OptionBase, OptionType >::value, bool >::type = true >
    const OptionType&
    get() const;

    /** Returns the value of the specified option from the associated Options object. \n
     *  Shortcut for get_options().get_value<OptionType>() */
    template<typename OptionType>
    typename OptionType::value_type
    get_value() const;
};



/**
 *  Base class for option description.
 */
template< typename ValueType >
class Option : public OptionBase {
public:
    using value_type = ValueType;
    using Optional   = boost::optional<value_type>;
    using Base       = Option<ValueType>;

protected:
    Optional       _value;

public:
    virtual
    ~Option() = default;

    /** Implement to specify the option name. */
    virtual std::string
    name() const = 0;

    /** Override to specify the option group. \n
     *  Currently option groups are not implemented. */
    virtual std::string
    group() const override
    { return ""; }

    /** Override to specify the option description. \n
     *  The description will be shown in the help.  */
    virtual std::string
    description() const override
    { return ""; }

    /** Override to specify the default value. \n
     *  Empty Optional means no default value. */
    virtual Optional
    default_value() const
    { return Optional(); }

    /** Retruns the unprocessed option value.
     *  Throws if the value is not set. Use is_set() to check */
    virtual value_type
    value() const;

    /** Override to add a validity check. \n
     *  \param[out] error_message Optional explanation why the option value is invalid. */
    virtual bool
    is_valid( std::string& error_message ) const
    { return true; }

    /** Set the option value.
     *  Old value, if any, is silently overwritten. \n
     *  No validity check is performed. */
    void
    set( const value_type& value );

    /** True if the option value was set. \n
     *  Option value can be set with:
     *     - Option<ValueType>::set(...)
     *     - Options::parse(...)
     *     - Options::declare_and_set<OptionType>(...) */
    virtual bool
    is_set() const override;

    /** Print value (not the raw_value) using the corresponding operator<< */
    virtual std::ostream &
    print_value( std::ostream& os ) const override;

    /** If returns true, then Options::print_values() will omit this option */
    virtual bool
    omit_when_printing() const override;

protected:
    /** Declare this option in the \p opt_descr. \n
     *  Needed by Options::parse to construct the boost::program_options::options_description
     *  to perform the parsing. */
    virtual void
    declare( boost::program_options::options_description& opt_descr ) const override;

    /** Set the value from the variables_map. \n
     *  Called from Options::parse to set the option values.
     *  The Options class can not set the values directly, as it doesn't know the
     *  exact type of each option, so it gives the boost::program_options::variables_map.
     *  to each option to find its value. */
    virtual void
    set_from_vm( const boost::program_options::variables_map& vm ) override final;
};



/** Special implementation of a boolean option, enabling to specify it like
 *
 *      ./analysis --my-bool-opt
 *
 *  insetad of
 *
 *      ./analysis --my-bool-opt=true
 *
 *  (later form is also supported)
 *  */
class OptionSwitch : public Option<bool>
{
protected:
    /** See parent */
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
 * Collection of Option<ValueType> objects. \n
 * The stored Option<ValueType> objects may or may not have the values.
 * Can parse command line arguments or a configuration file, and set the
 * values of the Option<ValueType> objects.
 */
class Options {
    using options_description = boost::program_options::options_description;
    using variables_map = boost::program_options::variables_map;

private:
    std::string _caption;
    unsigned    _lineLength;
    unsigned    _minDescriptionLength;
    std::vector< polymorphic< OptionBase > > _options;

public:
    /** With default \p caption, \p lineLength, and \p minDescriptionLength
     *  See also the ctor with arguments. */
    Options();

    /** \param[in] caption                Caption in the help
     *  \param[in] lineLength             Width of the help
     *  \param[in] minDescriptionLength   Min width of the description column in the help */
    Options( const std::string& caption,
             unsigned lineLength,
             unsigned minDescriptionLength );

    Options( const Options& options );

    Options( Options&& options );

    Options&
    operator=( const Options& other );

    Options&
    operator=( Options&& other );

    ~Options() = default;

    /** Declare a single option. \n
     *  If this or a more derived option has already been declared, no action is done. \n
     *  If other option with the same name has already been declared, an exception is thrown. */
    template<typename OptionType,
    typename std::enable_if< std::is_base_of< OptionBase, OptionType >::value, bool >::type = true >
    Options &
    declare();

    /** Declare a non-empty tuple of options. \n
     *  Rules of the previous overload apply \n
     *  Note: don't specify the \p Index manually. */
    template<typename TupleType, size_t Index = 0,
    typename std::enable_if< ( Index < (std::tuple_size<TupleType>::value) ), bool >::type = true >
    Options &
    declare();

    /** Declare an empty tuple of options, i.e. do nothing. Necessary for technical reasons. */
    template<typename TupleType, size_t Index = 0,
    typename std::enable_if< ( Index == std::tuple_size<TupleType>::value ),  bool >::type = true >
    Options &
    declare();

    /** Declare an arbitrary combination of options or tuples (including tuples of tuples). \n
     *  Rules of the first overload apply. */
    template<typename FirstOptionOrTuple, typename... OtherOptionsOrTuples>
    typename  std::enable_if< (sizeof...(OtherOptionsOrTuples) > 0), Options &>::type
    declare();

    /** Un-declare an option of type OptionType or any derived type.
     *  If the option was not declared, or if there are more than one option found
     *  (which normally not be the case), then exception is thrown. */
    template<typename OptionType,
    typename std::enable_if< std::is_base_of< OptionBase, OptionType >::value, bool >::type = true >
    Options &
    forget();

    /** Declare and set in one call.
     *  No validity check is done */
    template<typename OptionType>
    Options &
    declare_and_set( typename OptionType::value_type value );

    /** Declare, set, and check_valid in one call. */
    template<typename OptionType>
    Options &
    declare_set_check( typename OptionType::value_type value );

    /** Checks if the option OptionType has already been declared. */
    template<typename OptionType>
    bool
    is_declared() const;

    /** Parse the option values from the command line, and, if provided, also from the \p optionsFile.
     *  The values specified from the command line (i.e. in the \p argv) have a priority over the ones
     *  in the \p optionsFile. No warning is generated if one option is specified in both \p argv and \p optionsFile.
     *  In case an undeclared option is encountered (either in \p argv or in \p optionsFile),
     *  an exception is thrown.
     *  The parsed values are set to the options. Old values of the options are overwritten without any warnings.
     *  Finally, check_valid() is called for every option that has a value (regardless of whether the option
     *  has got the value as a result of the parsing, or was set earlier). */
    Options &
    parse( int argc, const char ** argv, std::string optionsFile = "" );

    /** Get option object. \n
     * Throws if the option was not declared. */
    template<typename OptionType>
    const OptionType&
    get() const;

    /** Get option object. \n
     *  Throws if the option was not declared. */
    template<typename OptionType>
    OptionType&
    get();

    /** Get the value of the option. \n
     *  Throws if the option was not declared. \n
     *  Throws if the option value was not set.  \n
     *  Use \c is_declared<OptionType>() and \c is_set<OptionType>() to check. */
    template<typename OptionType>
    typename OptionType::value_type
    get_value() const;

    /** Set the option value. \n
     *  Old value, if was available, is overwritten silently. \n
     *  Throws if the option was not declared. \n
     *  No validity check is done. */
    template<typename OptionType>
    Options&
    set_value( typename OptionType::value_type value);

    /** True if the option value was set.
     *  Throws if the option was not declared.*/
    template<typename OptionType>
    bool
    is_set() const;

    /** is_declared() and is_set() in one call */
    template<typename OptionType>
    bool
    is_declared_and_set() const;

    /** set and check_valid (this option only) in one call */
    template<typename OptionType>
    Options&
    set_value_and_check( typename OptionType::value_type value);

    /** Help is generated by boost::program options. Caption, line length, and the width of the description
     *  column are set in the constructor:
     *  Options::Options( const std::string& caption, unsigned lineLength, unsigned minDescriptionLength ); */
    void
    print_help( std::ostream& os = std::cout ) const;

    /** Print a table \n
     *     option_name    : value \n
     *     option_name    : value \n
     *     ... \n
     *  The Option<ValueType>::value(), and not Option<ValueType>::raw_value() are printed. */
    const Options&
    print_values( std::ostream& os = std::cout ) const;

    /** Do arbitrary logic within a chained call.
     *  This is useful in member variable initializer lists,
     *  when only single expression can be executed. E.g.:
     *
     *  MyClass::MyClass( Options options )
     *  : _options( options.declare<OptionA>()
     *                     .declare<OptionB>()
     *                     .call( [&]( Options& o ) {
     *                          if( o.is_declared<OptionC>() ) {
     *                                  o.forget<OptionC>();
     *                                  o.declare<OptionCNew>();
     *                          }
     *                      } )
     *                     .declare<OptionZ>()
     *                     .parse( argc, argv ) );
     *
     *  \p func is a function or a functor with interface equivalent to:
     *
     *      void func(Options& opt);
     *
     *  \p func is called with *this as an argument
     *  The func return value is ignored.
     *  Returns *this    */
    template<typename Func>
    Options&
    call( Func func );

protected:
    /** Checks all declared options, and if one with the same name
     *  is found, and if it is not of type OptionType or derived,
     *  then std::logic_error is thrown. */
    template<typename OptionType>
    void
    check_no_name_collisions() const;

//    /** Checks that
//     *     if the options \p option and \p OptionType are of the same  type,  then they also have the same name,
//     *     if the options \p option and \p OptionType are of different types, then they also have different names */
//    template<typename OptionType>
//    void
//    assert_no_name_collision( const OptionBase& option ) const;
//
//    /** Calls assert_no_name_collision for all declared options */
//    template<typename OptionType>
//    void
//    assert_no_name_collisions() const;

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
                             const char ** argv,
                             variables_map & parsed_options );

    /** Set the values of all options in \p _options from the \p vm*/
    void
    set_from_vm( const variables_map & vm );
};



template<typename OptionType,
         typename std::enable_if< std::is_base_of< OptionBase, OptionType >::value, bool >::type >
OptionType
OptionBase::construct( const Options* options )
{
    auto option = OptionType();
    dynamic_cast<OptionBase*>(&option)->assign_to( options );
    return option;
}



inline std::string
OptionBase::name_long_prefixed() const
{
    return std::string("--") + name();
}



inline void
OptionBase::check_valid() const
{
    std::string error_message;
    if( not is_valid( error_message ) ) {
        throw std::invalid_argument( error_message );
    }
}



inline void
OptionBase::assign_to( const Options* options )
{
    _options = options;
}



inline const Options&
OptionBase::get_options() const
{
    if( nullptr == _options ) {
        throw std::logic_error( std::string("Option ") + typeid(*this).name() + " is not associated with any Options object. assign_to have not been called." );
    }
    return *_options;
}



template<typename OptionType,
         typename std::enable_if< std::is_base_of< OptionBase, OptionType >::value, bool >::type >
const OptionType&
OptionBase::get() const
{
    return get_options().get<OptionType>();
}



template<typename OptionType>
typename OptionType::value_type
OptionBase::get_value() const
{
    return get_options().get_value<OptionType>();
}



template< typename ValueType >
typename Option<ValueType>::value_type
Option<ValueType>::value() const
{
    if( not is_set() ) {
        throw std::runtime_error( std::string("Option ") + name_long_prefixed() + " is not set." );
    }
    return _value.get();
}



template< typename ValueType >
void
Option<ValueType>::set( const value_type& value )
{
    _value = value;
}



template< typename ValueType >
bool
Option<ValueType>::is_set() const
{
    return _value.is_initialized();
}



template< typename ValueType >
std::ostream &
Option<ValueType>::print_value( std::ostream& os ) const
{
    os << value();
    return os;
}



template< typename ValueType >
bool
Option<ValueType>::omit_when_printing() const
{
    return false;
}



template< typename ValueType >
void
Option<ValueType>::declare( boost::program_options::options_description& opt_descr ) const
{
    auto value = boost::program_options::value<value_type>();

    if( default_value().is_initialized() ) {
        value->default_value( default_value().get() );
    }

    //        if( Option().Implicit().is_initialized() ) {
    //            value->implicit_value( Option().Implicit().get() );
    //        }

    opt_descr.add_options()( name().c_str(), value, description().c_str() );
}



template< typename ValueType >
void
Option<ValueType>::set_from_vm( const boost::program_options::variables_map& vm )
{
    using boost::program_options::variable_value;

    if( vm.count( name() ) ) {
        const variable_value& variableValue = vm[ name() ];
        set( variableValue.as<ValueType>() );
    }
}



inline
Options::Options()
: Options( "Avaliable options", 120, 80 )
{}



inline
Options::Options( const std::string& caption,
         unsigned lineLength,
         unsigned minDescriptionLength )
: _caption( caption )
, _lineLength( lineLength )
, _minDescriptionLength( minDescriptionLength )
{}



inline
Options::Options( const Options& options )
: _caption( options._caption )
, _lineLength( options._lineLength )
, _minDescriptionLength( options._minDescriptionLength )
, _options( options._options )
{
    for( auto& option: _options ) {
        option.get().assign_to( this );
    }
}



inline
Options::Options( Options&& options )
: _caption( std::move(options._caption) )
, _lineLength( options._lineLength )
, _minDescriptionLength( options._minDescriptionLength )
, _options( std::move( options._options ) )
{
    for( auto& option: _options ) {
        option.get().assign_to( this );
    }
}



inline Options&
Options::operator=( const Options& other )
{
    _caption              = other._caption;
    _lineLength           = other._lineLength;
    _minDescriptionLength = other._minDescriptionLength;
    _options              = other._options;

    for( auto& option: _options ) {
        option.get().assign_to( this );
    }

    return *this;
}



inline Options&
Options::operator=( Options&& other )
{
    _caption              = std::move( other._caption );
    _lineLength           = std::move( other._lineLength );
    _minDescriptionLength = std::move( other._minDescriptionLength );
    _options              = std::move( other._options );

    for( auto& option: _options ) {
        option.get().assign_to( this );
    }

    return *this;
}



template<typename OptionType,
typename std::enable_if< std::is_base_of< OptionBase, OptionType >::value, bool >::type >
Options & Options::declare()
{
    if( not is_declared<OptionType>() ) {
        check_no_name_collisions<OptionType>();
        _options.push_back( polymorphic<OptionBase>( OptionBase::construct<OptionType>( this ) ) );
    }

    return *this;
}



template<typename TupleType, size_t Index,
typename std::enable_if< ( Index < (std::tuple_size<TupleType>::value) ), bool >::type >
Options & Options::declare()
{
    declare< typename std::tuple_element<Index, TupleType>::type >();
    return declare< TupleType, Index + 1 >();
}



template<typename TupleType, size_t Index,
typename std::enable_if< ( Index == std::tuple_size<TupleType>::value ) , bool >::type >
Options & Options::declare()
{
    return *this;
}



template<typename FirstOptionOrTuple, typename... OtherOptionsOrTuples>
typename  std::enable_if< (sizeof...(OtherOptionsOrTuples) > 0), Options &>::type
Options::declare() {
    declare<FirstOptionOrTuple>();
    return declare<OtherOptionsOrTuples...>();
}



template<typename OptionType,
typename std::enable_if< std::is_base_of< OptionBase, OptionType >::value, bool >::type >
Options &
Options::forget()
{
    auto opt_iter = find_option<OptionType>();

    if( opt_iter == _options.end() ) {
        throw std::logic_error("Option not found");
    }

    _options.erase( opt_iter );

    return *this;
}



template<typename OptionType>
Options &
Options::declare_and_set( typename OptionType::value_type value )
{
    return declare<OptionType>().set_value<OptionType>( value );
}



template<typename OptionType>
Options &
Options::declare_set_check( typename OptionType::value_type value )
{
    declare<OptionType>();
    set_value<OptionType>( value );
    get<OptionType>().check_valid();
    return *this;
}



template<typename OptionType>
bool
Options::is_declared() const
{
    return find_option_const<OptionType>() != _options.cend();
//    bool declared = false;
//    for( const auto& option : _options ) {
//        if( typeid( option.get() ) == typeid(OptionType) ) {
//            assert( not declared && "There must be not more than one instance of type OptionType in the _options vector."  );
//            declared = true;
//        }
//    }
//    return declared;
}



template<typename OptionType>
auto
Options::find_option_const() const -> decltype(_options)::const_iterator
{
    auto found = _options.end();

    for( auto iOption = _options.begin(); iOption < _options.end(); ++iOption ) {
        if( dynamic_cast< const OptionType* >( &(iOption->get()) ) ) {
            if( found != _options.end() ) {
                throw std::logic_error("There must be not more than one instance of type OptionType in the _options vector.");
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
    auto iter = _options.begin();
    std::advance( iter, std::distance( _options.cbegin(), find_option_const<OptionType>() ) );
    return iter;
}



template<typename OptionType>
const OptionType&
Options::get() const
{
    auto iter = find_option_const<OptionType>();
    if( iter == _options.cend() ) {
        throw std::logic_error( std::string("Option ") + typeid(OptionType).name() + " was not declared." );
    }
    return dynamic_cast< const OptionType&>( (*iter).get() );
}



template<typename OptionType>
OptionType&
Options::get()
{
    auto iter = find_option<OptionType>();
    if( iter == _options.cend() ) {
        throw std::logic_error( std::string("Option ") + typeid(OptionType).name() + " was not declared." );
    }
    return dynamic_cast<OptionType&>( (*iter).get() );
}



inline Options::options_description
Options::make_options_description() const
{
    auto opt_descr = options_description( _caption, _lineLength, _minDescriptionLength );

    for( const auto& option: _options ) {
        option.get().declare( opt_descr );
    }

    return opt_descr;
}



inline Options&
Options::parse( int argc, const char ** argv, std::string optionsFile )
{
    auto vm = boost::program_options::variables_map();
    auto opt_descr = make_options_description();

    if( optionsFile.size() ) {
        parse_from_file( opt_descr, optionsFile, vm );
    }

    parse_from_command_line( opt_descr, argc, argv, vm );

    set_from_vm( vm );

    for( const auto& option: _options ) {
        if( option.get().is_set() ) {
            option.get().check_valid();
        }
    }

    return *this;
}



template<typename OptionType>
typename OptionType::value_type
Options::get_value() const
{
    return get<OptionType>().value();
}



template<typename OptionType>
bool
Options::is_set() const
{
    return get<OptionType>().is_set();
}



template<typename OptionType>
bool
Options::is_declared_and_set() const
{
    return is_declared<OptionType>() && is_set<OptionType>();
}



template<typename OptionType>
Options&
Options::set_value( typename OptionType::value_type value)
{
    return get<OptionType>().set( value );
}



template<typename OptionType>
Options&
Options::set_value_and_check( typename OptionType::value_type value)
{
    auto& option = get<OptionType>();
    option.set( value );
    option.check_valid();
    return *this;
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
                                       const char ** argv,
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
Options::print_values( std::ostream& os ) const
{
    size_t maxNameLength = 0;
    for( const auto& option : _options ) {
        if( option.get().omit_when_printing() ) {
            continue;
        }
        maxNameLength = std::max( maxNameLength, option.get().name().size() );
    }

    for( const auto& option : _options ) {
        if( option.get().omit_when_printing() ) {
            continue;
        }
        std::string name_with_spaces;
        name_with_spaces = option.get().name();
        name_with_spaces.append( maxNameLength - name_with_spaces.size(), ' ' );
        os << name_with_spaces << "  : ";
        if( option.get().is_set() ) {
            option.get().print_value( os );
        }
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



template<typename OptionType>
void
Options::check_no_name_collisions() const
{
    for( const auto& option : _options ) {

        if( option.get().name() == OptionType().name()
                and nullptr == dynamic_cast< const OptionType * >( &(option.get()) ) ) {
            throw std::logic_error( std::string("Option ") + typeid(OptionType).name()
                    + " has the same name as " + OptionType().name()
                    + " and the first was not derived from the second." );
        }
    }
}


//
//template<typename OptionType>
//void
//Options::assert_no_name_collision( const OptionBase& option ) const
//{
//    if( typeid(option) == typeid(OptionType ) ) {
//        assert( option.name() == OptionType().name() && "All instances of OptionType must have the same name."  );
//    }
//    else {
//        if( option.name() == OptionType().name() ) {
//            throw std::logic_error( std::string("Options ") + typeid(OptionType).name()
//                    + " has the same name " + OptionType().name()
//                    + " as the previously declared option" + typeid(option).name()
//                    + ". Option names must be unique. " );
//        }
//    }
//}
//
//
//
//template<typename OptionType>
//void
//Options::assert_no_name_collisions() const
//{
//    for( const auto& option : _options ) {
//        assert_no_name_collision<OptionType>( option.get() );
//    }
//}



#endif /* BASE_OPTIONT_H_ */
