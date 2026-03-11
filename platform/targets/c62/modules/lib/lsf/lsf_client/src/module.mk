#includings and flags
module_libs += -lurpc -ldbg

module_cflags +=

module_includes += -I$(subdirectory) \
					 -I$(subdirectory)/fifo \
					 -I$(subdirectory)/mtrace \
					 -I$(TOPDIR)/include/urpc \
					 -I$(TOPDIR)/include/debug

# 全部
local_src := $(wildcard $(subdirectory)/*.c) \
			 $(wildcard $(subdirectory)/mtrace/*.c)

# 测试用例
test_src := $(wildcard $(subdirectory)/test_*.c)

# $(eval $(call make-library, $(subdirectory)/libcodec.a, $(local_src)))

# 导出module.mk时, 排除测试用例
module_sources += $(filter-out $(test_src),$(local_src))
