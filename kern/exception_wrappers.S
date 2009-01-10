/** @file keyboard_wrapper.S
 *  @brief Wrapper for calling the exception handler
 *
 *
 *  @author Anshuman P.Kanetkar (apk)
 */
#include <exception_wrappers.h>


DEFINE_EXCEPTION_WRAPPER(generic)	

DEFINE_EXCEPTION_WRAPPER(div)

DEFINE_EXCEPTION_WRAPPER(dbg)

DEFINE_EXCEPTION_WRAPPER(ovf)

DEFINE_EXCEPTION_WRAPPER(inv)

DEFINE_EXCEPTION_WRAPPER(dbl)

DEFINE_EXCEPTION_WRAPPER(seg)

DEFINE_EXCEPTION_WRAPPER(stk)

DEFINE_EXCEPTION_WRAPPER(gpf)

DEFINE_EXCEPTION_WRAPPER(pgf)
