# ----------------------------
# Program Options
# ----------------------------

NAME         ?= MATHSUIT
ICON         ?= icon.png
DESCRIPTION  ?= "MAT142 Exam Suite"
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
