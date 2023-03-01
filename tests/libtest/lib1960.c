/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * SPDX-License-Identifier: curl
 *
 ***************************************************************************/
#include <sys/socket.h>
#include <arpa/inet.h>
#include "test.h"

#include "memdebug.h"

/* provide our own socket */
static curl_socket_t socket_cb(void *clientp,
                               curlsocktype purpose,
                               struct curl_sockaddr *address)
{
  int s = *(int *)clientp;
  (void)purpose;
  (void)address;
  return (curl_socket_t)s;
}

/* tell libcurl the socket is connected */
static int sockopt_cb(void *clientp,
                      curl_socket_t curlfd,
                      curlsocktype purpose)
{
  (void)clientp;
  (void)curlfd;
  (void)purpose;
  return CURL_SOCKOPT_ALREADY_CONNECTED;
}

/* Expected args: URL IP PORT */
int test(char *URL)
{
  CURL *curl;
  CURLcode res = TEST_ERR_MAJOR_BAD;
  struct curl_slist *list = NULL;
  struct curl_slist *connect_to = NULL;
  int status;
  int client_fd;
  struct sockaddr_in serv_addr;
  unsigned short port = (unsigned short)atoi(libtest_arg3);

  if(curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK) {
    fprintf(stderr, "curl_global_init() failed\n");
    return TEST_ERR_MAJOR_BAD;
  }

  /*
   * This code connects to the TCP port "manually" so that we then can hand
   * over this socket as "already connected" to libcurl and make sure that
   * this works.
   */
  client_fd = socket(AF_INET, SOCK_STREAM, 0);
  if(client_fd < 0) {
    fprintf(stderr, "socket creation error\n");
    return 1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);

  if(inet_pton(AF_INET, libtest_arg2, &serv_addr.sin_addr) <= 0) {
    fprintf(stderr, "inet_pton failed\n");
    return 2;
  }

  status = connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
  if(status < 0) {
    fprintf(stderr, "connection failed\n");
    return 3;
  }

  curl = curl_easy_init();
  if(!curl) {
    fprintf(stderr, "curl_easy_init() failed\n");
    curl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }

  test_setopt(curl, CURLOPT_VERBOSE, 1L);
  test_setopt(curl, CURLOPT_OPENSOCKETFUNCTION, socket_cb);
  test_setopt(curl, CURLOPT_OPENSOCKETDATA, &client_fd);
  test_setopt(curl, CURLOPT_SOCKOPTFUNCTION, sockopt_cb);
  test_setopt(curl, CURLOPT_SOCKOPTDATA, NULL);
  test_setopt(curl, CURLOPT_VERBOSE, 1L);
  test_setopt(curl, CURLOPT_HEADER, 1L);
  test_setopt(curl, CURLOPT_URL, URL);

  res = curl_easy_perform(curl);

test_cleanup:

  curl_slist_free_all(connect_to);
  curl_slist_free_all(list);
  curl_easy_cleanup(curl);
  curl_global_cleanup();

  return res;
}
