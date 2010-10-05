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


static const char* fileinfo_to_str(apr_pool_t* pool, const char* name, const apr_finfo_t* finfo, char type)
{
  int mode = (finfo->filetype==APR_REG)? (0x100000): ( \
             (finfo->filetype==APR_DIR)? (0x040000): ( \
             (finfo->filetype==APR_LNK)? (0x020000): ( \
              0)));

  if(name[0]=='.') return NULL;
  if(mode==0) return NULL;

  if(type=='[') {
    // name of array
    return apr_psprintf(pool, "\"%s\"", name);;
  } else {
    // hash
    char ms[11] = "----------", mt[25] = "";
    apr_size_t mts = sizeof(mt);
    apr_time_exp_t tm;

    apr_time_exp_lt(&tm, finfo->mtime);
    apr_strftime(mt, &mts, mts, "%Y-%m-%dT%H:%M:%S%z", &tm);
    mode |= (finfo->protection & APR_FPROT_OS_DEFAULT);
    if(mode & 0x040000) ms[0]='d';
    if(mode & 0x020000) ms[0]='l';
    if(mode & APR_FPROT_UREAD)    ms[1]='r';
    if(mode & APR_FPROT_UWRITE)   ms[2]='w';
    if(mode & APR_FPROT_UEXECUTE) ms[3]='x';
    if(mode & APR_FPROT_GREAD)    ms[4]='r';
    if(mode & APR_FPROT_GWRITE)   ms[5]='w';
    if(mode & APR_FPROT_GEXECUTE) ms[6]='x';
    if(mode & APR_FPROT_WREAD)    ms[7]='r';
    if(mode & APR_FPROT_WWRITE)   ms[8]='w';
    if(mode & APR_FPROT_WEXECUTE) ms[9]='x';
    return apr_psprintf(pool, "\"%s\":{\"mode\":\"%s\",\"size\":%llu,\"mtime\":\"%s\"}", \
                        name, ms, (unsigned long long)finfo->size, mt);
  }
}

 
/* text/json handler */
static int index_json_handler(request_rec *r)
{
  static const apr_fileperms_t PERM = APR_FINFO_TYPE|APR_FINFO_UPROT|APR_FINFO_GPROT|APR_FINFO_WPROT|APR_FINFO_MTIME;
  apr_finfo_t finfo;
  const char* type;
  apr_dir_t* dir;

  if(strcmp(r->handler, "index_json")) {
    return DECLINED;
  }
  type = apr_table_get(r->headers_in, "Accept");
  if(type) {
    if((strcasecmp(type, "text/json")==0) || (strcasecmp(type, "text/json;array")==0)) {
      type = "[]";
    } else
    if(strcasecmp(type, "text/json;hash")==0) {
      type = "{}";
    } else return DECLINED;
  } else return DECLINED;

  if(apr_stat(&finfo, r->filename, PERM, r->pool)==APR_SUCCESS) {
    const char* stat = fileinfo_to_str(r->pool, r->parsed_uri.path+1, &r->finfo, '{');
    if(stat) {
      stat = apr_psprintf(r->pool, "{%s}", stat);      
      apr_table_set(r->headers_out, "X-FileStat-Json", stat);
    }
  }
  if(apr_dir_open(&dir, r->filename, r->pool)!=APR_SUCCESS) return DECLINED;

  r->content_type = "text/json";
  if(!r->header_only) {
    int count = 0;
    ap_rprintf(r, "%c", type[0]);
    while(apr_dir_read(&finfo, PERM, dir)==APR_SUCCESS) {
      const char* stat = fileinfo_to_str(r->pool, finfo.name, &finfo, type[0]);
      if(stat==NULL) continue;

      if(count>0) ap_rputs(",", r);
      ap_rprintf(r, "%s", stat);
      count++;
    }
    ap_rprintf(r, "%c\n", type[1]);
  }

  apr_dir_close(dir);
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

