/* $Id: msiDarwinDescriptorDecoder.cpp 113399 2026-03-13 23:48:15Z knut.osmundsen@oracle.com $ */
/** @file
 * msiDarwinDescriptorDecoder
 */

/*
 * Copyright (C) 2016-2026 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */


#include <stdio.h>
#include <iprt/win/windows.h>  /* Avoid -Wall warnings. */



typedef DWORD (WINAPI *PFNMSIDECOMPOSEDESCRIPTORW)(PCWSTR pwszDescriptor,
                                                   LPWSTR pwszProductCode /*[40]*/,
                                                   LPWSTR pwszFeatureId /*[40]*/,
                                                   LPWSTR pwszComponentCode /*[40]*/,
                                                   DWORD *poffArguments);

int wmain(int cArgs, wchar_t **papwszArgs)
{
    HMODULE hmodMsi = LoadLibrary("msi.dll");
    if (!hmodMsi)
    {
        fprintf(stderr, "Failed to load msi.dll\n");
        return 1;
    }
    PFNMSIDECOMPOSEDESCRIPTORW pfnMsiDecomposeDescriptorW;
    pfnMsiDecomposeDescriptorW = (PFNMSIDECOMPOSEDESCRIPTORW)GetProcAddress(hmodMsi, "MsiDecomposeDescriptorW");
    if (!pfnMsiDecomposeDescriptorW)
    {
        fprintf(stderr, "Failed to resolve 'MsiDecomposeDescriptorW'\n");
        return 1;
    }

    int rcExit = 0;
    for (int iArg = 1; iArg < cArgs; iArg++)
    {
        wchar_t wszProductCode[40]   = { 0 };
        wchar_t wszFeatureId[40]     = { 0 };
        wchar_t wszComponentCode[40] = { 0 };
        DWORD   offArguments = ~(DWORD)0;
        DWORD dwErr = pfnMsiDecomposeDescriptorW(papwszArgs[iArg], wszProductCode, wszFeatureId, wszComponentCode, &offArguments);
        if (dwErr == 0)
            fprintf(stderr,
                    "#%d: '%ls'\n"
                    " ->       Product=%ls\n"
                    " ->     FeatureId=%ls\n"
                    " -> ComponentCode=%ls\n"
                    " ->  offArguments=%#lx (%lu)\n"
                    , iArg, papwszArgs[iArg], wszProductCode, wszFeatureId, wszComponentCode, offArguments, offArguments);
        else
        {
            fprintf(stderr,
                    "#%d: '%ls'\n"
                    " -> error %lu (%#lx)\n"
                    , iArg, papwszArgs[iArg], dwErr, dwErr);
            rcExit = 1;
        }
    }

    return rcExit;
}

