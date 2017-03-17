
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

protected:
    OptionBase() = default;

    /** To avoid forcing the users to define (or explicitly inherit) a non-trivial constructor
     *  in their implementations of Option<ValueType> */
    template<typename OptionType>
    static std::unique_ptr<OptionBase>
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

protected:
    /** see Option<ValueType>::declare(...) */
    virtual void
    declare( boost::program_options::options_description& description ) const = 0;

    /** see Option<ValueType>::set_from_vm(...) */
    virtual void
    set_from_vm( const boost::program_options::variables_map& vm ) = 0;

    /** see Option<ValueType>::set_from_vm(...) */
    virtual void
    assign_to( const Options* options ) = 0;
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
    const Options* _options = nullptr;
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
     *  Asserts that the option was declared. \n
     *  Asserts that the value was set. */
    value_type
    raw_value() const;

    /** Override to add a validity check. \n
     *  A meaningful explanation why the option value is invalid
     *  may be returned via \p error_message.
     *  If the option must always have a value, indicate it here. \n  */
    virtual bool
    is_valid( std::string& error_message ) const
    { return true; }

    /** Call to this->is_valid and throw std::invalid_argument (with the
     *  returned \p error_message), if the value is invalid. */
    void
    check_valid() const;

    /** Explicitly set the value of the option. \n
     *  No validity check is performed. */
    void
    set( const value_type& value );

    /** Check if the option value was set. \n
     *  Option value can be set with:
     *     - Option<ValueType>::set
     *     - Options::parse
     *     - Options::declare_and_set<OptionType> */
    bool
    is_set() const;

protected:
    /** Store the pointer to the corresponding Options object, in order
     *  to be able to access the values of other options from the
     *  Option<ValueType>::is_valid and Option<ValueType>::value functions. */
    virtual void
    assign_to( const Options* options ) override;

    /** Declare this option in the \p opt_descr. \n
     *  Needed by Options::parse to construct
     *  the boost::program_options::options_description
     *  to perform the parsing.                          */
    virtual void
    declare( boost::program_options::options_description& opt_descr ) const override;

    /** Set the value from the variables_map. \n
     *  Should be called from Options::parse to set the option value,
     *  when parsing is completed, and the parsed values are stored in the
     *  type-erased boost::program_options::variables_map.                  */
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
    std::vector< std::unique_ptr< OptionBase > > _options;

public:
    Options( const std::string& caption = "Available options",
             unsigned lineLength = 120,
             unsigned minDescriptionLength = 80 );

    /** It would require to implement a polymorphic clone of Option<ValueType>,
     *  which in turn would require the user to use either CRTP or macros when
     *  defining a new option, and I don't want either. \n
     *  Feel free to propose your solution. */
    Options( const Options& options ) = delete;

    Options( Options&& options );

    /** Declare a single option. \n
     *  If this option has already been declared, no action is done. \n
     *  Asserts there are no name collisions. */
    template<typename OptionType,
    typename std::enable_if< std::is_base_of< Option<typename OptionType::value_type>, OptionType >::value, bool >::type = true >
    Options &
    declare();

    /** Declare a non-empty tuple of options. \n
     *  If some of the options in the tuple have already been declared, these options are omitted silently.
     *  Asserts there are no name collisions.
     *  Note: don't specify the \p Index manually. */
    template<typename TupleType, size_t Index = 0,
    typename std::enable_if< ( Index < (std::tuple_size<TupleType>::value) ), bool >::type = true >
    Options &
    declare();

    /** "Declare" an empty tuple of options. Necessary for technical reasons. */
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

    /** Declare a single option and set its value. \n
     *  Asserts that the option has not yet been declared. */
    template<typename OptionType>
    Options &
    declare_and_set( typename OptionType::value_type value );

    /** Checks if the option OptionType has already been declared.
     *  Asserts that the option was not declared more that once. */
    template<typename OptionType>
    bool
    is_declared() const;

    /** Set the option OptionType to the given value. \n
     *  If the option has not been declared, declare it (assert there is no name collision). */
    template<typename OptionType>
    Options &
    force( typename OptionType::value_type value );

    /** Parse the command line arguments, and, if provided, also the optionsFile.
     *  Command line arguments have a priority over the ones in the optionsFile,
     *  and no warning is given in case of a conflict. The parsed values are set
     *  to the options. It is not required, that all options obtain a value.
     *  Finally, Option<ValueType>::check_valid() is called for every option that
     *  has a value (independently, on whether the option has got the value as a result
     *  of the parsing, or was set with Option<ValueType>::set(...) earlier). */
    Options &
    parse( int argc, const char ** argv, std::string optionsFile = "" );

    /** Get the value of the option. \n
     *  Throws if the option was not declared, or if the value was not set.
     *  Use is_declared<OptionType>() and
     *  is_set<OptionType>() to check.*/
    template<typename OptionType>
    typename OptionType::value_type
    get() const;

    /** Get the value of the option. \n
     *  Throws if the option was not declared.
     *  If the options was not set, returns the \p fallback value. */
    template<typename OptionType>
    typename OptionType::value_type
    get_value_or( typename OptionType::value_type fallback ) const;

    /** If the option value was set.
     *  Throws if the option was not declared. */
    template<typename OptionType>
    bool
    is_set() const;

    /** Set the */
    template<typename OptionType>
    Options&
    set( typename OptionType::value_type value);

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

    template<typename OptionType>
    const OptionType*
    find() const;

    template<typename OptionType>
    OptionType*
    find();

    options_description
    make_options_description() const;

    void
    parse_from_file( const options_description & optionsDescription,
                     std::string optionsFile,
                     variables_map & parsedOptions );

    void
    parse_from_command_line( const options_description & opt_descr,
                             int argc,
                             const char ** argv,
                             variables_map & parsed_options );

    void
    set_from_vm( const variables_map & vm )
    {
        for( auto & option : _options ) {
            option->set_from_vm( vm );
        }
    }

};



template<typename OptionType>
std::unique_ptr<OptionBase>
OptionBase::construct( const Options* options )
{
    auto option = std::unique_ptr<OptionBase>( new OptionType() );
    option->assign_to( options );
    return option;
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
Option<ValueType>::check_valid() const
{
    std::string error;
    if( not is_valid( error ) ) {
        throw std::invalid_argument( error );
    }
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
Option<ValueType>::assign_to( const Options* options )
{
    _options = options;
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
Options::Options( const std::string& caption,
         unsigned lineLength,
         unsigned minDescriptionLength )
: _caption( caption )
, _lineLength( lineLength )
, _minDescriptionLength( minDescriptionLength )
{}



inline
Options::Options( Options&& options )
: _caption( std::move(options._caption) )
, _lineLength( options._lineLength )
, _minDescriptionLength( options._minDescriptionLength )
, _options( std::move( options._options ) )
{
    for( auto& option: _options ) {
        option->assign_to( this );
    }
}



template<typename OptionType,
typename std::enable_if< std::is_base_of< Option<typename OptionType::value_type>, OptionType >::value, bool >::type >
Options & Options::declare()
{
    assert_no_name_collisions<OptionType>();

    if( not is_declared<OptionType>() ) {
        _options.push_back( OptionBase::construct<OptionType>( this ) );
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



template<typename OptionType>
bool
Options::is_declared() const
{
    bool declared = false;
    for( const auto& option : _options ) {
        if( typeid(*option) == typeid(OptionType) ) {
            assert( not declared );
            declared = true;
        }
    }
    return declared;
}



template<typename OptionType>
const OptionType*
Options::find() const
{
    const OptionType* found = nullptr;

    for( const auto& option : _options ) {
        if( typeid(*option) == typeid(OptionType) ) {
            assert( not found && "There must be not more than one instance of type OptionType in the _options vector." );
            found = dynamic_cast<const OptionType*>( option.get() );
        }
    }

    return found;
}



template<typename OptionType>
OptionType*
Options::find()
{
    OptionType* found = nullptr;

    for( const auto& option : _options ) {
        if( typeid(*option) == typeid(OptionType) ) {
            assert( not found && "There must be not more than one instance of type OptionType in the _options vector." );
            found = dynamic_cast<OptionType*>( option.get() );
        }
    }

    return found;
}



Options::options_description
Options::make_options_description() const
{
    auto opt_descr = options_description( _caption, _lineLength, _minDescriptionLength );

    for( const auto& option: _options ) {
        option->declare( opt_descr );
    }

    return opt_descr;
}




Options&
Options::parse( int argc, const char ** argv, std::string optionsFile )
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
Options::get() const
{
    const auto& option = find<OptionType>();
    if( nullptr == option ) {
        throw std::logic_error( std::string("Option ") + "--"+ OptionType().name() + " was not declared." );
    }
    if( not option->is_set() ) {
        throw std::invalid_argument( std::string("Option ") + "--"+ OptionType().name() + " was not set." );
    }
    return option->value();
}



template<typename OptionType>
bool
Options::is_set() const
{
    return find<OptionType>()->is_set();
}



template<typename OptionType>
typename OptionType::value_type
Options::get_value_or( typename OptionType::value_type fallback ) const
{
    if( is_set<OptionType>() ) {
        return get<OptionType>();
    }
    return fallback;
}



template<typename OptionType>
Options&
Options::set( typename OptionType::value_type value)
{
    return find<OptionType>()->set( value );
}



void Options::parse_from_file( const options_description& optionsDescription,
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



void Options::parse_from_command_line( const options_description & opt_descr,
                                       int argc,
                                       const char ** argv,
                                       variables_map & parsed_options  )
{
    boost::program_options::store( boost::program_options::command_line_parser( argc, argv ).options( opt_descr ).run(), parsed_options );
    boost::program_options::notify( parsed_options );
}



void
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
        assert_no_name_collision<OptionType>( *option );
    }
}



#endif /* BASE_OPTIONT_H_ */
