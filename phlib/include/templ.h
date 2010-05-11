#ifndef TEMPL_H
#define TEMPL_H

#define TEMPLATE_(f,T) f##_##T
#define T___(f,T) TEMPLATE_(f,T)

#endif
