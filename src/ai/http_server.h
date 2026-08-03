#ifndef GARLIC_HTTP_SERVER_H
#define GARLIC_HTTP_SERVER_H

/**
 * Minimal HTTP static file server.
 *
 * @param root_dir  Directory to serve files from (e.g. analysis output dir).
 * @param port      TCP port to listen on (e.g. 8080).
 * @return 0 on clean shutdown, -1 on error.
 */
int http_serve(const char *root_dir, int port);

#endif /* GARLIC_HTTP_SERVER_H */
