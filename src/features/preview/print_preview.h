#pragma once
/* Called from the UI/main loop; network/decode work runs in a bounded worker. */
void xtouch_print_preview_poll();
// ftpCleanupResult: 0=not run, 1=FTP confirmed, 2=failed, 3=intentional socket release.
struct XTouchPrintPreviewStatus {bool busy,visible;unsigned width,height,attempts;int result;unsigned stage;int httpStatus;int tlsError;unsigned renderedLines;unsigned tlsPhase;int jsonError;unsigned workerStackBytes,workerStackMin;int ftpTransferReply,ftpAbortReply,ftpQuitReply;unsigned ftpCleanupResult;};
XTouchPrintPreviewStatus xtouch_print_preview_status();

/* Fullscreen camera owns image/network resources until it closes. */
void xtouch_print_preview_suspend(bool suspended);
