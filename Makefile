# ----------------------------
# Program Options
# ----------------------------

NAME         ?= BARCHART
ICON         ?= icon.png
DESCRIPTION  ?= "Bar Chart Demo"
COMPRESSED   ?= YES
ARCHIVED     ?= NO

# ----------------------------

CFLAGS ?= -Wall -Wextra -Oz
CXXFLAGS ?= -Wall -Wextra -Oz

# ----------------------------

ifndef CEDEV
$(error CEDEV environment path variable is not set)
endif

include $(CEDEV)/meta/makefile.mk
