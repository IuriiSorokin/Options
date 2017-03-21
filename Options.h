
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
 *         in a single collection, and providing necessary common interface.
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

    /** Pointer to the corresponding Options object. \n
     *  See also assign_to(...) */
    const Options*
    get_assigned_to() const;
};



/**
 *  Base class for option description.
 */
template< typename ValueType >
class Option : public OptionBase {
public:
    using value_type = ValueType;
    using Optional   = boost::optional<value_type>;

private:
    Optional       _raw_value;

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

    /** Override to add any post-processing of the option value. */
    virtual value_type
    value() const
    { return raw_value(); }

    /** Get Option value, as it was set using:
     *     - Option<ValueType>::set
     *     - Options::parse
     *     - Options::declare_and_set<OptionType>
     *  without the post-processing (as opposed to Option<ValueType>::value() ). \n
     *  Asserts that the value was set. */
    value_type
    raw_value() const;

    /** Override to add a validity check. \n
     *  If the option is required to always have a value, implement the check here. \n
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
    bool
    is_set() const;

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

    // friend void swap( Options& first, Options& second );

    /** Declare a single option. \n
     *  If this option has already been declared, no action is done. \n
     *  Asserts there are no name collisions. */
    template<typename OptionType,
    typename std::enable_if< std::is_base_of< OptionBase, OptionType >::value, bool >::type = true >
    Options &
    declare();

    /** Declare a non-empty tuple of options. \n
     *  Options that have already been declared are omitted silently.
     *  Asserts there are no name collisions.
     *  Note: don't specify the \p Index manually. */
    template<typename TupleType, size_t Index = 0,
    typename std::enable_if< ( Index < (std::tuple_size<TupleType>::value) ), bool >::type = true >
    Options &
    declare();

    /** Declare an empty tuple of options (i.e. do nothing). Necessary for technical reasons. */
    template<typename TupleType, size_t Index = 0,
    typename std::enable_if< ( Index == std::tuple_size<TupleType>::value ),  bool >::type = true >
    Options &
    declare();

    /** Declare an arbitrary combination of options or tuples
     * (including tuples of tuples). \n
     * Options that have already been declared are omitted silently.
     * Asserts there are no name collisions. */
    template<typename FirstOptionOrTuple, typename... OtherOptionsOrTuples>
    typename  std::enable_if< (sizeof...(OtherOptionsOrTuples) > 0), Options &>::type
    declare();

    /** Un-declare an option.
     *  Asserts that the option was declared. */
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

    /** Checks if the option OptionType has already been declared.
     *  Asserts that the option was not declared more that once. */
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

    /** Get the value of the option. \n
     *  Asserts that the option was declared. \n
     *  Asserts that the option has a value. \n
     *  Use \c is_declared<OptionType>() and \c is_set<OptionType>() to check. */
    template<typename OptionType>
    typename OptionType::value_type
    get() const;

    /** If the option was set, returns its value,
     *  otherwise returns \p fallback. \n
     *  Asserts that the option was declared. */
    template<typename OptionType>
    typename OptionType::value_type
    get_value_or( typename OptionType::value_type fallback ) const;

    /** True if the option value was set.
     *  Asserts that the option was declared */
    template<typename OptionType>
    bool
    is_set() const;

    /** Set the option value. \n
     *  Old value, if was available, is overwritten silently. \n
     *  Asserts that the option was declared. \n
     *  No validity check is done. */
    template<typename OptionType>
    Options&
    set( typename OptionType::value_type value);

    /** set and check_valid (this option only) in one call */
    template<typename OptionType>
    Options&
    set_and_check( typename OptionType::value_type value);

    void
    print_help( std::ostream& os ) const;

protected:
    /** If the dynamic type of the \p option is OptionType, asserts that option.name() is the same as OptionType().name(). \n
     *  If the dynamic type of the \p option is different from OptionType, asserts that option.name() is different from OptionType().name(). */
    template<typename OptionType>
    void
    assert_no_name_collision( const OptionBase& option ) const;

    /** Calls assert_no_name_collision for all declared options */
    template<typename OptionType>
    void
    assert_no_name_collisions() const;

    /** returns _options::cend() if the option was not found (not declared) */
    template<typename OptionType>
    decltype(_options)::const_iterator
    find_option() const;

    /** returns _options::end() if the option was not found (not declared) */
    template<typename OptionType>
    decltype(_options)::iterator
    find_option();

    /** Asserts that the option was declared */
    template<typename OptionType>
    const OptionType&
    get_option() const;

    /** Asserts that the option was declared */
    template<typename OptionType>
    OptionType&
    get_option();

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



inline const Options*
OptionBase::get_assigned_to() const
{
    return _options;
}



template< typename ValueType >
typename Option<ValueType>::value_type
Option<ValueType>::raw_value() const
{

    assert( _raw_value.is_initialized() );
    return _raw_value.get();
}



template< typename ValueType >
void
Option<ValueType>::set( const value_type& value )
{
    _raw_value = value;
}



template< typename ValueType >
bool
Option<ValueType>::is_set() const
{
    return _raw_value.is_initialized();
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



//void
//swap( Options& first, Options& second )
//{
//    for( auto& option: first._options ) {
//        assert( option.get().get_assigned_to() == &first );
//    }
//    for( auto& option: second._options ) {
//        assert( option.get().get_assigned_to() == &second );
//    }
//
//    using std::swap;
//    swap( first._caption,              second._caption    );
//    swap( first._lineLength,           second._lineLength );
//    swap( first._minDescriptionLength, second._minDescriptionLength );
//    swap( first._options,              second._options );
//
//    for( auto& option: first._options ) {
//        option.get().assign_to( &first );
//    }
//
//    for( auto& option: second._options ) {
//        option.get().assign_to( &second );
//    }
//}



template<typename OptionType,
typename std::enable_if< std::is_base_of< OptionBase, OptionType >::value, bool >::type >
Options & Options::declare()
{
    assert_no_name_collisions<OptionType>();

    if( not is_declared<OptionType>() ) {
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
    auto found = _options.end();

    for( auto iOption = _options.begin(); iOption < _options.end(); ++iOption ) {
        if( typeid( iOption->get() ) == typeid(OptionType) ) {
            found = iOption;
        }
    }

    assert( found != _options.end() );

    _options.erase( found );

    return *this;
}



template<typename OptionType>
Options &
Options::declare_and_set( typename OptionType::value_type value )
{
    return declare<OptionType>().set<OptionType>( value );
}



template<typename OptionType>
Options &
Options::declare_set_check( typename OptionType::value_type value )
{
    declare<OptionType>();
    set<OptionType>( value );
    get_option<OptionType>()->check_valid();
    return *this;
}



template<typename OptionType>
bool
Options::is_declared() const
{
    bool declared = false;
    for( const auto& option : _options ) {
        if( typeid( option.get() ) == typeid(OptionType) ) {
            assert( not declared && "Must not be declared twice" );
            declared = true;
        }
    }
    return declared;
}



template<typename OptionType>
auto
Options::find_option() const -> decltype(_options)::const_iterator
{
    auto found = _options.end();

    for( auto iOption = _options.begin(); iOption < _options.end(); ++iOption ) {
        if( typeid( iOption->get() ) == typeid(OptionType) ) {
            assert( found == _options.end() && "There must be not more than one instance of type OptionType in the _options vector." );
            found = iOption;
        }
    }

    return found;
}



template<typename OptionType>
auto
Options::find_option() -> decltype(_options)::iterator
{
    const auto& const_this = *this;
    return decltype(_options)::iterator( const_this.find_option<OptionType>() );
}



template<typename OptionType>
const OptionType&
Options::get_option() const
{
    auto iter = find_option<OptionType>();
    assert( iter != _options.cend() && "Option was found" );
    return dynamic_cast< const OptionType&>( (*iter).get() );
}



template<typename OptionType>
OptionType&
Options::get_option()
{
    auto iter = find_option<OptionType>();
    assert( iter != _options.end() && "Option was found" );
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
        option.get().check_valid();
    }

    return *this;
}



template<typename OptionType>
typename OptionType::value_type
Options::get() const
{
    const auto& option = get_option<OptionType>();
    assert( option.is_set() );
    return option.value();
}



template<typename OptionType>
bool
Options::is_set() const
{
    return get_option<OptionType>().is_set();
}



template<typename OptionType>
typename OptionType::value_type
Options::get_value_or( typename OptionType::value_type fallback ) const
{
    return is_set<OptionType>() ? get<OptionType>() : fallback;
}



template<typename OptionType>
Options&
Options::set( typename OptionType::value_type value)
{
    return get_option<OptionType>()->set( value );
}



template<typename OptionType>
Options&
Options::set_and_check( typename OptionType::value_type value)
{
    auto& option = get_option<OptionType>();
    option->set( value );
    option->check_valid();
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



template<typename OptionType>
void
Options::assert_no_name_collision( const OptionBase& option ) const
{
    if( typeid(option) == typeid(OptionType ) ) {
        assert( option.name() == OptionType().name() && "All instances of OptionType must have the same name."  );
    }
    else {
        assert( option.name() != OptionType().name() && "Every option must have a unique name." );
    }
}



template<typename OptionType>
void
Options::assert_no_name_collisions() const
{
    for( const auto& option : _options ) {
        assert_no_name_collision<OptionType>( option.get() );
    }
}



#endif /* BASE_OPTIONT_H_ */
