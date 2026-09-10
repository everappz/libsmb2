/* -*-  mode:c; tab-width:8; c-basic-offset:8; indent-tabs-mode:nil;  -*- */
/*
 * downstream extension — DCE/RPC server-side response builder.
 *
 * This file is NOT part of upstream libsmb2. It is a source *fragment* that is #included at the end
 * of the fork-only wrapper lib/libsmb2-dcerpc.c (right after "../libdcerpc/dcerpc.c"), so it compiles
 * in the SAME translation unit and can see the private `struct dcerpc_pdu` plus dcerpc.c's statics.
 * Keeping it in a separate, upstream-absent file means merges from upstream never touch it and never
 * conflict. Do NOT add this file to the build globs (pod: lib/*.c, SPM: sources ["lib"]); it is
 * include-only.
 *
 * Declared in <smb2/libsmb2-srvsvc-server.h>. Symbol is prefixed to libsmb2_dcerpc_server_build_response
 * by lib/libsmb2-dcerpc-prefix.h in the wrapper TU.
 */

int
dcerpc_server_build_response(struct dcerpc_context *dce,
                             uint32_t call_id, uint16_t context_id,
                             dcerpc_coder rep_coder, void *rep,
                             uint8_t *out, int cap)
{
        struct dcerpc_pdu *pdu;
        struct dcerpc_iovec iov;
        int offset = 0, o;
        int len;

        if (dce == NULL || rep_coder == NULL || out == NULL) {
                return -1;
        }

        pdu = dcerpc_allocate_pdu(dce, ENCODING_NDR, DCERPC_ENCODE, NSE_BUF_SIZE);
        if (pdu == NULL) {
                return -1;
        }

        pdu->hdr.rpc_vers = 5;
        pdu->hdr.rpc_vers_minor = 0;
        pdu->hdr.PTYPE = PDU_TYPE_RESPONSE;
        pdu->hdr.pfc_flags = PFC_FIRST_FRAG | PFC_LAST_FRAG;
        pdu->hdr.packed_drep[0] = dce->packed_drep[0];
        pdu->hdr.packed_drep[1] = 0;
        pdu->hdr.packed_drep[2] = 0;
        pdu->hdr.packed_drep[3] = 0;
        pdu->hdr.frag_length = 0;  /* fixed up below */
        pdu->hdr.auth_length = 0;
        pdu->hdr.call_id = call_id;

        iov.buf = pdu->payload;
        iov.len = NSE_BUF_SIZE;
        iov.free = NULL;

        /* 16-byte common header */
        if (dcerpc_header_coder(dce, pdu, &iov, &offset, &pdu->hdr)) {
                goto fail;
        }
        /* 8-byte RESPONSE header: alloc_hint(4) context_id(2) cancel_count(1)
         * reserved(1). alloc_hint is a placeholder, fixed up below. */
        if (dcerpc_set_uint32(dce, pdu, &iov, &offset, 0)) {
                goto fail;
        }
        if (dcerpc_set_uint16(dce, pdu, &iov, &offset, context_id)) {
                goto fail;
        }
        if (dcerpc_set_uint8(dce, &iov, &offset, 0)) {  /* cancel_count */
                goto fail;
        }
        if (dcerpc_set_uint8(dce, &iov, &offset, 0)) {  /* reserved */
                goto fail;
        }

        /* NDR stub, encoded exactly like the client request path: a top-level
         * coder that runs the conformance pass + deferred pointers itself. */
        pdu->top_level = 1;
        if (rep_coder("Response", dce, pdu, &iov, &offset, rep)) {
                goto fail;
        }

        len = offset;

        /* Fixup frag_length @8 and alloc_hint @16 (= stub length). */
        o = 8;
        if (dcerpc_set_uint16(dce, pdu, &iov, &o, (uint16_t)len)) {
                goto fail;
        }
        o = 16;
        if (dcerpc_set_uint32(dce, pdu, &iov, &o, (uint32_t)(len - 24))) {
                goto fail;
        }

        if (len > cap) {
                goto fail;
        }
        memcpy(out, iov.buf, (size_t)len);
        dcerpc_free_pdu(dce, pdu);
        return len;

 fail:
        dcerpc_free_pdu(dce, pdu);
        return -1;
}
