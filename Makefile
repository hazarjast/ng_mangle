KMOD	=	ng_mangle
SRCS	=	ng_mangle.c
KO	=	${KMOD}.ko
#COPTS	=	-g


KLDLOAD		= /sbin/kldload
KLDUNLOAD	= /sbin/kldunload

load: ${KO}
	${KLDLOAD} -v ./${KO}

unload: ${KO}
	${KLDUNLOAD} -v -n ${KO}


.include <bsd.kmod.mk>
