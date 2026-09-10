/* -*-  mode:c; tab-width:8; c-basic-offset:8; indent-tabs-mode:nil;  -*- */
/*
   Server-side srvsvc responder helpers.

   These let an SMB *server* answer the srvsvc named-pipe RPC that clients
   (macOS Finder, Windows Explorer) use to enumerate shares over IPC$, without
   the server having to touch libsmb2's internal DCE/RPC machinery. The heavy
   NDR marshalling is done inside libsmb2 (reusing the same coders the client
   decode path uses); the caller just supplies the share list and the call/
   context ids echoed from the client REQUEST PDU.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2.1 of the License, or
   (at your option) any later version.
*/

#ifndef _LIBSMB2_SRVSVC_SERVER_H_
#define _LIBSMB2_SRVSVC_SERVER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

struct smb2_context;

/*
 * Build a srvsvc NetrShareEnum (opnum 0x0f) level-1 RESPONSE PDU advertising
 * `nshares` shares into `out` (capacity `cap`). `names`/`types` are parallel
 * arrays (`types` uses the SRVSVC_SHARE_TYPE_* bits from libsmb2-share-enum.h).
 * `call_id` and `context_id` are echoed from the client's REQUEST PDU so the
 * reply matches. Shares are advertised with an empty remark.
 *
 * Returns the number of bytes written to `out`, or -1 on error. Does not
 * fragment: `cap` must be large enough for the whole reply.
 */
int smb2_srvsvc_server_netshareenum(struct smb2_context *smb2,
                                    uint32_t call_id, uint16_t context_id,
                                    const char *const *names,
                                    const uint32_t *types,
                                    int nshares,
                                    uint8_t *out, int cap);

#ifdef __cplusplus
}
#endif

#endif /* !_LIBSMB2_SRVSVC_SERVER_H_ */
