#include "http.c"

extern err_t http_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
extern err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err);
extern void start_http_server(void);