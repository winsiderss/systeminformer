/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010
 *
 */

#ifndef _PH_TEMPL_H
#define _PH_TEMPL_H

#define TEMPLATE_(f,T) f##_##T
#define T___(f,T) TEMPLATE_(f,T)

#endif
