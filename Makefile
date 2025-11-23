# ----------------------------
# Program Options
# ----------------------------

NAME         ?= PIECHART
ICON         ?= icon.png
DESCRIPTION  ?= "Pie Chart Demo"
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
