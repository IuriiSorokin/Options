
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

template< typename ValueType >
class Option;

class Options;


class OptionBase
{
    friend class Options;

protected:
    using variables_map       = boost::program_options::variables_map;
    using options_description = boost::program_options::options_description;
    const Options* _options = nullptr;

    OptionBase() = default;

    template<typename OptionType>
    static std::unique_ptr<OptionBase>
    construct( const Options* options )
    {
        auto option = std::unique_ptr<OptionBase>( new OptionType() );
        option->_options = options;
        return option;
    }

public:
    virtual
    ~OptionBase() = default;

    virtual std::string
    name() const = 0;

    virtual std::string
    group() const
    { return ""; }

    virtual std::string
    description() const
    { return ""; };

protected:
    virtual void
    set_from_vm( const variables_map& vm ) = 0;

    virtual void
    declare( options_description& description ) const = 0;
};



template< typename ValueType >
class Option :  public OptionBase {
public:
    using value_type = ValueType;
    using Optional = boost::optional<value_type>;

private:
    Optional _raw_value;

public:
    virtual
    ~Option() = default;

    virtual Optional
    default_value() const
    {
        return Optional();
    }

    virtual bool
    is_valid( std::string& error_message ) const
    {
        error_message = "";
        return true;
    }

    bool
    is_set() const
    {
        return _raw_value.is_initialized();
    }

    void
    set( const value_type& value )
    {
        _raw_value = value;
    }

    value_type
    raw_value() const
    {
        assert( _raw_value.is_initialized() );
        return _raw_value.get();
    }

    virtual value_type
    value() const
    {
        return raw_value();
    }

    void
    check_valid() const
    {
        std::string error;
        if( ! is_valid( error ) ) {
            throw std::invalid_argument( error );
        }
    }

protected:
    virtual void
    set_from_vm( const variables_map& vm ) override final
    {
        using boost::program_options::variable_value;

        if( vm.count( name() ) ) {
            const variable_value& variableValue = vm[ name() ];
            set( variableValue.as<ValueType>() );
        }
    }

    virtual void
    declare( options_description& opt_descr ) const override
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
};



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
             unsigned minDescriptionLength = 80 )
    : _caption( caption )
    , _lineLength( lineLength )
    , _minDescriptionLength( minDescriptionLength )
    {}

    Options( const Options& options ) = delete;
    //    : _caption( options._caption )
    //    , _lineLength( options._lineLength )
    //    , _minDescriptionLength( options._minDescriptionLength )
    //    {
    //        for( auto& option: options._options ) {
    //            _options
    //        }
    //    }

    Options( Options&& options )
    : _caption( std::move(options._caption) )
    , _lineLength( options._lineLength )
    , _minDescriptionLength( options._minDescriptionLength )
    , _options( std::move( options._options ) )
    {
        for( auto& option: _options ) {
            option->_options = this;
        }
    }

    /** Signle option */
    template<typename OptionType,
    typename std::enable_if< std::is_base_of< Option<typename OptionType::value_type>, OptionType >::value, bool >::type = true >
    Options &
    declare();

    /** A tuple of options */
    template<typename TupleType, size_t Index = 0,
    typename std::enable_if< ( Index < (std::tuple_size<TupleType>::value) ), bool >::type = true >
    Options &
    declare();

    /** A tuple of options */
    template<typename TupleType, size_t Index = 0,
    typename std::enable_if< ( Index == std::tuple_size<TupleType>::value ),  bool >::type = true >
    Options &
    declare();

    /** A tuple of options or tuples */
    template<typename FirstOptionOrTuple, typename... OtherOptionsOrTuples>
    typename  std::enable_if< (sizeof...(OtherOptionsOrTuples) > 0), Options &>::type
    declare();

    Options&
    parse( int argc, const char ** argv, std::string optionsFile = "" );

    template<typename OptionType>
    typename OptionType::value_type
    get() const
    {
        return find<OptionType>()->value();
    }

    template<typename OptionType>
    bool
    is_set() const
    {
        return find<OptionType>()->is_set();
    }

    template<typename OptionType>
    typename OptionType::value_type
    get_value_or( typename OptionType::value_type fallback ) const
    {
        if( is_set<OptionType>() ) {
            return get<OptionType>();
        }
        return fallback;
    }

    template<typename OptionType>
    Options&
    set( typename OptionType::value_type value)
    {
        return find<OptionType>()->set( value );
    }

    void
    print_help( std::ostream& os ) const;

protected:
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



template<typename OptionType,
typename std::enable_if< std::is_base_of< Option<typename OptionType::value_type>, OptionType >::value, bool >::type >
Options & Options::declare()
{
    bool already_declared = false;

    for( auto& existingOption : _options ) {
        if( existingOption->name() == OptionType().name() ) {
            assert( typeid(*existingOption) == typeid(OptionType) && "Attempting to declare two different options with the same type." );
            assert( not already_declared && "Found two OptionType objects in _options vector. This must never happen." );
            already_declared = true;
        }
        else {
            assert( typeid(*existingOption) != typeid(OptionType) && "Two instances of the same Option must have the same name, but they don't." );
        }
    }

    if( not already_declared ) {
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




#endif /* BASE_OPTIONT_H_ */
