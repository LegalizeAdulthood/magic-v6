
/*
 * Define NEED_HELPER if the system does not support SIGIO on sockets
 */
#if defined( ultrix ) || defined( hp9000s300 )
#    define	NEED_HELPER
#else
#    ifdef NO_SIGIO		/* this comes from CFLAGS at the top */
#	define	NEED_HELPER
#    endif
#endif

