/*
 * Copyright 2006 Robert Reif
 *
 * netapi32 local group functions
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "lmcons.h"
#include "lmaccess.h"
#include "lmapibuf.h"
#include "lmerr.h"
#include "winreg.h"
#include "ntsecapi.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#define NTOS_MODE_USER
#include <ndk/rtlfuncs.h>
#include "ntsam.h"
#include "netapi32.h"

WINE_DEFAULT_DEBUG_CHANNEL(netapi32);


typedef struct _ENUM_CONTEXT
{
    SAM_HANDLE ServerHandle;
    SAM_HANDLE BuiltinDomainHandle;
    SAM_HANDLE AccountDomainHandle;

    SAM_ENUMERATE_HANDLE EnumerationContext;
    PSAM_RID_ENUMERATION EnumBuffer;
    ULONG EnumReturned;

} ENUM_CONTEXT, *PENUM_CONTEXT;

static SID_IDENTIFIER_AUTHORITY NtAuthority = {SECURITY_NT_AUTHORITY};


static
NTSTATUS
GetAccountDomainSid(PSID *AccountDomainSid)
{
    PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomainInfo = NULL;
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE PolicyHandle = NULL;
    ULONG Length = 0;
    NTSTATUS Status;

    memset(&ObjectAttributes, 0, sizeof(LSA_OBJECT_ATTRIBUTES));

    Status = LsaOpenPolicy(NULL,
                           &ObjectAttributes,
                           POLICY_VIEW_LOCAL_INFORMATION,
                           &PolicyHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsaOpenPolicy failed (Status %08lx)\n", Status);
        return Status;
    }

    Status = LsaQueryInformationPolicy(PolicyHandle,
                                       PolicyAccountDomainInformation,
                                       (PVOID *)&AccountDomainInfo);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsaQueryInformationPolicy failed (Status %08lx)\n", Status);
        goto done;
    }

    Length = RtlLengthSid(AccountDomainInfo->DomainSid);

    *AccountDomainSid = RtlAllocateHeap(RtlGetProcessHeap(), 0, Length);
    if (*AccountDomainSid == NULL)
    {
        ERR("Failed to allocate SID\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    memcpy(*AccountDomainSid, AccountDomainInfo->DomainSid, Length);

done:
    if (AccountDomainInfo != NULL)
        LsaFreeMemory(AccountDomainInfo);

    LsaClose(PolicyHandle);

    return Status;
}


static
NTSTATUS
GetBuiltinDomainSid(PSID *BuiltinDomainSid)
{
    PSID Sid = NULL;
    PULONG Ptr;
    NTSTATUS Status = STATUS_SUCCESS;

    *BuiltinDomainSid = NULL;

    Sid = RtlAllocateHeap(RtlGetProcessHeap(),
                          0,
                          RtlLengthRequiredSid(1));
    if (Sid == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = RtlInitializeSid(Sid,
                              &NtAuthority,
                              1);
    if (!NT_SUCCESS(Status))
        goto done;

    Ptr = RtlSubAuthoritySid(Sid, 0);
    *Ptr = SECURITY_BUILTIN_DOMAIN_RID;

    *BuiltinDomainSid = Sid;

done:
    if (!NT_SUCCESS(Status))
    {
        if (Sid != NULL)
            RtlFreeHeap(RtlGetProcessHeap(), 0, Sid);
    }

    return Status;
}


/************************************************************
 *                NetLocalGroupAdd  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupAdd(
    LPCWSTR servername,
    DWORD level,
    LPBYTE buf,
    LPDWORD parm_err)
{
    FIXME("(%s %d %p %p) stub!\n", debugstr_w(servername), level, buf,
          parm_err);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupAddMember  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupAddMember(
    LPCWSTR servername,
    LPCWSTR groupname,
    PSID membersid)
{
    FIXME("(%s %s %p) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), membersid);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupAddMembers  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupAddMembers(
    LPCWSTR servername,
    LPCWSTR groupname,
    DWORD level,
    LPBYTE buf,
    DWORD totalentries)
{
    FIXME("(%s %s %d %p %d) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), level, buf, totalentries);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupDel  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupDel(
    LPCWSTR servername,
    LPCWSTR groupname)
{
    FIXME("(%s %s) stub!\n", debugstr_w(servername), debugstr_w(groupname));
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupDelMember  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupDelMember(
    LPCWSTR servername,
    LPCWSTR groupname,
    PSID membersid)
{
    FIXME("(%s %s %p) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), membersid);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupDelMembers  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupDelMembers(
    LPCWSTR servername,
    LPCWSTR groupname,
    DWORD level,
    LPBYTE buf,
    DWORD totalentries)
{
    FIXME("(%s %s %d %p %d) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), level, buf, totalentries);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupEnum  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupEnum(
    LPCWSTR servername,
    DWORD level,
    LPBYTE* bufptr,
    DWORD prefmaxlen,
    LPDWORD entriesread,
    LPDWORD totalentries,
    PDWORD_PTR resumehandle)
{
    PENUM_CONTEXT EnumContext = NULL;
    PSID DomainSid = NULL;
    ULONG i;

    NET_API_STATUS ApiStatus = NERR_Success;
    NTSTATUS Status = STATUS_SUCCESS;


    FIXME("(%s %d %p %d %p %p %p) stub!\n", debugstr_w(servername),
          level, bufptr, prefmaxlen, entriesread, totalentries, resumehandle);

    *entriesread = 0;
    *totalentries = 0;
    *bufptr = NULL;

    if (resumehandle != NULL && *resumehandle != 0)
    {
        EnumContext = (PENUM_CONTEXT)resumehandle;
    }
    else
    {
        ApiStatus = NetApiBufferAllocate(sizeof(ENUM_CONTEXT), (PVOID*)&EnumContext);
        if (ApiStatus != NERR_Success)
            goto done;

        EnumContext->EnumerationContext = 0;
        EnumContext->EnumBuffer = NULL;
        EnumContext->EnumReturned = 0;

        Status = SamConnect(NULL,
                            &EnumContext->ServerHandle,
                            SAM_SERVER_CONNECT | SAM_SERVER_LOOKUP_DOMAIN,
                            NULL);
        if (!NT_SUCCESS(Status))
        {
            ERR("SamConnect failed (Status %08lx)\n", Status);
            ApiStatus = NetpNtStatusToApiStatus(Status);
            goto done;
        }

        Status = GetAccountDomainSid(&DomainSid);
        if (!NT_SUCCESS(Status))
        {
            ERR("GetAccountDomainSid failed (Status %08lx)\n", Status);
            ApiStatus = NetpNtStatusToApiStatus(Status);
            goto done;
        }

        Status = SamOpenDomain(EnumContext->ServerHandle,
                               DOMAIN_LIST_ACCOUNTS,
                               DomainSid,
                               &EnumContext->AccountDomainHandle);

        RtlFreeHeap(RtlGetProcessHeap(), 0, DomainSid);

        if (!NT_SUCCESS(Status))
        {
            ERR("SamOpenDomain failed (Status %08lx)\n", Status);
            ApiStatus = NetpNtStatusToApiStatus(Status);
            goto done;
        }

        Status = GetBuiltinDomainSid(&DomainSid);
        if (!NT_SUCCESS(Status))
        {
            ERR("GetAccountDomainSid failed (Status %08lx)\n", Status);
            ApiStatus = NetpNtStatusToApiStatus(Status);
            goto done;
        }

        Status = SamOpenDomain(EnumContext->ServerHandle,
                               DOMAIN_LIST_ACCOUNTS,
                               DomainSid,
                               &EnumContext->BuiltinDomainHandle);

        RtlFreeHeap(RtlGetProcessHeap(), 0, DomainSid);

        if (!NT_SUCCESS(Status))
        {
            ERR("SamOpenDomain failed (Status %08lx)\n", Status);
            ApiStatus = NetpNtStatusToApiStatus(Status);
            goto done;
        }
    }

    while (TRUE)
    {
        Status = SamEnumerateAliasesInDomain(EnumContext->BuiltinDomainHandle,
                                             &EnumContext->EnumerationContext,
                                             (PVOID *)&EnumContext->EnumBuffer,
                                             prefmaxlen,
                                             &EnumContext->EnumReturned);

        TRACE("SamEnumerateAliasesInDomain returned (Status %08lx)\n", Status);

        if (!NT_SUCCESS(Status))
        {
            ERR("SamEnumerateAliasesInDomain failed (Status %08lx)\n", Status);
            ApiStatus = NetpNtStatusToApiStatus(Status);
            goto done;
        }

        TRACE("EnumContext: %lu\n", EnumContext);
        TRACE("EnumReturned: %lu\n", EnumContext->EnumReturned);
        TRACE("EnumBuffer: %p\n", EnumContext->EnumBuffer);

        for (i = 0; i < EnumContext->EnumReturned; i++)
        {
            TRACE("RID: %lu\n", EnumContext->EnumBuffer[i].RelativeId);
            TRACE("Name: %p\n", EnumContext->EnumBuffer[i].Name.Buffer);
            TRACE("Name: %S\n", EnumContext->EnumBuffer[i].Name.Buffer);
            if (EnumContext->EnumBuffer[i].Name.Buffer != NULL)
            {
                TRACE("Name: %hx\n", EnumContext->EnumBuffer[i].Name.Buffer[0]);
            }
        }

        if (Status != STATUS_MORE_ENTRIES)
            break;
    }


done:
    if (resumehandle == NULL || ApiStatus != ERROR_MORE_DATA)
    {
        if (EnumContext != NULL)
        {
            if (EnumContext->BuiltinDomainHandle != NULL)
                SamCloseHandle(EnumContext->BuiltinDomainHandle);

            if (EnumContext->AccountDomainHandle != NULL)
                SamCloseHandle(EnumContext->AccountDomainHandle);

            if (EnumContext->ServerHandle != NULL)
                SamCloseHandle(EnumContext->ServerHandle);

            if (EnumContext->EnumBuffer != NULL)
            {
                for (i = 0; i < EnumContext->EnumReturned; i++)
                {
                    SamFreeMemory(EnumContext->EnumBuffer[i].Name.Buffer);
                }

                SamFreeMemory(EnumContext->EnumBuffer);
            }

            NetApiBufferFree(EnumContext);
            EnumContext = NULL;
        }
    }


    if (resumehandle != NULL)
        *resumehandle = (DWORD_PTR)EnumContext;

    return ApiStatus;
}

/************************************************************
 *                NetLocalGroupGetInfo  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupGetInfo(
    LPCWSTR servername,
    LPCWSTR groupname,
    DWORD level,
    LPBYTE* bufptr)
{
    FIXME("(%s %s %d %p) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), level, bufptr);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupGetMembers  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupGetMembers(
    LPCWSTR servername,
    LPCWSTR localgroupname,
    DWORD level,
    LPBYTE* bufptr,
    DWORD prefmaxlen,
    LPDWORD entriesread,
    LPDWORD totalentries,
    PDWORD_PTR resumehandle)
{
    FIXME("(%s %s %d %p %d, %p %p %p) stub!\n", debugstr_w(servername),
          debugstr_w(localgroupname), level, bufptr, prefmaxlen, entriesread,
          totalentries, resumehandle);

    if (level == 3)
    {
        WCHAR userName[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD userNameLen;
        DWORD len,needlen;
        PLOCALGROUP_MEMBERS_INFO_3 ptr;

        /* still a stub,  current user is belonging to all groups */

        *totalentries = 1;
        *entriesread = 0;

        userNameLen = MAX_COMPUTERNAME_LENGTH + 1;
        GetUserNameW(userName,&userNameLen);
        needlen = sizeof(LOCALGROUP_MEMBERS_INFO_3) +
             (userNameLen+2) * sizeof(WCHAR);
        if (prefmaxlen != MAX_PREFERRED_LENGTH)
            len = min(prefmaxlen,needlen);
        else
            len = needlen;

        NetApiBufferAllocate(len, (LPVOID *) bufptr);
        if (len < needlen)
            return ERROR_MORE_DATA;

        ptr = (PLOCALGROUP_MEMBERS_INFO_3)*bufptr;
        ptr->lgrmi3_domainandname = (LPWSTR)(*bufptr+sizeof(LOCALGROUP_MEMBERS_INFO_3));
        lstrcpyW(ptr->lgrmi3_domainandname,userName);

        *entriesread = 1;
    }

    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupSetInfo  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupSetInfo(
    LPCWSTR servername,
    LPCWSTR groupname,
    DWORD level,
    LPBYTE buf,
    LPDWORD parm_err)
{
    FIXME("(%s %s %d %p %p) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), level, buf, parm_err);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupSetMember (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupSetMembers(
    LPCWSTR servername,
    LPCWSTR groupname,
    DWORD level,
    LPBYTE buf,
    DWORD totalentries)
{
    FIXME("(%s %s %d %p %d) stub!\n", debugstr_w(servername),
            debugstr_w(groupname), level, buf, totalentries);
    return NERR_Success;
}
