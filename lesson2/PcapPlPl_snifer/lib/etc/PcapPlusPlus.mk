
### COMMON ###

# includes
PCAPPP_INCLUDES := -I../GB_Cpp_Network/PcapPlPl_snifer/include/pcapplusplus


# libs
PCAPPP_LIBS := -lPcap++ -lPacket++ -lCommon++

# post build
PCAPPP_POST_BUILD :=

# build flags
PCAPPP_BUILD_FLAGS := -fPIC

ifdef PCAPPP_ENABLE_CPP_FEATURE_DETECTION
	PCAPPP_BUILD_FLAGS += -DPCAPPP_CPP_FEATURE_DETECTION -std=c++11
endif

ifndef CXXFLAGS
CXXFLAGS := -O2 -g -Wall
endif

PCAPPP_BUILD_FLAGS += $(CXXFLAGS)
### LINUX ###

# libs
PCAPPP_LIBS += -lpcap -lpthread

# allow user to add custom LDFLAGS
PCAPPP_BUILD_FLAGS += $(LDFLAGS)
