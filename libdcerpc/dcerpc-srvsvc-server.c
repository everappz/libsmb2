/* -*-  mode:c; tab-width:8; c-basic-offset:8; indent-tabs-mode:nil;  -*- */
/*
 * downstream extension — srvsvc server-side responders (NetrShareEnum / NetrShareGetInfo /
 * NetrServerGetInfo).
 *
 * NOT part of upstream libsmb2. Source *fragment* #included at the end of the fork-only wrapper
 * lib/libsmb2-dcerpc-srvsvc.c (right after "../libdcerpc/dcerpc-srvsvc.c"), so it compiles in the
 * SAME translation unit and can reach the srvsvc coders/structs + core NDR primitives. Keeping it in
 * a separate, upstream-absent file means upstream merges never touch it. Do NOT add it to the build
 * globs (pod: lib/*.c, SPM: sources ["lib"]); it is include-only.
 *
 * Public entry points (smb2_srvsvc_server_*) are declared in <smb2/libsmb2-srvsvc-server.h> and are
 * NOT symbol-prefixed, so the ObjC server wrapper links them directly. They call the internal
 * dcerpc_server_build_response (defined in dcerpc-server-response.c, in the other wrapper TU), which
 * IS prefixed by lib/libsmb2-dcerpc-prefix.h; the forward declaration below is renamed by that same
 * prefix macro in this TU, so it resolves to the prefixed definition at link time.
 */

/* Defined in dcerpc-server-response.c (dcerpc.c wrapper TU); prefixed by the wrapper. */
int dcerpc_server_build_response(struct dcerpc_context *dce,
                                 uint32_t call_id, uint16_t context_id,
                                 dcerpc_coder rep_coder, void *rep,
                                 uint8_t *out, int cap);

#define SMB2_SRVSVC_SERVER_MAX_SHARES 64

/*
 * NetrShareEnum (opnum 0x0f), level 1: reuses the always-compiled srvsvc_NetrShareEnum_rep_coder.
 */
int
smb2_srvsvc_server_netshareenum(struct smb2_context *smb2,
                                uint32_t call_id, uint16_t context_id,
                                const char *const *names,
                                const uint32_t *types,
                                int nshares,
                                uint8_t *out, int cap)
{
        struct dcerpc_context *dce;
        struct srvsvc_SHARE_INFO_1 shares[SMB2_SRVSVC_SERVER_MAX_SHARES];
        struct srvsvc_NetrShareEnum_rep rep;
        int i, n;

        if (smb2 == NULL || out == NULL || names == NULL || types == NULL ||
            nshares < 0) {
                return -1;
        }
        if (nshares > SMB2_SRVSVC_SERVER_MAX_SHARES) {
                nshares = SMB2_SRVSVC_SERVER_MAX_SHARES;
        }

        memset(shares, 0, sizeof(shares));
        for (i = 0; i < nshares; i++) {
                shares[i].netname = discard_const(names[i] ? names[i] : "");
                shares[i].type = types[i];
                shares[i].remark = discard_const("");
        }

        memset(&rep, 0, sizeof(rep));
        rep.ses.Level = 1;
        rep.ses.ShareEnum.Level1.EntriesRead = (uint32_t)nshares;
        rep.ses.ShareEnum.Level1.share_info_1 = shares;
        rep.total_entries = (uint32_t)nshares;
        rep.resume_handle = 0;
        rep.status = 0;

        dce = dcerpc_create_context(smb2);
        if (dce == NULL) {
                return -1;
        }
        n = dcerpc_server_build_response(dce, call_id, context_id,
                                         srvsvc_NetrShareEnum_rep_coder, &rep,
                                         out, cap);
        dcerpc_destroy_context(dce);
        return n;
}

/*
 * NetrShareGetInfo (opnum 0x10), level 1.
 *
 * The real srvsvc_NetrShareGetInfo_rep_coder + srvsvc_SHARE_INFO_STRUCT_coder live behind
 * HAVE_DCERPC_FULL (they reference SHARE_INFO_502, which pulls in MS-DTYP types that collide with
 * the Windows SDK). These self-contained coders replicate the SAME wire layout for level 1 only,
 * reusing the always-compiled srvsvc_SHARE_INFO_1_STRUCT_coder, so no guarded code is needed.
 */
static int
sc_share_info_union_l1(char *name, struct dcerpc_context *dce, struct dcerpc_pdu *pdu,
                       struct dcerpc_iovec *iov, int *offset, void *ptr)
{
        union srvsvc_SHARE_INFO *info = ptr;
        return dcerpc_ptr_coder("ShareInfo1", dce, pdu, iov, offset, &info->ShareInfo1,
                                PTR_UNIQUE, srvsvc_SHARE_INFO_1_STRUCT_coder);
}
static int
sc_share_info_struct_l1(char *name, struct dcerpc_context *dce, struct dcerpc_pdu *pdu,
                        struct dcerpc_iovec *iov, int *offset, void *ptr)
{
        uint32_t level = 1;
        return dcerpc_union_coder("InfoStruct", dce, pdu, iov, offset, &level, ptr,
                                  sc_share_info_union_l1);
}
static int
sc_netsharegetinfo_rep(char *name, struct dcerpc_context *dce, struct dcerpc_pdu *pdu,
                       struct dcerpc_iovec *iov, int *offset, void *ptr)
{
        struct srvsvc_NetrShareGetInfo_rep *rep = ptr;
        dcerpc_set_switch_is(pdu, 1);
        if (dcerpc_ptr_coder("InfoStruct", dce, pdu, iov, offset, &rep->InfoStruct,
                             PTR_REF, sc_share_info_struct_l1)) {
                return -1;
        }
        if (dcerpc_uint32_coder("Status", dce, pdu, iov, offset, &rep->status)) {
                return -1;
        }
        return 0;
}

int
smb2_srvsvc_server_netsharegetinfo(struct smb2_context *smb2,
                                   uint32_t call_id, uint16_t context_id,
                                   const char *name, uint32_t type,
                                   uint8_t *out, int cap)
{
        struct dcerpc_context *dce;
        struct srvsvc_NetrShareGetInfo_rep rep;
        int n;

        if (smb2 == NULL || out == NULL) {
                return -1;
        }
        memset(&rep, 0, sizeof(rep));
        rep.InfoStruct.ShareInfo1.netname = discard_const(name ? name : "");
        rep.InfoStruct.ShareInfo1.type = type;
        rep.InfoStruct.ShareInfo1.remark = discard_const("");
        rep.status = 0;

        dce = dcerpc_create_context(smb2);
        if (dce == NULL) {
                return -1;
        }
        n = dcerpc_server_build_response(dce, call_id, context_id,
                                         sc_netsharegetinfo_rep, &rep, out, cap);
        dcerpc_destroy_context(dce);
        return n;
}

/*
 * NetrServerGetInfo (opnum 0x15), level 101.
 *
 * SERVER_INFO_101's real coder is also behind HAVE_DCERPC_FULL (and uses YAML/JSON pretty-printers).
 * SERVER_INFO_101 needs no MS-DTYP types, so this self-contained coder encodes it with plain
 * primitives, matching srvsvc_SERVER_INFO_101_coder's field order.
 */
static int
sc_server_info_101(char *name, struct dcerpc_context *dce, struct dcerpc_pdu *pdu,
                   struct dcerpc_iovec *iov, int *offset, void *ptr)
{
        struct srvsvc_SERVER_INFO_101 *si = ptr;
        if (dcerpc_uint32_coder("Platform_Id", dce, pdu, iov, offset, &si->platform_id)) return -1;
        if (dcerpc_ptr_coder("Name", dce, pdu, iov, offset, &si->name, PTR_UNIQUE, dcerpc_utf16z_coder)) return -1;
        if (dcerpc_uint32_coder("Version_Major", dce, pdu, iov, offset, &si->version_major)) return -1;
        if (dcerpc_uint32_coder("Version_Minor", dce, pdu, iov, offset, &si->version_minor)) return -1;
        if (dcerpc_uint32_coder("Type", dce, pdu, iov, offset, &si->type)) return -1;
        if (dcerpc_ptr_coder("Comment", dce, pdu, iov, offset, &si->comment, PTR_UNIQUE, dcerpc_utf16z_coder)) return -1;
        return 0;
}
static int
sc_server_info_101_struct(char *name, struct dcerpc_context *dce, struct dcerpc_pdu *pdu,
                          struct dcerpc_iovec *iov, int *offset, void *ptr)
{
        return dcerpc_struct_coder("InfoStruct", dce, pdu, iov, offset, ptr, sc_server_info_101);
}
static int
sc_server_info_union_l101(char *name, struct dcerpc_context *dce, struct dcerpc_pdu *pdu,
                          struct dcerpc_iovec *iov, int *offset, void *ptr)
{
        union srvsvc_SERVER_INFO *info = ptr;
        return dcerpc_ptr_coder("ServerInfo101", dce, pdu, iov, offset, &info->ServerInfo101,
                                PTR_UNIQUE, sc_server_info_101_struct);
}
static int
sc_server_info_struct_l101(char *name, struct dcerpc_context *dce, struct dcerpc_pdu *pdu,
                           struct dcerpc_iovec *iov, int *offset, void *ptr)
{
        uint32_t level = 101;
        return dcerpc_union_coder("InfoStruct", dce, pdu, iov, offset, &level, ptr,
                                  sc_server_info_union_l101);
}
static int
sc_netservergetinfo_rep(char *name, struct dcerpc_context *dce, struct dcerpc_pdu *pdu,
                        struct dcerpc_iovec *iov, int *offset, void *ptr)
{
        struct srvsvc_NetrServerGetInfo_rep *rep = ptr;
        dcerpc_set_switch_is(pdu, 101);
        if (dcerpc_ptr_coder("InfoStruct", dce, pdu, iov, offset, &rep->InfoStruct,
                             PTR_REF, sc_server_info_struct_l101)) {
                return -1;
        }
        if (dcerpc_uint32_coder("Status", dce, pdu, iov, offset, &rep->status)) {
                return -1;
        }
        return 0;
}

int
smb2_srvsvc_server_netservergetinfo(struct smb2_context *smb2,
                                    uint32_t call_id, uint16_t context_id,
                                    const char *server_name, const char *comment,
                                    uint8_t *out, int cap)
{
        struct dcerpc_context *dce;
        struct srvsvc_NetrServerGetInfo_rep rep;
        int n;

        if (smb2 == NULL || out == NULL) {
                return -1;
        }
        memset(&rep, 0, sizeof(rep));
        rep.InfoStruct.ServerInfo101.platform_id = 500;               /* PLATFORM_ID_NT */
        rep.InfoStruct.ServerInfo101.name = discard_const(server_name ? server_name : "");
        rep.InfoStruct.ServerInfo101.version_major = 6;
        rep.InfoStruct.ServerInfo101.version_minor = 1;
        rep.InfoStruct.ServerInfo101.type = 0x00000003;               /* WORKSTATION | SERVER */
        rep.InfoStruct.ServerInfo101.comment = discard_const(comment ? comment : "");
        rep.status = 0;

        dce = dcerpc_create_context(smb2);
        if (dce == NULL) {
                return -1;
        }
        n = dcerpc_server_build_response(dce, call_id, context_id,
                                         sc_netservergetinfo_rep, &rep, out, cap);
        dcerpc_destroy_context(dce);
        return n;
}
