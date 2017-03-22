/*
 * OptHelp.h
 *
 *  Created on: Mar 21, 2017
 *      Author: sorokin
 */

#ifndef OPTIONS_OPTHELP_H_
#define OPTIONS_OPTHELP_H_

#include "Options.h"

struct OptHelp: public OptionSwitch {
    std::string  name()                  const override { return "help"; }
    std::string  description()           const override { return "Print help and exit"; }
    Optional     default_value()         const override { return false; }
    void         print_and_exit_if_set() const;
    bool         omit_when_printing()    const override { return true; }
};

inline void OptHelp::print_and_exit_if_set() const {
    if( value() ) {
        get_options().print_help( std::cout );
        exit( EXIT_SUCCESS );
    }
}




#endif /* OPTIONS_OPTHELP_H_ */
