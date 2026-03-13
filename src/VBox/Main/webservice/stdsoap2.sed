# $Id: stdsoap2.sed 113398 2026-03-13 23:46:55Z knut.osmundsen@oracle.com $
## @file
# WebService - SED script for inserting a iprt/win/windows.h include
#              before stdsoap2.h in soapStub.h.  This prevents hacking
#              client and server code to do the same when using -Wall.
#

#
# Copyright (C) 2016-2026 Oracle and/or its affiliates.
#
# This file is part of VirtualBox base platform packages, as
# available from https://www.virtualbox.org.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation, in version 3 of the
# License.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <https://www.gnu.org/licenses>.
#
# SPDX-License-Identifier: GPL-3.0-only
#

s/\(#include "stdsoap2\.h"\)/#ifdef RT_OS_WINDOWS\n# include <iprt\/win\/winsock2.h>\n# include <iprt\/win\/ws2tcpip.h>\n# include <iprt\/win\/windows.h>\n#endif\n\1/

# Suppress soapC-5.cpp(23) : warning C6262: Function uses '47056' bytes of stack.  Consider moving some data to heap.
s/\(SOAP_FMAC3  *void  *SOAP_FMAC4  *soap_finsert *[(]\)/#if defined(_MSC_VER) \&\& defined(_PREFAST_)\n# pragma warning(disable:6262)\n#endif\n\1/

