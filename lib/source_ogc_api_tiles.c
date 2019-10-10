/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  MapCache tile caching support file: OGCF tiles combining proxy
 * Author:   nls-jajuko@users.noreply.github.com
 * Copyright (c) 2019 National Land Survey of Finland 
 *
 ******************************************************************************
 * 
 * Based on work by
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

typedef struct mapcache_source_ogc_api_tiles mapcache_source_ogc_api_tiles;
typedef struct mapcache_source_ogc_api_vtmap mapcache_source_ogc_api_vtmap;
typedef struct mapcache_source_ogc_api_vtmatrix mapcache_source_ogc_api_vtmatrix;


struct mapcache_source_ogc_api_tiles {
  mapcache_source source;    
  apr_array_header_t *maps;

};

struct mapcache_source_ogc_api_vtmap {
  char *name;
  char *version;
  char *tilematrixset ; // diff urls for diff tilematrixsets 
  apr_array_header_t *matrices;
};

struct mapcache_source_ogc_api_vtmatrix {
  int level;
  apr_array_header_t *urls;
};


static mapcache_source_ogc_api_vtmap* _ogc_api_vtmap_create(apr_pool_t *pool, char *name, char *ver, char* tilematrixset)
{
  mapcache_source_ogc_api_vtmap *vtmap = apr_pcalloc(pool, sizeof(mapcache_source_ogc_api_vtmap));
  vtmap->name = apr_pstrdup(pool, name);
  vtmap->version = apr_pstrdup(pool, ver);
  vtmap->tilematrixset = apr_pstrdup(pool, tilematrixset);
  vtmap->matrices = apr_array_make(pool,1,sizeof(mapcache_source_ogc_api_vtmatrix*));
  return vtmap;
}

static mapcache_source_ogc_api_vtmatrix* _ogc_api_vtmatrix_create(apr_pool_t *pool, int level)
{
  mapcache_source_ogc_api_vtmatrix *vtmat = apr_pcalloc(pool, sizeof(mapcache_source_ogc_api_vtmatrix));
  vtmat->level = level;
  vtmat->urls = apr_array_make(pool,1,sizeof(mapcache_http*));
  return vtmat;
}

mapcache_source_ogc_api_vtmap *_ogc_api_vtmap_get(mapcache_context *ctx, mapcache_source_ogc_api_tiles *src, const char *name, const char *ver, const char* grid)
{
    mapcache_source_ogc_api_vtmap *map = NULL;

    int i = src->maps->nelts;

    while(i--) {
      mapcache_source_ogc_api_vtmap *entry = APR_ARRAY_IDX(src->maps,i,mapcache_source_ogc_api_vtmap*);

      if( strcmp( name, entry->name) ) {
          continue;
      }

      if( ver && entry->version && strcmp( ver, entry->version) ) {
          continue;
      }

      if( grid && entry->tilematrixset && strcmp( grid, entry->tilematrixset) ) {
          continue;
      }

      map = entry;
    }

    return map;
}

void _ogc_api_vtmap_add(mapcache_source_ogc_api_tiles *src, mapcache_source_ogc_api_vtmap *map)
{
    APR_ARRAY_PUSH(src->maps,mapcache_source_ogc_api_vtmap*) = map;
}


mapcache_source_ogc_api_vtmatrix *_ogc_api_vtmatrix_get(mapcache_context *ctx, mapcache_source_ogc_api_vtmap *map, int level)
{
    mapcache_source_ogc_api_vtmatrix *matrix = NULL;

    int i = map->matrices->nelts;
    ctx->log(ctx,MAPCACHE_WARN,"ogc_api_tiles: checking %d matrices for %s %d",i, map->name,level);   

    while(i--) {
      mapcache_source_ogc_api_vtmatrix *entry = APR_ARRAY_IDX(map->matrices,i,mapcache_source_ogc_api_vtmatrix*);

      if( entry->level != level ) {
          continue;
      }
    
      ctx->log(ctx,MAPCACHE_WARN,"ogc_api_tiles: selecting %d matrix for %s %d",i, map->name,level);   

      matrix = entry;
    }

    return matrix;
}


/**
 * \private \memberof mapcache_source_ogc_api_tiles
 * \sa mapcache_source::render_map()
 */
void _mapcache_source_ogc_api_tiles_render_map(mapcache_context *ctx, mapcache_source *psource, mapcache_map *map)
{
   ctx->set_error(ctx,500,"dummy source does not support render map");
}


char* _mapcache_source_ogc_api_tiles_get_tile_url(mapcache_context *ctx,
  char *template, char* tilematrixset, int tilematrix, int tilerow, int tilecol, char *tilesetname, char *extension)
{
  char *path = apr_pstrdup(ctx->pool, template);

  if(strstr(path,"{TileCol}"))
    path = mapcache_util_str_replace(ctx->pool,path, "{TileCol}", apr_psprintf(ctx->pool,"%d",tilecol));
  if(strstr(path,"{TileRow}"))
    path = mapcache_util_str_replace(ctx->pool,path, "{TileRow}", apr_psprintf(ctx->pool,"%d",tilerow));
  if(strstr(path,"{TileMatrix}"))
    path = mapcache_util_str_replace(ctx->pool,path, "{TileMatrix}", apr_psprintf(ctx->pool,"%d",tilematrix));
  if(strstr(path,"{TileMatrixSet}"))
    path = mapcache_util_str_replace(ctx->pool,path, "{TileMatrixSet}", tilematrixset);
  if(strstr(path,"{TileSetName}"))
    path = mapcache_util_str_replace(ctx->pool,path, "{TileSetName}", tilesetname);
  if(strstr(path,"{ext}"))
    path = mapcache_util_str_replace(ctx->pool,path, "{ext}", extension);


  return path;
}


char* _mapcache_source_ogc_api_get_requested_dimension(apr_array_header_t *dimensions, const char *name ) {
  int i;
  if(!dimensions || dimensions->nelts <= 0) {
    return NULL;
  }
  for(i=0;i<dimensions->nelts;i++) {
    mapcache_requested_dimension *dim = APR_ARRAY_IDX(dimensions,i,mapcache_requested_dimension*);

    if(!strcasecmp(dim->dimension->name,name)) {
      return dim->requested_value;
    }
  }
  return NULL;
}

char* _mapcache_source_ogc_api_get_default_dimension(apr_array_header_t *dimensions, const char *name ) {
  int i;
  if(!dimensions || dimensions->nelts <= 0) {
    return NULL;
  }
  for(i=0;i<dimensions->nelts;i++) {
    mapcache_requested_dimension *dim = APR_ARRAY_IDX(dimensions,i,mapcache_requested_dimension*);

    if(!strcasecmp(dim->dimension->name,name)) {
      return dim->dimension->default_value;
    }
  }
  return NULL;
}


void _mapcache_source_ogc_api_tiles_proxy_map(mapcache_context *ctx, mapcache_source *source, mapcache_metatile *mt, mapcache_map *map)
{
  mapcache_source_ogc_api_tiles *src = (mapcache_source_ogc_api_tiles*)source;
  mapcache_grid_link *grid_link = map->grid_link;
  mapcache_source_ogc_api_vtmap *vtmap;
  mapcache_source_ogc_api_vtmatrix *vtmatrix;
   
  int col, row, matrix, x, y, level, i ;
  char *tilematrixset = grid_link->grid->name;
  char *extension = map->tileset->format->extension;
  char *tilesetname = map->tileset->name;
  char *tileversion = NULL;
  
  if(!mt->tiles || mt->ntiles!=1) {
    ctx->set_error(ctx,500,"BUG: no tile or metatile");
    return;
  }

  if( map->dimensions ) {
    tileversion = _mapcache_source_ogc_api_get_requested_dimension(mt->tiles[0].dimensions,"TileVersion");
  } 
  if( !tileversion || !strcasecmp(tileversion,"latest") || !strcasecmp(tileversion,"default") ) {
    tileversion = _mapcache_source_ogc_api_get_default_dimension(mt->tiles[0].dimensions,"TileVersion");
  }

  x = mt->tiles[0].x;
  y = mt->tiles[0].y;
  level = mt->tiles[0].z;
  matrix = mt->tiles[0].z;

  /* revert x,y to col,row values */
  switch(grid_link->grid->origin) {
    case MAPCACHE_GRID_ORIGIN_BOTTOM_LEFT:
      col = x; 
      row = grid_link->grid->levels[level]->maxy - y - 1; 
      break;
    case MAPCACHE_GRID_ORIGIN_TOP_LEFT:
      col = x;
      row = y; 
      break;
    case MAPCACHE_GRID_ORIGIN_BOTTOM_RIGHT:
      col = grid_link->grid->levels[level]->maxx - x - 1;
      row = grid_link->grid->levels[level]->maxy - y - 1;
      break;
    case MAPCACHE_GRID_ORIGIN_TOP_RIGHT:
      col = grid_link->grid->levels[level]->maxx - x - 1;
      row = y; 
      break;
    default:
      ctx->set_error(ctx,500,"BUG: invalid grid origin");
      return;

  } 

  ctx->log(ctx,MAPCACHE_WARN,"ogc_api_tiles: map for %s tilematrixset %s version %s",tilesetname,
      tilematrixset, tileversion ? tileversion : "*");   

  vtmap = _ogc_api_vtmap_get(ctx,src,tilesetname,tileversion,tilematrixset);
  if(!vtmap) {
    ctx->set_error(ctx,404,"not found");
    return;
  }

  ctx->log(ctx,MAPCACHE_WARN,"ogc_api_tiles: matrix for %s %d",tilesetname,matrix);   

  vtmatrix = _ogc_api_vtmatrix_get(ctx,vtmap, matrix);
  if(!vtmatrix) {
    ctx->set_error(ctx,404,"not found (no matrix in %s for %d)",tilesetname, matrix);
    return;
  }


  map->encoded_data = mapcache_buffer_create(30000,ctx->pool);


  i = vtmatrix->urls->nelts;

  ctx->log(ctx,MAPCACHE_WARN,"ogc_api_tiles: fetching tiles...%d",i);   
  while(i--) {
    mapcache_http *http, *entry = APR_ARRAY_IDX(vtmatrix->urls,i,mapcache_http*);

    ctx->log(ctx,MAPCACHE_WARN,"ogc_api_tiles: fetching tile %s",entry->url);   

    http = mapcache_http_clone(ctx, entry);
    http->url = _mapcache_source_ogc_api_tiles_get_tile_url(ctx, entry->url, tilematrixset, matrix, row, col, tilesetname, extension);
  
    ctx->log(ctx,MAPCACHE_WARN,"URL %s from Template %s",http->url, entry->url);   
    mapcache_http_do_request(ctx,http,map->encoded_data,NULL,NULL);
    GC_CHECK_ERROR(ctx);
  }

  GC_CHECK_ERROR(ctx);
 }

void _mapcache_source_ogc_api_tiles_query(mapcache_context *ctx, mapcache_source *psource, mapcache_feature_info *fi)
{
  ctx->set_error(ctx,500,"ogc_api_tiles source does not support queries");
}

/**
 * \private \memberof mapcache_source_ogc_api_tiles
 * \sa mapcache_source::configuration_parse()
 */
void _mapcache_source_ogc_api_tiles_configuration_parse_xml(mapcache_context *ctx, ezxml_t node, mapcache_source *source, mapcache_cfg *config)
{

  ezxml_t map_node, grid_node, matrix_node, http_node;
  mapcache_source_ogc_api_tiles *src = (mapcache_source_ogc_api_tiles*)source;
  mapcache_source_ogc_api_vtmap *map;

  for(map_node = ezxml_child(node,"map"); map_node; map_node = map_node->next) {

    char *name = (char*)ezxml_attr(map_node,"name");
    char *ver = (char*)ezxml_attr(map_node,"version");
    char *grid = (char*)ezxml_attr(map_node,"grid");
    mapcache_source_ogc_api_vtmatrix *matrix;

    grid_node = ezxml_child(node,"grid");

    if( !name) {
        ctx->set_error(ctx, 400, "ogc_api_tiles: missing name map configuration");
        return;
    }
    if( grid_node ) {
        grid = grid_node->txt;
    }
    if( !grid) {
        grid = "*";
    }
    if( !ver) {
        ver = "*";
    }

    ctx->log(ctx,MAPCACHE_WARN,"ogc_api_tiles: creating MAP %s (%s,%s)",name,ver,grid);   

    map = _ogc_api_vtmap_create(ctx->pool,name,ver,grid);

    for(matrix_node = ezxml_child(map_node,"matrix"); matrix_node; matrix_node = matrix_node->next) {
        char *level = (char*)ezxml_attr(matrix_node,"level");
        int z ;
        if( level) {
            z = atoi(level);
        }
        matrix = _ogc_api_vtmatrix_create(ctx->pool, z);

        ctx->log(ctx,MAPCACHE_WARN,"ogc_api_tiles: add matrix %d to  MAP %s", z, map->name);

        for(http_node = ezxml_child(matrix_node,"http"); http_node; http_node = http_node->next) {
            mapcache_http *http = mapcache_http_configuration_parse_xml(ctx,http_node);
            if(!http) {
               ctx->set_error(ctx, 400, "ogc_api_tiles: failed to parse http for matrix");
               return;
            }
            ctx->log(ctx,MAPCACHE_WARN,"ogc_api_tiles: add http %s to matrix %d in MAP %s", http->url, z, map->name);

            APR_ARRAY_PUSH(matrix->urls,mapcache_http*) = http;
         }
        APR_ARRAY_PUSH(map->matrices,mapcache_source_ogc_api_vtmatrix*) = matrix;
    }
    _ogc_api_vtmap_add(src,map);
  }
  


  // map -> matrix -> http 
}

/**
 * \private \memberof mapcache_source_ogc_api_tiles
 * \sa mapcache_source::configuration_check()
 */
void _mapcache_source_ogc_api_tiles_configuration_check(mapcache_context *ctx, mapcache_cfg *cfg,
    mapcache_source *source)
{
  mapcache_source_ogc_api_tiles *src = (mapcache_source_ogc_api_tiles*)source;
  int i = src->maps->nelts;

  if(!i) {
    ctx->set_error(ctx, 400, "ogc_api_tiles: missing map definitions");
  }
 }

mapcache_source* mapcache_source_ogc_api_tiles_create(mapcache_context *ctx)
{
  mapcache_source_ogc_api_tiles *source = apr_pcalloc(ctx->pool, sizeof(mapcache_source_ogc_api_tiles));
  if(!source) {
    ctx->set_error(ctx, 500, "failed to allocate ogc_api_tiles source");
    return NULL;
  }
  mapcache_source_init(ctx, &(source->source));
  source->source.type = MAPCACHE_SOURCE_OGC_API_TILES;
  source->source._render_map = _mapcache_source_ogc_api_tiles_render_map;
  source->source._proxy_map = _mapcache_source_ogc_api_tiles_proxy_map;
  source->source.configuration_check = _mapcache_source_ogc_api_tiles_configuration_check;
  source->source.configuration_parse_xml = _mapcache_source_ogc_api_tiles_configuration_parse_xml;
  source->source._query_info = _mapcache_source_ogc_api_tiles_query;

  source->maps = apr_array_make(ctx->pool,1,sizeof(mapcache_source_ogc_api_vtmap*));


  return (mapcache_source*)source;
}


/* vim: ts=2 sts=2 et sw=2
*/
