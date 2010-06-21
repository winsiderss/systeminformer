#ifndef _PH_TEMPL_H
#define _PH_TEMPL_H

#define TEMPLATE_(f,T) f##_##T
#define T___(f,T) TEMPLATE_(f,T)

#endif
