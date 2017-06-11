#
# This is a GNU Makefile macro that generates rules which build C and assembly
# objects for a specific component with a full destination object directory.
#
# When used, the name of the component should be supplied as the first
# argument. I.e., "MYPROJECT" in:
#
#    $(eval $(call compile_rules MYPROJECT))
#
# Requires the following to be defined at time of use:
#
# <arg>_SRCDIR
#        The absolute path of the project source directory.
#        This variable simply controls how the individual files
#        listed in the <arg>_SRCS variable are interpreted.
#
# <arg>_SRCS
#        The list of source files (.c, .S, .s) to compile for this
#        component. The list will be interpreted as being relative
#        to the <arg>_SRCDIR directory.
#
# <arg>_OBJDIR
#        The absolute path of the directory into which to place all
#        compiled objects.
#
# <arg>_CFLAGS
#        Flags (additional arguments) to pass to the C compiler when
#        compiling this component's C source files.
#
# <arg>_AFLAGS
#        Flags (additional arugments) to pass to the assembler when
#        compiling this component's assembly files. These flags will
#        only be used for .s files. See _CAFLAGS for flags for .S files.
#
# <arg>_CAFLAGS
#        Flags (additional arugments) to pass to the compiler when
#        assembling this component's assembly files. These flags will
#        only be used for .S files. See _AFLAGS for flags for .s files.
# 
# CC
#        The path to the C compiler executable to use. More
#        specifically, this executable should be a "compiler driver",
#        not just a C compiler. A compiler driver is the most visible
#        front-end to a typical compiler suite and its job is to
#        decide which futher tool to use to process a source file,
#        usually by examining the file's suffix (e.g. '.c', '.s',
#        '.S'). This tool is usually named 'cc'.
#
# AS
#        The path to the assembler to use when compiling
#        non-preprocessed (.s) assembly files. Pre-processed files
#        will be passed to the compiler driver (CC) with special
#        flags.
define create_object_list
#
# Convert the list of source files to absolute locations.
#
$1_SRCS_ABS = $$(patsubst %,$$($1_SRCDIR)/%,$$($1_SRCS))

$1_SRCS_C = $$(filter %.c,$$($1_SRCS_ABS))
$1_SRCS_s = $$(filter %.s,$$($1_SRCS_ABS))
$1_SRCS_S = $$(filter %.S,$$($1_SRCS_ABS))

$1_C_OBJS = $$(patsubst $$($1_SRCDIR)/%,$$($1_OBJDIR)/%,$$($1_SRCS_C:.c=.o))
$1_S_OBJS = $$(patsubst $$($1_SRCDIR)/%,$$($1_OBJDIR)/%,$$($1_SRCS_S:.S=.o))
$1_s_OBJS = $$(patsubst $$($1_SRCDIR)/%,$$($1_OBJDIR)/%,$$($1_SRCS_s:.s=.o))

$1_OBJS = $$($1_C_OBJS) $$($1_S_OBJS) $$($1_s_OBJS)

endef

define create_compile_rules

$$(eval $$(call create_object_list,$1))

# Auto-dependency logic.
-include $$($1_C_OBJS:.o=.d)
-include $$($1_S_OBJS:.o=.d)

$$($1_OBJDIR)/%.o: $$($1_SRCDIR)/%.c
	@ [ -d $$(@D) ] || mkdir -p $$(@D)
	$$(CC) $$($1_CFLAGS) -c $$< -o $$@
#	Build dependency fragment, for use during recompilations.
	@$$(CC) $$($1_CFLAGS) -MM $$($1_SRCDIR)/$$*.c -o $$($1_OBJDIR)/$$*.dt
	@sed -e 's|^.*:|$$($1_OBJDIR)/$$*.o:|' < $$($1_OBJDIR)/$$*.dt > \
		 $$($1_OBJDIR)/$$*.d
  
$$($1_OBJDIR)/%.o: $$($1_SRCDIR)/%.s
	@ [ -d $$(@D) ] || mkdir -p $$(@D)
	$$(AS) $$($1_AFLAGS) $$< -o $$@

$($1_OBJDIR)/%.o: $($1_SRCDIR)/%.S
	@ [ -d $$(@D) ] || mkdir -p $$(@D)
	$$(CC) $$($1_CAFLAGS) -c -o $$@ $$^
#	Build dependency fragment, for use during recompilations.
	@$$(CC) $$($1_CFLAGS) -MM $$($1_SRCDIR)/$$*.S -o $$($1_OBJDIR)/$$*.dt
	@sed -e 's|^.*:|$$($1_OBJDIR)/$$*.o:|' < $$($1_OBJDIR)/$$*.dt > \
		 $$($1_OBJDIR)/$$*.d

endef
