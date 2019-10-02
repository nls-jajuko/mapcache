/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  MapCache tile caching support file: WMTS single tile passthrough proxy
 * Author:   nls-jajuko@users.noreply.github.com
 *
 ******************************************************************************
 * 
 * Copyright (c) 1996-2011 Regents of the University of Minnesota.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of this Software or works derived from this Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *****************************************************************************/


#include "mapcache.h"
#include "ezxml.h"
#include <apr_tables.h>
#include <apr_strings.h>

typedef struct mapcache_source_wmts_proxy mapcache_source_wmts_proxy;
struct mapcache_source_wmts_proxy {
  mapcache_source source;
  mapcache_http *http;
};

/**
 * \private \memberof mapcache_source_wmts_proxy
 * \sa mapcache_source::render_map()
 */
void _mapcache_source_wmts_proxy_render_map(mapcache_context *ctx, mapcache_source *psource, mapcache_map *map)
{
   ctx->set_error(ctx,500,"dummy source does not support render map");
}


void _mapcache_source_wmts_proxy_proxy_map(mapcache_context *ctx, mapcache_source *source, mapcache_metatile *mt, mapcache_map *map)
{
    /* 
    mt->tiles[0].x;
    mt->tiles[0].y;
    mt->tiles[0].z;
    */
    mapcache_source_wmts_proxy *src = (mapcache_source_wmts_proxy*)source;
    mapcache_grid_link *grid_link = map->grid_link;
    mapcache_http *http;
    int col, row, matrix ;

   if(mt->tiles && mt->ntiles==1) {

    int x = mt->tiles[0].x, y = mt->tiles[0].y, level = mt->tiles[0].z;
    
    matrix = mt->tiles[0].z;
  
     /* revert col, row calculations */
    switch(grid_link->grid->origin) {
    case MAPCACHE_GRID_ORIGIN_BOTTOM_LEFT:
      col = x; // fixed
      row = grid_link->grid->levels[level]->maxy - y - 1; // fixed
      break;
    case MAPCACHE_GRID_ORIGIN_TOP_LEFT:
      col = x; // fixed
      row = y; // fixed
      break;
    case MAPCACHE_GRID_ORIGIN_BOTTOM_RIGHT:
      col = grid_link->grid->levels[level]->maxx - x - 1;
      row = grid_link->grid->levels[level]->maxy - y - 1;
      break;
    case MAPCACHE_GRID_ORIGIN_TOP_RIGHT:
      col = grid_link->grid->levels[level]->maxx - x - 1;
      row = y; // fixed
      break;
    default:
      ctx->set_error(ctx,500,"BUG: invalid grid origin");
      return;
    } 
    

    map->encoded_data = mapcache_buffer_create(30000,ctx->pool);
    http = mapcache_http_clone(ctx, src->http);
    http->url = apr_psprintf(ctx->pool,http->url, "WGS84_Pseudo-Mercator", matrix,row,col);
    ctx->log(ctx,MAPCACHE_WARN,"URL ZYX %s",http->url);   
    mapcache_http_do_request(ctx,http,map->encoded_data,NULL,NULL);
    GC_CHECK_ERROR(ctx);
    
   } else {
      ctx->set_error(ctx,500,"BUG: no tile");
   }
 }

void _mapcache_source_wmts_proxy_query(mapcache_context *ctx, mapcache_source *psource, mapcache_feature_info *fi)
{
  ctx->set_error(ctx,500,"dummy source does not support queries");
}

/**
 * \private \memberof mapcache_source_wmts_proxy
 * \sa mapcache_source::configuration_parse()
 */
void _mapcache_source_wmts_proxy_configuration_parse_xml(mapcache_context *ctx, ezxml_t node, mapcache_source *source, mapcache_cfg *config)
{

  ezxml_t cur_node;
  mapcache_source_wmts_proxy *src = (mapcache_source_wmts_proxy*)source;
  
  if ((cur_node = ezxml_child(node,"http")) != NULL) {
    src->http = mapcache_http_configuration_parse_xml(ctx,cur_node);
  }
}

/**
 * \private \memberof mapcache_source_wmts_proxy
 * \sa mapcache_source::configuration_check()
 */
void _mapcache_source_wmts_proxy_configuration_check(mapcache_context *ctx, mapcache_cfg *cfg,
    mapcache_source *source)
{
    mapcache_source_wmts_proxy *src = (mapcache_source_wmts_proxy*)source;

   if(!src->http) {
    ctx->set_error(ctx, 400, "wmts source %s has no <http> request configured",source->name);
  }
 }

mapcache_source* mapcache_source_wmts_proxy_create(mapcache_context *ctx)
{
  mapcache_source_wmts_proxy *source = apr_pcalloc(ctx->pool, sizeof(mapcache_source_wmts_proxy));
  if(!source) {
    ctx->set_error(ctx, 500, "failed to allocate dummy source");
    return NULL;
  }
  mapcache_source_init(ctx, &(source->source));
  source->source.type = MAPCACHE_SOURCE_WMTS_PROXY;
  source->source._render_map = _mapcache_source_wmts_proxy_render_map;
  source->source._proxy_map = _mapcache_source_wmts_proxy_proxy_map;
  source->source.configuration_check = _mapcache_source_wmts_proxy_configuration_check;
  source->source.configuration_parse_xml = _mapcache_source_wmts_proxy_configuration_parse_xml;
  source->source._query_info = _mapcache_source_wmts_proxy_query;

  return (mapcache_source*)source;
}


/* vim: ts=2 sts=2 et sw=2
*/
