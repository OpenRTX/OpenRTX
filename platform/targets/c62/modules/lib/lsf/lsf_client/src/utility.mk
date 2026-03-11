
# $(subdirectory)
subdirectory = $(patsubst %/module.mk,%, \
				$(word \
				$(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST)))

# Collect information from each module in these four variables.
# Initialize them here as simple variables.
module_sources :=
module_includes :=
module_cflags :=
module_libs :=
