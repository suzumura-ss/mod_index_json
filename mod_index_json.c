/* 
 * Copyright 2010 Toshiyuki Terashita
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "httpd.h"
#include "http_config.h"
#include "http_protocol.h"
#include "ap_config.h"
#include "apr_strings.h"
#include "apr_file_info.h"
#include "apr_tables.h"


/* text/json handler */
static int index_json_handler(request_rec *r)
{
  const char* hdr;

  if(strcmp(r->handler, "index_json")) {
    return DECLINED;
  }
  hdr = apr_table_get(r->headers_in, "Accept");
  if(!hdr || strcmp(hdr, "text/json")) {
    return DECLINED;
  }

  r->content_type = "text/json";

  if(!r->header_only) {
    apr_dir_t* dir;
    apr_finfo_t finfo;
    int count = 0;
    if(apr_dir_open(&dir, r->filename, r->pool)!=APR_SUCCESS) return DECLINED;
    ap_rputs("[", r);
    while(apr_dir_read(&finfo, 0/*APR_FINFO_XXX*/, dir)==APR_SUCCESS) {
      if(finfo.name[0]!='.') {
        if(count>0) ap_rputs(",", r);
        ap_rprintf(r, "\"%s\"", finfo.name);
        count++;
      }
    }
    ap_rputs("]\n", r);
    apr_dir_close(dir);
  }

  return OK;
}

static void index_json_register_hooks(apr_pool_t *p)
{
  ap_hook_handler(index_json_handler, NULL, NULL, APR_HOOK_MIDDLE);
}

/* Dispatch list for API hooks */
module AP_MODULE_DECLARE_DATA index_json_module = {
  STANDARD20_MODULE_STUFF, 
  NULL,                     /* create per-dir    config structures */
  NULL,                     /* merge  per-dir    config structures */
  NULL,                     /* create per-server config structures */
  NULL,                     /* merge  per-server config structures */
  NULL,                     /* table of config file commands       */
  index_json_register_hooks /* register hooks                      */
};

