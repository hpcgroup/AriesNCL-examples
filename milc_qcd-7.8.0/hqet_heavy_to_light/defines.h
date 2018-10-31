#ifndef _DEFINES_H
#define _DEFINES_H

/* Compiler macros common to all targets in this application */

/* #define SITERAND */	/* Use site-based random number generators */
#define GAUGE_FIX_TOL 1.0e-7 /* For gauge fixing */

#define IF_MASTER  if(this_node==0)
#endif /* _DEFINES_H */
